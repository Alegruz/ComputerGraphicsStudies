#pragma once

class Fl_Double_Window;

namespace cgs
{
    class Launcher
    {
    public:
        static void MenuQuitCallback(Fl_Widget*, void*) noexcept;
        static void MenuOpenCallback(Fl_Widget*, void*) noexcept;
        static void MenuSaveAsCallback(Fl_Widget*, void*) noexcept;
        static void MenuSaveCallback(Fl_Widget*, void* pData) noexcept;

    public:
        Launcher();
        ~Launcher();

        void UpdateConfig();
        int Run(const int32_t argc, char **argv);
    
    private:
        std::unique_ptr<Fl_Double_Window> mWindow;
        std::unique_ptr<Fl_Menu_Bar> mAppMenuBar;
        std::filesystem::path mConfigFilePath;
        std::unique_ptr<cgs::core::Config> mConfig;

        std::vector<std::unique_ptr<Fl_Widget>> mWidgets; // Store widgets to manage their lifetime
    };
} // namespace cgs
