#include "Launcher/pch.h"

#include "Launcher/Launcher.h"

#include "Core/Delegate.h"
#include "Core/Window.h"

#include "Graphics/pch.h"
#include "Graphics/Renderable.h"
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

    void Launcher::OnPhysicalDeviceGroupChange(Fl_Widget* widget, void* pData) noexcept
    {
        if (pData == nullptr)
        {
            CGS_LOG_ERROR("pData must be a valid pointer to Launcher instance.");
            return;
        }

        Launcher* launcher = static_cast<Launcher*>(pData);
        Fl_Choice* choice = static_cast<Fl_Choice*>(widget);
        if (choice == nullptr)
        {
            CGS_LOG_ERROR("Choice widget is null.");
            return;
        }

        const uint32_t selectedIndex = static_cast<uint32_t>(choice->value());
        const std::vector<std::unique_ptr<graphics::rhi::PhysicalDeviceGroup>>& physicalDeviceGroups = launcher->mInstance->GetPhysicalDeviceGroups(); // Create physical device groups
        graphics::rhi::PhysicalDeviceGroup& group = *physicalDeviceGroups[selectedIndex];
        const std::vector<std::unique_ptr<graphics::rhi::PhysicalDevice>>& physicalDevices = group.GetPhysicalDevices();
        launcher->mRendererConfig->SetSetting(CONFIG_PHYSICAL_DEVICE_GROUP_INDEX, selectedIndex);

        if (launcher->mPhysicalDeviceChoice != nullptr)
        {
            delete launcher->mPhysicalDeviceChoice;
            launcher->mPhysicalDeviceChoice = nullptr;
        }

        {
            launcher->mPhysicalDeviceChoice = new Fl_Choice(250, 500, 250, 30, "Physical Devices");
            launcher->mPhysicalDeviceChoice->callback(OnPhysicalDeviceChange, launcher); // Set the callback for the combo box
        }
        
        Fl_Choice& comboBox = *static_cast<Fl_Choice*>(launcher->mPhysicalDeviceChoice);
        comboBox.clear(); // Clear the existing items in the combo box
        for (const auto& physicalDevice : physicalDevices)
        {
            const char* deviceName = physicalDevice->GetProperties().PhysicalDeviceProperties.properties.deviceName;
            comboBox.add(deviceName);
            CGS_LOG_INFO("Adding physical device: %s", deviceName);
        }
        comboBox.value(0); // Select the first device by default
        OnPhysicalDeviceChange(&comboBox, launcher); // Call the change handler to update the renderer config

        launcher->mWindow->redraw(); // Redraw the window to show the updated combo box
    }

    void Launcher::OnPhysicalDeviceChange(Fl_Widget* widget, void* pData) noexcept
    {
        if (pData == nullptr)
        {
            CGS_LOG_ERROR("pData must be a valid pointer to Launcher instance.");
            return;
        }

        Launcher* launcher = static_cast<Launcher*>(pData);
        Fl_Choice* choice = static_cast<Fl_Choice*>(widget);
        if (choice == nullptr)
        {
            CGS_LOG_ERROR("Choice widget is null.");
            return;
        }

        uint32_t selectedPhysicalDeviceGroupIndex = 0;
        const bool result = launcher->mRendererConfig->GetSetting(CONFIG_PHYSICAL_DEVICE_GROUP_INDEX, selectedPhysicalDeviceGroupIndex);
        if (!result)
        {
            CGS_LOG_ERROR("Failed to get PhysicalDeviceGroupIndex from configuration.");
            return;
        }

        const std::vector<std::unique_ptr<graphics::rhi::PhysicalDeviceGroup>>& physicalDeviceGroups = launcher->mInstance->GetPhysicalDeviceGroups(); // Create physical device groups
        graphics::rhi::PhysicalDeviceGroup& group = *physicalDeviceGroups[selectedPhysicalDeviceGroupIndex];
        
        const uint32_t selectedPhysicalDeviceIndex = static_cast<uint32_t>(choice->value());
        const std::vector<std::unique_ptr<graphics::rhi::PhysicalDevice>>& physicalDevices = group.GetPhysicalDevices();
        launcher->mRendererConfig->SetSetting(CONFIG_PHYSICAL_DEVICE_INDEX, selectedPhysicalDeviceIndex);
        const std::string selectedDeviceName = physicalDevices[selectedPhysicalDeviceIndex]->GetProperties().PhysicalDeviceProperties.properties.deviceName;
        launcher->mRendererConfig->SetSetting(CONFIG_PHYSICAL_DEVICE, selectedDeviceName);
        launcher->mWindow->redraw(); // Redraw the window to show the updated combo box
    }

    static constexpr int SCROLL_X = 80;
    static constexpr int SCROLL_Y = 25;
    static constexpr int SCROLL_W = 640;
    static constexpr int SCROLL_H = 520;

    Launcher::Launcher() noexcept
        : mWindow(std::make_unique<Fl_Double_Window>(1600, 900, "CGS Launcher"))
        , mAppMenuBar(nullptr)
		, mScroll(nullptr) // Create a scroll widget to hold configuration settings
		, mPhysicalDeviceChoice(nullptr) // Initialize the physical device choice widget to nullptr
		, mPhysicalDeviceGroupChoice(nullptr) // Initialize the physical device group choice widget to nullptr
		, mLaunchButton(nullptr) // Initialize the launch button to nullptr
        , mConfigFilePath("config.ini") // Default path for the configuration file
        , mConfig(std::make_unique<cgs::core::Config>(core::Config::CreateInfo{.ConfigFilePath = mConfigFilePath}))
        , mRendererConfig()
        , mInstance()
        , mEngine()
        , mRendererWindow()
    {
        Fl::scheme("gtk+"); // Set the FLTK scheme to GTK+ for better appearance on Linux
        mWindow->begin();
        mAppMenuBar = new Fl_Menu_Bar(0, 0, mWindow->w(), 25);
        mAppMenuBar->add("File/Quit Editor", FL_COMMAND+'q', MenuQuitCallback);
        mWindow->callback(MenuQuitCallback);

        int ix = mAppMenuBar->find_index(MenuQuitCallback);
        mAppMenuBar->insert(ix, "Open", FL_COMMAND+'o', MenuOpenCallback, this, FL_MENU_DIVIDER);
        mAppMenuBar->insert(ix+1, "Save", FL_COMMAND+'s', MenuSaveCallback, this);
        mAppMenuBar->insert(ix+2, "Save as...", FL_COMMAND+'S', MenuSaveAsCallback, this, FL_MENU_DIVIDER);
        
        UpdateConfig(); // Initialize the configuration settings and widgets
        mWindow->end();
    }

    Launcher::~Launcher() noexcept
    {
        mEngine.reset(); // Reset the engine instance to release resources
		mRendererWindow.reset(); // Reset the renderer window to release resources

        mInstance.reset(); // Reset the RHI instance to release resources

        mRendererConfig.reset(); // Reset the renderer configuration object
        mConfig.reset(); // Reset the configuration object
        mWindow.reset();
    }

    void Launcher::UpdateConfig() noexcept
    {
        mWindow->clear(); // Clear the window before adding new widgets
		mWindow->begin(); // Begin the window layout

        // Remove any previous scroll if present
        if (mScroll)
        {
            mWindow->remove(mScroll);
            mScroll = nullptr; // Reset the scroll pointer
        }

        // Create a new scroll widget
        mScroll = new Fl_Scroll(SCROLL_X, SCROLL_Y, SCROLL_W, SCROLL_H);

        int yOffset = 0;
        updateConfig(*mConfig, yOffset); // Initialize the configuration settings and widgets

        std::filesystem::path rendererConfigFilePath;
        bool result = mConfig->GetSetting(CONFIG_RENDERER_CONFIG_FILE_PATH, rendererConfigFilePath);
        if (result == false)
        {
            CGS_LOG_INFO("Using default path 'Engine/config.ini'.");
            rendererConfigFilePath = "Engine/config.ini"; // Default path if not specified
            mConfig->SetSetting(CONFIG_RENDERER_CONFIG_FILE_PATH, rendererConfigFilePath.string());
        }
        else
        {
            CGS_LOG_INFO("Using renderer configuration file: %s", rendererConfigFilePath.string().c_str());
        }
        mRendererConfig = std::make_unique<core::Config>(core::Config::CreateInfo{.ConfigFilePath = rendererConfigFilePath});

        graphics::rhi::Instance::CreateInfo instanceCreateInfo
        {
            .Config = *mRendererConfig,
            .bCreateLogicalDevice = false,
        };
        mConfig->CreateProjectInfo(instanceCreateInfo.ApplicationInfo);
        mRendererConfig->CreateProjectInfo(instanceCreateInfo.EngineInfo);
        mInstance = std::make_unique<graphics::rhi::Instance>(instanceCreateInfo);

        updateConfig(*mRendererConfig, yOffset); // Initialize the configuration settings and widgets

        // Optionally, set the scroll's box type for better appearance
        mScroll->box(FL_DOWN_BOX);

        // Add the scroll to the window
        mScroll->end();
        mWindow->resizable(mScroll); // Make the scroll widget resizable

        const int scrollY = mScroll->yposition();
		const int scrollH = mScroll->h();

		int y = scrollY + scrollH + 30; // Initial vertical offset for widgets
        if (mLaunchButton != nullptr)
        {
            delete mLaunchButton;
            mLaunchButton = nullptr;
        }
        mLaunchButton = new Fl_Button(350, y, 120, 30, "Launch Engine");
        y += 100;
        mLaunchButton->callback(OnButtonClick, this); // Set the callback for the button
        mWindow->add(mLaunchButton);

        if (mPhysicalDeviceGroupChoice != nullptr)
        {
            delete mPhysicalDeviceGroupChoice;
            mPhysicalDeviceGroupChoice = nullptr;
        }
        mPhysicalDeviceGroupChoice = new Fl_Choice(250, y, 250, 30, "Physical Device Group");
        y += 50;
        const std::vector<std::unique_ptr<graphics::rhi::PhysicalDeviceGroup>>& physicalDeviceGroups = mInstance->GetPhysicalDeviceGroups(); // Create physical device groups
        for (size_t i = 0; i < physicalDeviceGroups.size(); ++i)
        {
            std::string label = "Group " + std::to_string(i);
            mPhysicalDeviceGroupChoice->add(label.c_str(), 0, nullptr, reinterpret_cast<void*>(i));
        }
        mPhysicalDeviceGroupChoice->callback(OnPhysicalDeviceGroupChange, this); // Set the callback for the combo box
        mPhysicalDeviceGroupChoice->value(0); // Select the first device by default
        OnPhysicalDeviceGroupChange(mPhysicalDeviceGroupChoice, this); // Call the change handler to update the renderer config
		mWindow->add(mPhysicalDeviceGroupChoice); // Add the combo box to the window

		std::string renderablesPathString;
		bool bResult = mConfig->GetSetting(CONFIG_RENDERABLES_PATH, renderablesPathString); // Retrieve the path to the renderers from the configuration
		if (!bResult || renderablesPathString.empty())
		{
			CGS_LOG_ERROR("Renderers path not found in configuration. Using default path.");
			renderablesPathString = "Assets/Renderables"; // Default path if not specified
		}

		std::filesystem::path renderablesPath(renderablesPathString);
		if (!std::filesystem::exists(renderablesPath))
		{
			CGS_LOG_ERROR("Renderables path '%s' does not exist. Please check the configuration.", renderablesPath.string().c_str());
			return; // Exit if the renderers path does not exist
		}

        std::vector<std::filesystem::path> modelPaths = graphics::Renderable::GetModelPaths(renderablesPath); // Get all model paths from the specified directory
        // Create a combo box (Fl_Choice) for model selection
        Fl_Choice* modelChoice = new Fl_Choice(250, y, 250, 30, "Model");
        for (const auto& path : modelPaths)
        {
            modelChoice->add(path.filename().string().c_str());
        }
        modelChoice->value(0); // Select the first model by default
        const std::string selectedModel = modelChoice->text();
        mRendererConfig->SetSetting(CONFIG_SELECTED_RENDERABLE, selectedModel); // Store the selected model in the configuration

        modelChoice->callback([](Fl_Widget* widget, void* pData)
        {
            if (pData == nullptr)
            {
                CGS_LOG_ERROR("pData must be a valid pointer to Launcher instance.");
                return;
            }

            Launcher* launcher = static_cast<Launcher*>(pData);
            Fl_Choice* choice = static_cast<Fl_Choice*>(widget);
            if (choice == nullptr)
            {
                CGS_LOG_ERROR("Choice widget is null.");
                return;
            }

            const std::string selectedModel = choice->text();
            launcher->mRendererConfig->SetSetting(CONFIG_SELECTED_RENDERABLE, selectedModel); // Store the selected model in the configuration
        }, this); // Set the callback for the combo box
        mWindow->add(modelChoice);
        y += 50;

		mWindow->end(); // End the window layout

        uint32_t width = 1920; // Default width for the renderer window
        uint32_t height = 1080; // Default height
        result = mConfig->GetSetting(CONFIG_WIDTH, width);
        if (!result)
        {
            CGS_LOG_INFO("Using default width: %u", width);
            mConfig->SetSetting(CONFIG_WIDTH, width);
        }
        result = mConfig->GetSetting(CONFIG_HEIGHT, height);
        if (!result)
        {
            CGS_LOG_INFO("Using default height: %u", height);
            mConfig->SetSetting(CONFIG_HEIGHT, height);
        }

        mRendererWindow = std::make_unique<core::Window>(
            core::Window::CreateInfo
            {
                .CurrentProjectInfo = instanceCreateInfo.ApplicationInfo,
                .Width = width,
                .Height = height,
            }
            );
    }

    void Launcher::updateConfig(const core::Config& config, int& inoutYOffset) noexcept
    {
        const std::unordered_map<std::string, bool>& boolSettings = config.GetBoolSettings();
        const std::unordered_map<std::string, uint32_t>& intSettings = config.GetIntSettings();
        const std::unordered_map<std::string, std::string>& stringSettings = config.GetStringSettings();
        const std::unordered_map<std::string, float>& floatSettings = config.GetFloatSettings();

        const uint32_t xOffset = 100 + SCROLL_X; // Initial horizontal offset for widgets
		static constexpr const uint32_t Y_OFFSET = 50; // Vertical offset for widgets
        inoutYOffset += 30 + SCROLL_Y; // Initial vertical offset for widgets
        const uint32_t widgetWidth = 100; // Width of each widget
        const uint32_t widgetHeight = 25; // Height of each widget
        // booleans
        for (const auto& [key, value] : boolSettings)
        {
            CGS_LOG_INFO("Boolean setting: %s = %s", key.c_str(), value ? "true" : "false");
            Fl_Round_Button* button = new Fl_Round_Button(xOffset, inoutYOffset, widgetWidth, widgetHeight, key.c_str());
            inoutYOffset += Y_OFFSET; // Increment vertical offset for next widget
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
        }

        // integers
        for (const auto& [key, value] : intSettings)
        {
            CGS_LOG_INFO("Integer setting: %s = %u", key.c_str(), value);
            Fl_Value_Input* input = new Fl_Value_Input(xOffset, inoutYOffset, widgetWidth, widgetHeight, key.c_str());
            inoutYOffset += Y_OFFSET; // Increment vertical offset for next widget
            input->type(FL_INT_INPUT); // Set the input type to integer
            input->value(static_cast<double>(value)); // Set the initial value
            input->align(FL_ALIGN_TOP); // Set alignment for input widgets
        }

        // strings
        for (const auto& [key, value] : stringSettings)
        {
            CGS_LOG_INFO("String setting: %s = %s", key.c_str(), value.c_str());
            Fl_Input* input = new Fl_Input(xOffset, inoutYOffset, widgetWidth, widgetHeight, key.c_str());
            inoutYOffset += Y_OFFSET; // Increment vertical offset for next widget
            input->value(value.c_str()); // Set the initial value
            input->align(FL_ALIGN_TOP); // Set alignment for input widgets
        }

        // floats
        for (const auto& [key, value] : floatSettings)
        {
            CGS_LOG_INFO("Float setting: %s = %f", key.c_str(), value);
            Fl_Value_Input* input = new Fl_Value_Input(xOffset, inoutYOffset, widgetWidth, widgetHeight, key.c_str());
            inoutYOffset += Y_OFFSET; // Increment vertical offset for next widget
            input->type(FL_FLOAT_INPUT); // Set the input type to float
            input->value(static_cast<double>(value)); // Set the initial value
            input->align(FL_ALIGN_TOP); // Set alignment for input widgets
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
            core::Config rendererConfig = *mRendererConfig;
            
            Engine::CreateInfo engineCreateInfo
            {
                .EngineConfig = std::move(config),
                .RendererConfig = std::move(rendererConfig),
                .Window = *mRendererWindow,
            };

			mRendererConfig.reset(); // Reset the renderer configuration to release resources
			mConfig.reset(); // Reset the configuration to release resources
            mInstance.reset(); // Reset the instance to ensure it's created fresh
			mWindow.reset(); // Reset the window to release resources

            mEngine = std::make_unique<cgs::Engine>(engineCreateInfo);

            mEngine->Run(); // Start the engine's main loop
            mEngine.reset(); // Release the engine resources
            mRendererWindow.reset(); // Reset the renderer window to release resources
            UpdateConfig(); // Reinitialize the configuration settings and widgets
        }
    }
} // namespace cgs
