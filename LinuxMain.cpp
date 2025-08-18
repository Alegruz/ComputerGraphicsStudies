#if defined(CGS_LINUX)

#include "pch.hpp"

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

// Input (for popups & clicks)
static wl_seat*        gSeat       = nullptr;
static wl_pointer*     gPointer    = nullptr;
static uint32_t        gLastButtonSerial = 0;
static int             gPointerX   = 0;
static int             gPointerY   = 0;

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
                uint32_t colorARGB) noexcept
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
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            px[y * (shmBuffer.Stride / 4) + x] = colorARGB;
        }
    }
    return true;
}

// =================== Simple Menubar + Popup ===================
// We draw a top bar and create a dropdown popup using xdg_popup.
static constexpr int MENU_BAR_H = 28;
static constexpr int FILE_BTN_W = 64; // clickable "File" area (we don't render text here)

// "Commands" like on Win32:
enum : uint32_t 
{
    IDM_FILE_OPEN         = 1001,
    IDM_FILE_OPEN_RECENT  = 1002
};

// Popup state
struct Popup 
{
    wl_surface*     Surface = nullptr;
    xdg_surface*    XSurface = nullptr;
    xdg_popup*      Popup = nullptr;
    ShmBuffer       Buffer;
    int             Width = 160;
    int             Height = 2 * 24;   // two items
    int             HotIndex = -1;
    bool            IsVisible = false;
} gFilePopup;

// recent list (labels only; hook up real MRU later)
static std::vector<std::string> gRecent = {};

static void 
PaintMainSurface() noexcept
{
    if (gBackBuffer.Data == nullptr) 
    {
        return;
    }
    uint32_t* px = static_cast<uint32_t*>(gBackBuffer.Data);
    const int width = gBackBuffer.Width;
    const int height = gBackBuffer.Height;
    const int pitch = gBackBuffer.Stride / 4;

    // background (teal)
    const uint32_t bg = 0xFF20AAAA;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            px[y * pitch + x] = bg;
        }
    }

    // menubar (dark)
    static constexpr uint32_t bar = 0xFF202024;
    for (int y = 0; y < std::min(MENU_BAR_H, height); ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            px[y * pitch + x] = bar;
        }
    }

    // "File" button region (slightly lighter when hovered)
    const bool hot = (gPointerY >= 0 && gPointerY < MENU_BAR_H && gPointerX >= 8 && gPointerX < (8 + FILE_BTN_W));
    const uint32_t btn = hot ? 0xFF3A3A44 : 0xFF2A2A34;
    for (int y = 4; y < MENU_BAR_H-4; ++y)
    {
        for (int x = 8; x < 8 + FILE_BTN_W; ++x)
        {
            if (y < height && x < width) 
            {
                px[y * pitch + x] = btn;
            }
        }
    }
}

static void 
CommitMainSurface() noexcept
{
    wl_surface_attach(gSurface, gBackBuffer.Buffer, 0, 0);
    wl_surface_damage_buffer(gSurface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(gSurface);
}

static void 
DestroyPopup(Popup& popup) noexcept
{
    if (popup.Popup)
    {
        xdg_popup_destroy(popup.Popup);
    }
    if (popup.XSurface)
    {
        xdg_surface_destroy(popup.XSurface);
    }
    if (popup.Surface)
    {
        wl_surface_destroy(popup.Surface);
    }
    DestroyShmBuffer(popup.Buffer);
    popup = Popup{};
}

static void 
FillRow(ShmBuffer& inoutBuffer, 
        const int row, 
        const uint32_t color) noexcept
{
    uint32_t* px = static_cast<uint32_t*>(inoutBuffer.Data);
    const int pitch = inoutBuffer.Stride / 4;
    const int y0 = row * 24, y1 = y0 + 24;
    for (int y = y0; y < y1; ++y)
    {
        for (int x = 0; x < inoutBuffer.Width; ++x)
        {
            px[y * pitch + x] = color;
        }
    }
}

static void 
PaintFilePopup(Popup& popup) noexcept
{
    // simple 2-row menu: [0] Open..., [1] Open Recent
    static constexpr uint32_t bg = 0xFFF0F0F0;
    static constexpr uint32_t hi = 0xFFE0E6F8;
    const int width = popup.Width;
    const int height = popup.Height;
    CreateShmBuffer(popup.Buffer, width, height, bg);

    if (popup.HotIndex == 0) 
    {
        FillRow(popup.Buffer, 0, hi);
    }
    if (popup.HotIndex == 1) 
    {
        FillRow(popup.Buffer, 1, hi);
    }

    wl_surface_attach(popup.Surface, popup.Buffer, 0, 0);
    wl_surface_damage_buffer(popup.Surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(popup.Surface);
}

static void 
HandleMenuCommand(uint32_t id) noexcept
{
    if (id == IDM_FILE_OPEN) 
    {
        // TODO: implement file dialog via xdg-desktop-portal (DBus).
        std::fprintf(stderr, "[Wayland] File -> Open... (TODO: portal)\n");
    } 
    else if (id == IDM_FILE_OPEN_RECENT) 
    {
        std::fprintf(stderr, "[Wayland] File -> Open Recent (items=%zu)\n", gRecent.size());
        // TODO: create a second popup to the right listing gRecent
    }
}

static void 
ShowFileMenuPopup(const int anchorX, 
                  const int anchorY) noexcept
{
    DestroyPopup(gFilePopup);

    // create child surface for popup
    gFilePopup.Surface  = wl_compositor_create_surface(gCompositor);
    gFilePopup.XSurface = xdg_wm_base_get_xdg_surface(gWmBase, gFilePopup.Surface);

    // positioner: anchor under the File button rect
    xdg_positioner* pos = xdg_wm_base_create_positioner(gWmBase);
    xdg_positioner_set_size(pos, gFilePopup.Width, gFilePopup.Height);
    xdg_positioner_set_anchor_rect(pos, anchorX, anchorY, FILE_BTN_W, MENU_BAR_H);
    xdg_positioner_set_anchor(pos, XDG_POSITIONER_ANCHOR_BOTTOM_LEFT);
    xdg_positioner_set_gravity(pos, XDG_POSITIONER_GRAVITY_BOTTOM_LEFT);
    xdg_positioner_set_offset(pos, 0, 0);

    gFilePopup.Popup = xdg_surface_get_popup(
        gFilePopup.XSurface,
        gXdgSurface, // parent is the toplevel's xdg_surface
        pos);
    xdg_positioner_destroy(pos);

    // popup listeners
    static void (*noop)() = nullptr;
    auto popup_configure = [](void*, xdg_popup*, int32_t, int32_t, int32_t, int32_t) {};
    auto popup_done = [](void*, xdg_popup*) { DestroyPopup(gFilePopup); };
    static const xdg_popup_listener popLis = {
        /*configure*/ popup_configure,
        /*popup_done*/ popup_done,
        /*repositioned*/ nullptr
    };
    xdg_popup_add_listener(gFilePopup.popup, &popLis, nullptr);

    // xdg_surface for popup must ack configure
    auto xdg_popup_surface_configure = [](void*, xdg_surface* s, uint32_t serial) {
        xdg_surface_ack_configure(s, serial);
        PaintFilePopup(gFilePopup);
    };
    static const xdg_surface_listener xsLis = { xdg_popup_surface_configure };
    xdg_surface_add_listener(gFilePopup.XSurface, &xsLis, nullptr);

    // Take an implicit grab so outside clicks dismiss the popup:
    if (gSeat && gLastButtonSerial != 0) {
        xdg_popup_grab(gFilePopup.Popup, gSeat, gLastButtonSerial);
    }

    gFilePopup.IsVisible = true;
    wl_surface_commit(gFilePopup.Surface); // will trigger configure -> paint
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

    PaintMainSurface();
    CommitMainSurface();
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

// Seat/pointer
static void 
PointerEnter(void*, 
             wl_pointer*, 
             uint32_t serial, 
             wl_surface*, 
             wl_fixed_t sx, 
             wl_fixed_t sy) 
{
    gLastButtonSerial = serial;
    gPointerX = wl_fixed_to_int(sx);
    gPointerY = wl_fixed_to_int(sy);
    PaintMainSurface(); CommitMainSurface();
}

static void 
PointerLeave(void*, 
             wl_pointer*, 
             uint32_t serial, 
             wl_surface*)
{
    gLastButtonSerial = serial;
    gPointerX = gPointerY = -1;
    PaintMainSurface(); CommitMainSurface();
}

static void 
PointerMotion(void*, 
              wl_pointer*, 
              uint32_t /*time*/, 
              wl_fixed_t sx, 
              wl_fixed_t sy)
{
    gPointerX = wl_fixed_to_int(sx);
    gPointerY = wl_fixed_to_int(sy);
    // update popup hover if visible
    if (gFilePopup.IsVisible)
    {
        // popup relative coords: just re-evaluate from global pointer under the anchor
        // here we do nothing fancy; hover is recomputed when we paint after button press
    }
    PaintMainSurface(); CommitMainSurface();
}

static void 
PointerButton(void*, 
              wl_pointer*, 
              uint32_t serial, 
              uint32_t /*time*/, 
              uint32_t button, 
              uint32_t state)
{
    gLastButtonSerial = serial;
    const bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
    const int  x = gPointerX, y = gPointerY;

    // Click on "File" area -> open popup
    if (pressed && y >= 0 && y < MENU_BAR_H && x >= 8 && x < (8 + FILE_BTN_W))
    {
        ShowFileMenuPopup(/*anchorX*/8, /*anchorY*/MENU_BAR_H);
        return;
    }

    // If popup is visible, handle selection on release
    if (!pressed && gFilePopup.IsVisible)
    {
        // Rough hit-test in popup local coords: we don't track absolute popup pos;
        // since it's anchored directly under the File button, use that.
        const int px = x - 8;       // same anchor as ShowFileMenuPopup()
        const int py = y - MENU_BAR_H;
        if (px >= 0 && px < gFilePopup.Width && py >= 0 && py < gFilePopup.Height)
        {
            const int idx = py / 24;
            if (idx == 0) 
            {
                HandleMenuCommand(IDM_FILE_OPEN);
            }
            if (idx == 1) 
            {
                HandleMenuCommand(IDM_FILE_OPEN_RECENT);
            }
        }
        DestroyPopup(gFilePopup);
    }
}

static void
PointerAxis(void*, 
            wl_pointer*, 
            uint32_t, 
            uint32_t, 
            wl_fixed_t)
{}

static void 
PointerAxisSource(void*, 
                  wl_pointer*, 
                  uint32_t)
{}
static void 
PointerAxisStop(void*, 
                wl_pointer*, 
                uint32_t, 
                uint32_t)
{}
static void 
PointerAxisDiscrete(void*, 
                    wl_pointer*, 
                    uint32_t, 
                    int32_t)
{}
static void 
PointerAxisValue120(void*, 
                   wl_pointer*, 
                   uint32_t, 
                   int32_t)
{}
static void 
PointerFrame(void*, 
             wl_pointer*)
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
    .axis_value120 = PointerAxisValue120
};

static void 
SeatCapabilities(void*, 
                 wl_seat* seat, 
                 uint32_t caps) 
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
    else if (strcmp(interface, wl_seat_interface.name) == 0) 
    {
        gSeat = (wl_seat*)wl_registry_bind(reg, name, &wl_seat_interface, 5);
        wl_seat_add_listener(gSeat, &gSeatListener, nullptr);
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
    DestroyPopup(gFilePopup);
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
