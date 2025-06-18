#include "Launcher/pch.h"

#include "Launcher/Launcher.h"

#include "Graphics/pch.h"
#include "Graphics/RHI/Device.h"
#include "Graphics/RHI/Instance.h"
#include "Graphics/RHI/PhysicalDevice.h"
#include "Graphics/RHI/PhysicalDeviceGroup.h"

#include "Engine/Engine.h"

namespace cgs
{
    void Launcher::MenuQuitCallback(Fl_Widget*, void*) noexcept
    {
        const int choice = fl_choice("Do you want to quit the Launcher?", "Quit", "Cancel", NULL);
        if (choice == 1) 
        {
            return;
        }

        Fl::hide_all_windows();
    }

    void Launcher::MenuOpenCallback(Fl_Widget*, void* pData) noexcept
    {
        if (pData == nullptr)
        {
            CGS_LOG_ERROR("pData must be a valid pointer to Launcher instance.");
            return;
        }
        
        Launcher* launcher = static_cast<Launcher*>(pData);

        Fl_Native_File_Chooser fileChooser;
        fileChooser.title("Open File");
        fileChooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
        fileChooser.filter("*.ini\tConfig Files\n");

        if (fileChooser.show() == 0) 
        {
            // Handle file opening logic here
            CGS_LOG_INFO("Selected file: %s", fileChooser.filename());
            launcher->mConfigFilePath = fileChooser.filename();
            launcher->mConfig = std::make_unique<cgs::core::Config>(cgs::core::Config::CreateInfo{.ConfigFilePath = launcher->mConfigFilePath});
            CGS_LOG_INFO("Configuration file loaded: %s", launcher->mConfigFilePath.string().c_str());

            launcher->UpdateConfig(); // Initialize the configuration settings and widgets
        }
    }

    void Launcher::MenuSaveAsCallback(Fl_Widget*, void* pData) noexcept
    {
        if(pData != nullptr)
        {
            Fl_Native_File_Chooser file_chooser;
            file_chooser.title("Save File As...");
            file_chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);    
        }
        else
        {
            Fl_Native_File_Chooser file_chooser;
            file_chooser.title("Save File As...");
            file_chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
            if (file_chooser.show() == 0)
            {
                CGS_LOG_INFO(file_chooser.filename());
            }
        }
    }

    void Launcher::MenuSaveCallback(Fl_Widget*, void* pData) noexcept
    {
        if(pData == nullptr)
        {
            CGS_LOG_ERROR("No file to save.");
            return;
        }

        Launcher* launcher = static_cast<Launcher*>(pData);
        std::string filePath = launcher->mConfigFilePath.string();
        MenuSaveAsCallback(NULL, reinterpret_cast<void*>(filePath.data()));
    }

    void Launcher::OnButtonClick(Fl_Widget*, void* pData) noexcept
    {
        if (pData == nullptr)
        {
            CGS_LOG_ERROR("pData must be a valid pointer to Launcher instance.");
            return;
        }

        Launcher* launcher = static_cast<Launcher*>(pData);
        launcher->startEngine();
    }

    Launcher::Launcher() noexcept
        : mWindow(std::make_unique<Fl_Double_Window>(800, 600, "CGS Launcher"))
        , mAppMenuBar()
        , mConfigFilePath("config.ini") // Default path for the configuration file
        , mConfig(std::make_unique<cgs::core::Config>(core::Config::CreateInfo{.ConfigFilePath = mConfigFilePath}))
    {
        Fl::scheme("gtk+"); // Set the FLTK scheme to GTK+ for better appearance on Linux
        mWindow->label("CGS Launcher"); // Set the window title
        mWindow->begin();
        mAppMenuBar = std::make_unique<Fl_Menu_Bar>(0, 0, mWindow->w(), 25);
        mAppMenuBar->add("File/Quit Editor", FL_COMMAND+'q', MenuQuitCallback);
        mWindow->callback(MenuQuitCallback);

        int ix = mAppMenuBar->find_index(MenuQuitCallback);
        mAppMenuBar->insert(ix, "Open", FL_COMMAND+'o', MenuOpenCallback, this, FL_MENU_DIVIDER);
        mAppMenuBar->insert(ix+1, "Save", FL_COMMAND+'s', MenuSaveCallback, this);
        mAppMenuBar->insert(ix+2, "Save as...", FL_COMMAND+'S', MenuSaveAsCallback, this, FL_MENU_DIVIDER);
        
        UpdateConfig(); // Initialize the configuration settings and widgets

        auto launchButton = std::make_unique<Fl_Button>(350, 550, 120, 30, "Launch Engine");
        launchButton->callback(OnButtonClick, this); // Set the callback for the button
        mWindow->add(launchButton.get());
        mWidgets.push_back(std::move(launchButton));

        const std::vector<std::unique_ptr<graphics::rhi::PhysicalDeviceGroup>>& physicalDeviceGroups = mInstance->GetPhysicalDeviceGroups(); // Create physical device groups
        for (const auto& group : physicalDeviceGroups)
        {
            const std::vector<std::unique_ptr<graphics::rhi::PhysicalDevice>>& physicalDevices = group->GetPhysicalDevices();
            for (const auto& physicalDevice : physicalDevices)
            {
                const char* deviceName = physicalDevice->GetProperties().PhysicalDeviceProperties.properties.deviceName;
                auto comboBox = std::make_unique<Fl_Choice>(100, 400, 250, 30, "Physical Device");
                comboBox->add(deviceName);
                comboBox->value(0); // Select the first device by default
                mWidgets.push_back(std::move(comboBox));
            }
        }

        mWindow->end();
    }

    Launcher::~Launcher() noexcept
    {
        mAppMenuBar.reset();
        mWidgets.clear(); // Clear the widgets vector to release resources
        mConfig.reset(); // Reset the configuration object
        mWindow.reset();
    }

    void Launcher::UpdateConfig() noexcept
    {
        updateConfig(*mConfig); // Initialize the configuration settings and widgets

        std::filesystem::path rendererConfigFilePath;
        bool result = mConfig->GetSetting("RendererConfigFilePath", rendererConfigFilePath);
        if (result == false)
        {
            CGS_LOG_INFO("Using default path 'Engine/config.ini'.");
            rendererConfigFilePath = "Engine/config.ini"; // Default path if not specified
            mConfig->SetSetting("RendererConfigFilePath", rendererConfigFilePath.string());
        }
        else
        {
            CGS_LOG_INFO("Using renderer configuration file: %s", rendererConfigFilePath.string().c_str());
        }
        mRendererConfig = std::make_unique<core::Config>(core::Config::CreateInfo{.ConfigFilePath = rendererConfigFilePath});

        graphics::rhi::Instance::CreateInfo instanceCreateInfo
        {
            .Config = *mRendererConfig,
        };
        mConfig->CreateProjectInfo(instanceCreateInfo.ApplicationInfo);
        mRendererConfig->CreateProjectInfo(instanceCreateInfo.EngineInfo);
        mInstance = std::make_unique<graphics::rhi::Instance>(instanceCreateInfo);
    }

    void Launcher::updateConfig(const core::Config& config) noexcept
    {
        const std::unordered_map<std::string, bool> boolSettings = config.GetBoolSettings();
        const std::unordered_map<std::string, uint32_t> intSettings = config.GetIntSettings();
        const std::unordered_map<std::string, std::string> stringSettings = config.GetStringSettings();
        const std::unordered_map<std::string, float> floatSettings = config.GetFloatSettings();
        const uint32_t numSettings = static_cast<uint32_t>(boolSettings.size() + intSettings.size() + stringSettings.size() + floatSettings.size());
        mWindow->clear(); // Clear the window before adding new widgets
        mWidgets.reserve(numSettings);

        const uint32_t xOffset = 100; // Initial horizontal offset for widgets
        uint32_t yOffset = 30; // Initial vertical offset for widgets
        const uint32_t widgetWidth = 100; // Width of each widget
        const uint32_t widgetHeight = 25; // Height of each widget
        // booleans
        for (const auto& [key, value] : boolSettings)
        {
            CGS_LOG_INFO("Boolean setting: %s = %s", key.c_str(), value ? "true" : "false");
            std::unique_ptr<Fl_Round_Button> button = std::make_unique<Fl_Round_Button>(xOffset, yOffset, widgetWidth, widgetHeight, key.c_str());
            yOffset += 30; // Increment vertical offset for next widget
            button->type(FL_RADIO_BUTTON);
            if(value)
            {
                button->set(); // Set the button to checked if the value is true
            }
            else
            {
                button->clear(); // Clear the button if the value is false
            }
            button->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE); // Set alignment for buttons
            mWidgets.push_back(std::move(button));
        }

        // integers
        for (const auto& [key, value] : intSettings)
        {
            CGS_LOG_INFO("Integer setting: %s = %u", key.c_str(), value);
            std::unique_ptr<Fl_Value_Input> input = std::make_unique<Fl_Value_Input>(xOffset, yOffset, widgetWidth, widgetHeight, key.c_str());
            yOffset += 30; // Increment vertical offset for next widget
            input->type(FL_INT_INPUT); // Set the input type to integer
            input->value(static_cast<double>(value)); // Set the initial value
            input->align(FL_ALIGN_TOP); // Set alignment for input widgets
            mWidgets.push_back(std::move(input));
        }

        // strings
        for (const auto& [key, value] : stringSettings)
        {
            CGS_LOG_INFO("String setting: %s = %s", key.c_str(), value.c_str());
            std::unique_ptr<Fl_Input> input = std::make_unique<Fl_Input>(xOffset, yOffset, widgetWidth, widgetHeight, key.c_str());
            yOffset += 30; // Increment vertical offset for next widget
            input->value(value.c_str()); // Set the initial value
            input->align(FL_ALIGN_TOP); // Set alignment for input widgets
            mWidgets.push_back(std::move(input));
        }

        // floats
        for (const auto& [key, value] : floatSettings)
        {
            CGS_LOG_INFO("Float setting: %s = %f", key.c_str(), value);
            CGS_LOG_INFO("Integer setting: %s = %u", key.c_str(), value);
            std::unique_ptr<Fl_Value_Input> input = std::make_unique<Fl_Value_Input>(xOffset, yOffset, widgetWidth, widgetHeight, key.c_str());
            yOffset += 30; // Increment vertical offset for next widget
            input->type(FL_FLOAT_INPUT); // Set the input type to float
            input->value(static_cast<double>(value)); // Set the initial value
            input->align(FL_ALIGN_TOP); // Set alignment for input widgets
            mWidgets.push_back(std::move(input));
        }

        for (const auto& widget : mWidgets)
        {
            mWindow->add(widget.get()); // Add each widget to the window
        }
        mWindow->redraw(); // Redraw the window to show the new widgets
    }

    int Launcher::Run(const int32_t argc, char **argv) noexcept
    {
        mWindow->show(argc, argv);
        return Fl::run();
    }

    void Launcher::startEngine() noexcept
    {
        if (!mEngine)
        {
            core::Config config = *mConfig;
            mEngine = std::make_unique<cgs::Engine>(std::move(config));
        }
    }

    void Launcher::stopEngine() noexcept
    {
        if (mEngine)
        {
            mEngine.reset(); // Release the engine resources
        }
    }
} // namespace cgs
