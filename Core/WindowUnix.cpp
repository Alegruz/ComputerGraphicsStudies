#include "Core/Window.h"

#if defined(CGS_UNIX)
namespace cgs::core
{
    void Window::RegistryGlobalHandler([[maybe_unused]] void *data, [[maybe_unused]] wl_registry *registry, [[maybe_unused]] uint32_t name, [[maybe_unused]] const char *interface, [[maybe_unused]] uint32_t version) noexcept
    {
        CGS_LOG_INFO("RegistryGlobalHandler called with name: {}, interface: {}, version: {}", name, interface, version);
        // Handle global objects here, e.g., create surfaces or other Wayland objects
        Window* window = static_cast<Window*>(data);
        if (strcmp(interface, "wl_compositor") == 0)
        {
            window->mCompositor = static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
            window->mSurface = wl_compositor_create_surface(window->mCompositor);
            CGS_LOG_INFO("Compositor bound successfully.");
        }
    }

    void Window::RegistryGlobalRemoveHandler([[maybe_unused]] void* data, [[maybe_unused]] wl_registry* registry, [[maybe_unused]] uint32_t name) noexcept
    {
        CGS_LOG_INFO("RegistryGlobalRemoveHandler called with name: {}", name);
        // Handle removal of global objects here
    }

    const wl_registry_listener Window::smRegistryListener = 
    {
        .global = Window::RegistryGlobalHandler,
        .global_remove = Window::RegistryGlobalRemoveHandler
    };

    Window::Window([[maybe_unused]] const CreateInfo& createInfo) noexcept
        : mDisplay(wl_display_connect(nullptr)) // Connect to the Wayland display
        , mRegistry(nullptr)
    {
        if (!mDisplay)
        {
            CGS_LOG_ERROR("Failed to connect to Wayland display.");
            return;
        }
        CGS_LOG_INFO("Connected to Wayland display successfully.");

        mRegistry = wl_display_get_registry(mDisplay);
        if (!mRegistry)
        {
            CGS_LOG_ERROR("Failed to get Wayland registry.");
            return;
        }
        CGS_LOG_INFO("Wayland registry obtained successfully.");

        wl_registry_add_listener(mRegistry, &smRegistryListener, this);
        wl_display_roundtrip(mDisplay); // Process the Wayland events to ensure the registry is populated
    }

    Window::~Window() noexcept
    {
        if (mDisplay)
        {
            wl_display_disconnect(mDisplay);
            mDisplay = nullptr;
        }
    }

    void Window::Show() const noexcept
    {
        CGS_LOG_ERROR("Window::Show() is not implemented for Unix platforms.");
    }
}
#endif // defined(CGS_UNIX)