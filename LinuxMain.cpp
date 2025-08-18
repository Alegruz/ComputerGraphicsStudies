// WaylandMain.cpp
#if defined(CGS_LINUX) // build this file only for your Linux/Wayland target

#include <wayland-client.h>

#include <climits>   // INT32_MAX
#include <cstdio>
#include <cstring>
#include <cstdint>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>


// Generated from wayland-protocols' xdg-shell.xml (see CMake notes below)
#include "xdg-shell-client-protocol.h"

static bool gIsRunning = true;

// Wayland objects we need
static wl_display* gDisplay = nullptr;
static wl_registry* gRegistry = nullptr;
static wl_compositor* gCompositor = nullptr;
static xdg_wm_base* gWmBase = nullptr;
static wl_surface* gSurface = nullptr;
static xdg_surface* gXdgSurface = nullptr;
static xdg_toplevel* gTopLevel = nullptr;
static wl_shm* gShm = nullptr;   // <-- SHM

// Pending (suggested) size from toplevel.configure
static int32_t gPendingWidth = 0;
static int32_t gPendingHeight = 0;

// ---- Very small SHM helper ----
struct ShmBuffer {
    wl_buffer*  Buffer = nullptr;
    void*       Data = nullptr;
    int         Width = 0;
    int         Height = 0;
    int         Stride = 0;
    size_t      Size = 0;
};
static ShmBuffer gBackBuffer;

static int
CreateShmFile(size_t size)
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
DestroyShmBuffer(ShmBuffer& shmBuffer)
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
                int height)
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
    uint32_t* px = static_cast<uint32_t*>(shmBuffer.Data);
    const uint32_t color = 0xFF20AAAA; // ARGB
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            px[y * (shmBuffer.Stride / 4) + x] = color;
        }
    }
    return true;
}

// ----- xdg_wm_base (ping/pong) -----
static void
XdgWmBasePing(void*,
              xdg_wm_base* wm,
              uint32_t serial)
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
                    uint32_t serial)
{
    // Must ack every configure
    xdg_surface_ack_configure(surf, serial);

    // Determine a size to draw. Compositor may suggest 0,0; pick a default.
    const int width = (gPendingWidth > 0) ? gPendingWidth : 800;
    const int height = (gPendingHeight > 0) ? gPendingHeight : 600;

    if (!gBackBuffer.Buffer || gBackBuffer.Width != width || gBackBuffer.Height != height)
    {
        if (!CreateShmBuffer(gBackBuffer, width, height))
        {
            std::fprintf(stderr, "Failed to create shm buffer\n");
            return;
        }
    }

    // Attach + damage + commit -> this maps the surface (makes the window visible)
    wl_surface_attach(gSurface, gBackBuffer.Buffer, 0, 0);
    wl_surface_damage_buffer(gSurface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(gSurface);
}

static const xdg_surface_listener gXdgSurfaceListener =
{
    .configure = XdgSurfaceConfigure
};

// ----- xdg_toplevel -----
static void
XdgToplevelConfigure(void*,
                     xdg_toplevel*,
                     int32_t width,
                     int32_t height,
                     wl_array* /*states*/)
{
    gPendingWidth = width;
    gPendingHeight = height;
}

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
                          int32_t,
                          int32_t)
{
}

static void
XdgToplevelWmCapabilities(void*,
                          xdg_toplevel*,
                          wl_array*)
{
}

static const xdg_toplevel_listener gTopLevelListener =
{
    .configure = XdgToplevelConfigure,
    .close = XdgToplevelClose,
    .configure_bounds = XdgToplevelConfigureBound,
    .wm_capabilities = XdgToplevelWmCapabilities
};

// ----- wl_registry: discover globals -----
static void
RegistryGlobal(void*,
                wl_registry* reg,
                uint32_t name,
                const char* interface,
                [[maybe_unused]] uint32_t version)
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
}

static void
RegistryGlobalRemove(void*,
                     wl_registry*,
                     uint32_t) 
{
}

static const wl_registry_listener gRegistryListener =
{
    .global = RegistryGlobal,
    .global_remove = RegistryGlobalRemove
};

int
main(void)
{
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

    // Event loop (like PeekMessage/DispatchMessage)
    while (gIsRunning && wl_display_dispatch(gDisplay) != -1)
    {
        // no-op; callbacks above do the work
    }

    // Cleanup
    DestroyShmBuffer(gBackBuffer);
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
