#pragma once

namespace cgs
{
    class Engine;

    namespace graphics
    {
        namespace rhi
        {
            class Instance; // Forward declaration of Instance class
        } // namespace rhi
    }

    class Launcher
    {
    public:
        static void MenuQuitCallback(Fl_Widget*, void*) noexcept;
        static void MenuOpenCallback(Fl_Widget*, void*) noexcept;
        static void MenuSaveAsCallback(Fl_Widget*, void*) noexcept;
        static void MenuSaveCallback(Fl_Widget*, void* pData) noexcept;
        static void OnButtonClick(Fl_Widget*, void* pData) noexcept;
        static void OnPhysicalDeviceGroupChange(Fl_Widget*, void* pData) noexcept;
        static void OnPhysicalDeviceChange(Fl_Widget*, void* pData) noexcept;
        static void OnRendererWindowClose(Fl_Widget*, void* pData) noexcept;

    public:
        Launcher() noexcept;
        ~Launcher() noexcept;

        void UpdateConfig() noexcept;
        int Run(const int32_t argc, char **argv) noexcept;

    private:
        void updateConfig(const core::Config& config, int& inoutYOffset) noexcept;
        void startEngine() noexcept;
        void stopEngine() noexcept;
    
    private:
        std::unique_ptr<Fl_Double_Window> mWindow;
        Fl_Menu_Bar* mAppMenuBar;
		Fl_Scroll* mScroll; // Scroll widget to hold configuration settings
		Fl_Choice* mPhysicalDeviceChoice; // Choice widget for physical device selection
        std::filesystem::path mConfigFilePath;
        std::unique_ptr<core::Config> mConfig;
        std::unique_ptr<core::Config> mRendererConfig;
        std::unique_ptr<graphics::rhi::Instance> mInstance; // Renderer Hardware Interface instance

        std::unique_ptr<Engine> mEngine;

        std::unique_ptr<Fl_Double_Window> mRendererWindow;
    };
} // namespace cgs
