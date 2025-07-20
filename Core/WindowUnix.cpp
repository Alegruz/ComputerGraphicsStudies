#include "Core/Window.h"

#if defined(CGS_UNIX)
#include <iostream>

namespace cgs::core
{
    Window::Window([[maybe_unused]] const CreateInfo& createInfo) noexcept
    {
        CGS_LOG_ERROR("Window::Window() is not implemented for Unix platforms.");
        // Initialize the window with the provided create info
        // This is a placeholder implementation, as actual window creation would require platform-specific code
    }

    void Window::Show() const noexcept
    {
        CGS_LOG_ERROR("Window::Show() is not implemented for Unix platforms.");
    }
}
#endif // defined(CGS_UNIX)