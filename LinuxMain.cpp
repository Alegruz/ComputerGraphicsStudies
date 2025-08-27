#include "pch.hpp"

#if defined(CGS_LINUX)
#include "CommandLineParser.h"
#include "Renderer.hpp"

#include <iostream>

static bool gIsRunning = true;

namespace cgs
{
    std::vector<std::filesystem::path> gRecentFiles;
    std::vector<Texture> gBackBuffers(BACK_BUFFERS_COUNT, Texture(1600, 900));
    static RenderThreadInfo gRenderThread;
}

// Wayland objects we need
static wl_display* gDisplay = nullptr;
static wl_registry* gRegistry = nullptr;
static wl_compositor* gCompositor = nullptr;
static xdg_wm_base* gWmBase = nullptr;
static wl_surface* gSurface = nullptr;
static xdg_surface* gXdgSurface = nullptr;
static xdg_toplevel* gTopLevel = nullptr;
static wl_shm* gShm = nullptr;   // <-- SHM
static wl_callback* gFrameCallback = nullptr;

static bool gIsFrameReady = true;

// Input (for popups & clicks)
static wl_seat*         gSeat       = nullptr;
static wl_pointer*      gPointer    = nullptr;
static uint32           gLastButtonSerial = 0;
static int              gPointerX   = 0;
static int              gPointerY   = 0;

// ---- Very small SHM helper ----
struct ShmBuffer final
{
    wl_buffer*  Buffer = nullptr;
    void*       Data = nullptr;
    int         Width = 0;
    int         Height = 0;
    int         Stride = 0;
    size_t      Size = 0;
};
static ShmBuffer gShmBuffer;

static int
CreateShmFile(size_t size) noexcept
{
    // Use POSIX shm (portable). Name auto-unlinked after open.
    char name[] = "/cgs-shm-XXXXXX";
    int fileDescriptor = -1;

    // mkstemp on /dev/shm via shm_open portable pattern
    // 1) make a unique name
    for (size_t i = 0; i < 6; ++i)
    {
        name[sizeof(name) - 6 + i] = static_cast<char>('A' + (rand() % 26));
    }
    fileDescriptor = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fileDescriptor < 0)
    {
        return -1;
    }
    shm_unlink(name); // unlink name; fileDescriptor stays open

    if (ftruncate(fileDescriptor, static_cast<off_t>(size)) < 0)
    {
        close(fileDescriptor);
        return -1;
    }
    return fileDescriptor;
}

static void
DestroyShmBuffer(ShmBuffer& shmBuffer) noexcept
{
    if (shmBuffer.Buffer)
    {
        wl_buffer_destroy(shmBuffer.Buffer);
    }

    if (shmBuffer.Data)
    {
        munmap(shmBuffer.Data, shmBuffer.Size);
    }

    shmBuffer = { 0 };
}

static bool
CreateShmBuffer(ShmBuffer& shmBuffer,
                int width,
                int height, 
                uint32 colorARGB) noexcept
{
    DestroyShmBuffer(shmBuffer);
    shmBuffer.Width = width;
    shmBuffer.Height = height;
    shmBuffer.Stride = width * 4;                 // WL_SHM_FORMAT_ARGB8888
    shmBuffer.Size = static_cast<size_t>(shmBuffer.Stride) * static_cast<size_t>(height);

    int fileDescriptor = CreateShmFile(shmBuffer.Size);
    if (fileDescriptor < 0)
    {
        std::fprintf(stderr, "CreateShmFile failed\n");
        return false;
    }

    void* data = mmap(nullptr, shmBuffer.Size, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);
    if (data == MAP_FAILED)
    {
        std::fprintf(stderr, "mmap failed\n");
        close(fileDescriptor);
        return false;
    }

    wl_shm_pool* pool = wl_shm_create_pool(gShm, fileDescriptor, static_cast<int>(shmBuffer.Size));
    close(fileDescriptor); // pool now owns the fileDescriptor size reference

    shmBuffer.Buffer = wl_shm_pool_create_buffer(pool, 0,
        width, height, shmBuffer.Stride,
        WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);

    shmBuffer.Data = data;

    // Paint a solid color (opaque teal) so we see something.
    uint32* px = static_cast<uint32*>(shmBuffer.Data);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            px[y * (shmBuffer.Stride / 4) + x] = colorARGB;
        }
    }
    return true;
}

// recent list (labels only; hook up real MRU later)
static std::vector<std::string> gRecent = {};

static void
OnFrameDone(void*,
            wl_callback* callBack,
            uint32)
{
    wl_callback_destroy(callBack);
    gFrameCallback = nullptr;
    gIsFrameReady = true;
}

static constexpr wl_callback_listener gFrameListener = { .done = OnFrameDone };

// ----- xdg_wm_base (ping/pong) -----
static void
XdgWmBasePing(void*,
              xdg_wm_base* wm,
              uint32 serial)
{
    xdg_wm_base_pong(wm, serial);
}

static const xdg_wm_base_listener gWmBaseListener =
{
    .ping = XdgWmBasePing
};

// ----- xdg_surface -----
static void
XdgSurfaceConfigure(void*,
                    xdg_surface* surf,
                    uint32 serial)
{
    // Must ack every configure
    xdg_surface_ack_configure(surf, serial);

    // Determine a size to draw. Compositor may suggest 0,0; pick a default.
    const int width = static_cast<int>(cgs::gBackBuffers[0].GetWidth());
    const int height = static_cast<int>(cgs::gBackBuffers[0].GetHeight());

    if (!gShmBuffer.Buffer || gShmBuffer.Width != width || gShmBuffer.Height != height)
    {
        if (!CreateShmBuffer(gShmBuffer, width, height, 0xFF20AAAA))
        {
            std::fprintf(stderr, "Failed to create shm buffer\n");
            return;
        }
    }
}

static const xdg_surface_listener gXdgSurfaceListener =
{
    .configure = XdgSurfaceConfigure
};

// ----- xdg_toplevel -----
static void
XdgToplevelConfigure(void*,
                     xdg_toplevel*,
                     int32 /* width */,
                     int32 /* height */,
                     wl_array* /*states*/)
{}

static void
XdgToplevelClose(void*,
                   xdg_toplevel*)
{
    gIsRunning = false; // like WM_DESTROY -> quit
}

// Newer xdg-shell adds these (keep empty if unused)
static void
XdgToplevelConfigureBound(void*,
                          xdg_toplevel*,
                          int32,
                          int32)
{
}

static void
XdgToplevelWmCapabilities(void*,
                          xdg_toplevel*,
                          wl_array*)
{
}

static constexpr xdg_toplevel_listener gTopLevelListener =
{
    .configure = XdgToplevelConfigure,
    .close = XdgToplevelClose,
    .configure_bounds = XdgToplevelConfigureBound,
    .wm_capabilities = XdgToplevelWmCapabilities
};

// Seat/pointer
static void 
PointerEnter(void*, 
             wl_pointer*, 
             [[maybe_unused]] uint32 serial, 
             wl_surface*, 
             wl_fixed_t sx, 
             wl_fixed_t sy) 
{
    // gLastButtonSerial = serial;
    gPointerX = wl_fixed_to_int(sx);
    gPointerY = wl_fixed_to_int(sy);
}

static void 
PointerLeave(void*, 
             wl_pointer*, 
             [[maybe_unused]] uint32 serial, 
             wl_surface*)
{
    // gLastButtonSerial = serial;
    gPointerX = gPointerY = -1;
}

static void 
PointerMotion(void*, 
              wl_pointer*, 
              uint32 /*time*/, 
              wl_fixed_t sx, 
              wl_fixed_t sy)
{
    gPointerX = wl_fixed_to_int(sx);
    gPointerY = wl_fixed_to_int(sy);
}

static void 
PointerButton(void*, 
              wl_pointer*, 
              uint32 serial, 
              uint32 /*time*/, 
              [[maybe_unused]] uint32 button, 
              uint32 /*state*/)
{
    gLastButtonSerial = serial;
}

static void
PointerAxis(void*, 
            wl_pointer*, 
            uint32, 
            uint32, 
            wl_fixed_t)
{}

static void 
PointerAxisSource(void*, 
                  wl_pointer*, 
                  uint32)
{}
static void 
PointerAxisStop(void*, 
                wl_pointer*, 
                uint32, 
                uint32)
{}
static void 
PointerAxisDiscrete(void*, 
                    wl_pointer*, 
                    uint32, 
                    int32)
{}
static void 
PointerAxisValue120(void*, 
                   wl_pointer*, 
                   uint32, 
                   int32)
{}
static void 
PointerFrame(void*, 
             wl_pointer*)
{
}
static void
PointerAxisRelativeDirection(void*, 
                             wl_pointer*, 
                             uint32, 
                             uint32)
{}

static constexpr wl_pointer_listener gPointerListener = 
{
    .enter = PointerEnter,
    .leave = PointerLeave,
    .motion = PointerMotion,
    .button = PointerButton,
    .axis = PointerAxis,
    .frame = PointerFrame,
    .axis_source = PointerAxisSource,
    .axis_stop = PointerAxisStop,
    .axis_discrete = PointerAxisDiscrete,
    .axis_value120 = PointerAxisValue120,
    .axis_relative_direction = PointerAxisRelativeDirection
};

static void 
SeatCapabilities(void*, 
                 wl_seat* seat, 
                 uint32 caps) 
{
    const bool wantPointer = caps & WL_SEAT_CAPABILITY_POINTER;
    if (wantPointer && !gPointer) 
    {
        gPointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(gPointer, &gPointerListener, nullptr);
    } 
    else if (!wantPointer && gPointer)
    {
        wl_pointer_destroy(gPointer); gPointer = nullptr;
    }
}
static void 
SeatName(void*, 
         wl_seat*, 
         const char*)
{}
static constexpr wl_seat_listener gSeatListener = 
{ 
    .capabilities = SeatCapabilities, 
    .name = SeatName 
};

// ----- wl_registry: discover globals -----
static void
RegistryGlobal(void*,
                wl_registry* reg,
                uint32 name,
                const char* interface,
                [[maybe_unused]] uint32 version)
{
    if (std::strcmp(interface, wl_compositor_interface.name) == 0)
    {
        // Bind compositor; version 4 is common and safe
        gCompositor = static_cast<wl_compositor*>(
            wl_registry_bind(reg, name, &wl_compositor_interface, 4));
    }
    else if (std::strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        // Bind xdg_wm_base (shell); version 1 is sufficient for basics
        gWmBase = static_cast<xdg_wm_base*>(wl_registry_bind(reg, name, &xdg_wm_base_interface, 1));
    }
    else if (std::strcmp(interface, wl_shm_interface.name) == 0)
    {
        gShm = static_cast<wl_shm*>(wl_registry_bind(reg, name, &wl_shm_interface, 1));
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0) 
    {
        gSeat = (wl_seat*)wl_registry_bind(reg, name, &wl_seat_interface, 5);
        wl_seat_add_listener(gSeat, &gSeatListener, nullptr);
    }
}

static void
RegistryGlobalRemove(void*,
                     wl_registry*,
                     uint32) 
{
}

static const wl_registry_listener gRegistryListener =
{
    .global = RegistryGlobal,
    .global_remove = RegistryGlobalRemove
};

namespace cgs
{
    static void
    Present(const Texture& backBuffer)
    {
        const uint32 width = backBuffer.GetWidth();
        const uint32 height = backBuffer.GetHeight();
        for (uint32 y = 0; y < height; y++)
        {
            byte* destinationRow = static_cast<byte*>(gShmBuffer.Data) + ((height - 1 - y) * static_cast<uint32_t>(gShmBuffer.Stride));
            for (uint32 x = 0; x < width; x++)
            {
                const Rgba8 fragment = backBuffer.GetFragment(x, y);
                const Rgba8 fragmentAfterPremultiplyingAlpha =
                {
                    .R = static_cast<byte>(fragment.R * fragment.A / 255),
                    .G = static_cast<byte>(fragment.G * fragment.A / 255),
                    .B = static_cast<byte>(fragment.B * fragment.A / 255),
                    .A = 255
                };
                destinationRow[4 * x + 0] = fragmentAfterPremultiplyingAlpha.B;
                destinationRow[4 * x + 1] = fragmentAfterPremultiplyingAlpha.G;
                destinationRow[4 * x + 2] = fragmentAfterPremultiplyingAlpha.R;
                destinationRow[4 * x + 3] = fragmentAfterPremultiplyingAlpha.A;
            }
        }

        wl_surface_attach(gSurface, gShmBuffer.Buffer, 0, 0);
        wl_surface_damage_buffer(gSurface, 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height));
        wl_surface_commit(gSurface);

        wl_display_flush(gDisplay); // send all queued requests
    }
}

int
main(int argc, char** argv)
{
    cgs::CommandLineParser commandLineParser(argc, argv);
    const bool isOptionFound = commandLineParser.ParseArguments();
    if (isOptionFound == false)
    {
        return 0;
    }

    // Connect to compositor
    gDisplay = wl_display_connect(nullptr);
    if (!gDisplay)
    {
        std::fprintf(stderr, "Failed to connect to Wayland display (WAYLAND_DISPLAY?)\n");
        return 1;
    }

    // Get global registry, discover needed interfaces
    gRegistry = wl_display_get_registry(gDisplay);
    wl_registry_add_listener(gRegistry, &gRegistryListener, nullptr);
    wl_display_roundtrip(gDisplay); // process globals

    if (gCompositor == nullptr || gWmBase == nullptr || gShm == nullptr)
    {
        std::fprintf(stderr, "Required globals missing: compositor=%p, wm_base=%p, shm=%p\n",
            (void*)gCompositor, (void*)gWmBase, (void*)gShm);
        return 2;
    }

    xdg_wm_base_add_listener(gWmBase, &gWmBaseListener, nullptr);

    // Create surface and xdg toplevel (your "window")
    gSurface = wl_compositor_create_surface(gCompositor);
    gXdgSurface = xdg_wm_base_get_xdg_surface(gWmBase, gSurface);
    xdg_surface_add_listener(gXdgSurface, &gXdgSurfaceListener, nullptr);

    gTopLevel = xdg_surface_get_toplevel(gXdgSurface);
    xdg_toplevel_add_listener(gTopLevel, &gTopLevelListener, nullptr);
    xdg_toplevel_set_title(gTopLevel, "Computer Graphics Studies");
    // Optional: set app_id for desktops that use it (e.g., task switchers)
    // xdg_toplevel_set_app_id(gTopLevel, "ComputerGraphicsStudies");

    // First commit; compositor will send configure, which we ack+commit in the callback
    wl_surface_commit(gSurface);
    wl_display_roundtrip(gDisplay);

    [[maybe_unused]] float deltaTimeInMs = 0.0f;
    std::vector<cgs::Geometry> cornellBox;
    cgs::CreateCornellBoxScene(cornellBox);

    cgs::gRenderThread.RenderInfoPerFrame.reserve(cgs::BACK_BUFFERS_COUNT);
    for(uint32 frameIndex = 0; frameIndex < cgs::BACK_BUFFERS_COUNT; frameIndex++)
    {
        cgs::RenderInfo renderInfo(cgs::gBackBuffers[frameIndex], cornellBox);
        cgs::gRenderThread.RenderInfoPerFrame.push_back(renderInfo);
    }

    // Event loop (like PeekMessage/DispatchMessage)
    uint32 frameIndex = 0;
    timespec startTime;
    clock_gettime(CLOCK_MONOTONIC_RAW, &startTime);
    while (gIsRunning)
    {
        // Pump pending events (non-blocking)
    // Drain any queued callbacks first
        wl_display_dispatch_pending(gDisplay);

        // Prepare to poll the Wayland fd for new events
        while (wl_display_prepare_read(gDisplay) != 0)
        {
            // Another thread (or previous loop) queued events; drain them
            wl_display_dispatch_pending(gDisplay);
        }
        wl_display_flush(gDisplay);

        pollfd pfd =
        {
            .fd = wl_display_get_fd(gDisplay),
            .events = POLLIN,
            .revents = 0
        };
        // If we’re waiting for the next vblank callback, we can block a bit
        const int timeoutInMs = gIsFrameReady ? 0 : 16;          // tune as you like
        const int ret = poll(&pfd, 1, timeoutInMs);

        if (ret > 0 && (pfd.revents & POLLIN))
        {
            wl_display_read_events(gDisplay);           // pulls new events
        }
        else
        {
            wl_display_cancel_read(gDisplay);           // nothing to read
        }
        wl_display_dispatch_pending(gDisplay);          // deliver to handlers
        if (!gIsRunning)
        {
            break;                         // handled Alt+F4 → close
        }

        // ---- Render only when compositor is ready (smooth pacing) ----
        // if (gIsFrameReady == false)
        // {
        //     continue;
        // }
        // gIsFrameReady = false;


        gFrameCallback = wl_surface_frame(gSurface);
        wl_callback_add_listener(gFrameCallback, &gFrameListener, nullptr);

        if(cgs::gRenderThread.CurrentThreadHandle == nullptr || cgs::IsThreadValid(*cgs::gRenderThread.CurrentThreadHandle) == false)
        {
            cgs::ThreadCreateInfo createInfo =
            {
                .Name = "RenderThread",
                .StackSize = 0,
                .Process = &cgs::Render,
                .Argument = &cgs::gRenderThread
            };
            const bool threadCreateResult = cgs::Create(cgs::gRenderThread.CurrentThreadHandle, createInfo);
            if(threadCreateResult == false)
            {
                assert(false && "Failed to create render thread");
            }
            frameIndex = (frameIndex + 1) % cgs::BACK_BUFFERS_COUNT;
        }
        else
        {
            cgs::RenderInfo::eRenderState currentFrameRenderState = cgs::gRenderThread.RenderInfoPerFrame[frameIndex].RenderState.load();
            if(currentFrameRenderState == cgs::RenderInfo::eRenderState::RENDERING)
            {
                while (currentFrameRenderState != cgs::RenderInfo::eRenderState::FINISHED)
                {
                    currentFrameRenderState = cgs::gRenderThread.RenderInfoPerFrame[frameIndex].RenderState.load();
                }
                cgs::Present(*cgs::gRenderThread.RenderInfoPerFrame[frameIndex].InoutBackBuffer);
                cgs::gRenderThread.RenderInfoPerFrame[frameIndex].RenderState.store(cgs::RenderInfo::eRenderState::IDLE);
                frameIndex = (frameIndex + 1) % cgs::BACK_BUFFERS_COUNT;
            }
            else if (currentFrameRenderState == cgs::RenderInfo::eRenderState::IDLE)
            {
                frameIndex = (frameIndex + 1) % cgs::BACK_BUFFERS_COUNT;
            }
        }

        timespec endTime;
        clock_gettime(CLOCK_MONOTONIC_RAW, &endTime);
        deltaTimeInMs = static_cast<float>(endTime.tv_sec - startTime.tv_sec) * 1000.0f + static_cast<float>(endTime.tv_nsec - startTime.tv_nsec) * 1e-6f;
        startTime = endTime;
    }

    if(cgs::IsThreadValid(*cgs::gRenderThread.CurrentThreadHandle))
    {
        cgs::Join(*cgs::gRenderThread.CurrentThreadHandle);
    }

    // Cleanup
    DestroyShmBuffer(gShmBuffer);
    if (gTopLevel)
    {
        xdg_toplevel_destroy(gTopLevel);
    }
    if (gXdgSurface)
    {
        xdg_surface_destroy(gXdgSurface);
    }
    if (gSurface)
    {
        wl_surface_destroy(gSurface);
    }
    if (gWmBase)
    {
        xdg_wm_base_destroy(gWmBase);
    }
    if (gCompositor)
    {
        wl_compositor_destroy(gCompositor);
    }
    if (gRegistry)
    {
        wl_registry_destroy(gRegistry);
    }
    if (gDisplay)
    {
        wl_display_disconnect(gDisplay);
    }
    return 0;
}

#endif // defined(CGS_LINUX)
