#include "Graphics/pch.h"

#include "Graphics/Renderable.h"

namespace cgs::graphics
{
    std::unique_ptr<Renderable> Renderable::CreateOrNull(const CreateInfo& createInfo) noexcept
    {
        if (createInfo.ModelPath.empty())
        {
            CGS_LOG_ERROR("Model path is empty. Cannot create Renderable.");
            return nullptr; // Return null if the model path is empty
        }

        rapidobj::Result result = rapidobj::ParseFile(createInfo.ModelPath); // Parse the model file to extract necessary data
        if (result.error)
        {
            CGS_LOG_ERROR("Failed to parse model file '%s': %s", createInfo.ModelPath.string().c_str(), result.error.code.message().c_str());
            return nullptr; // Return null if parsing fails
        }

        rapidobj::Triangulate(result);
        if (result.error)
        {
            CGS_LOG_ERROR("Failed to triangulate model file '%s': %s", createInfo.ModelPath.string().c_str(), result.error.code.message().c_str());
            return nullptr; // Return null if parsing fails
        }

        auto renderable = std::make_unique<Renderable>();
        return renderable; // Return the created Renderable instance
    }

    std::vector<std::filesystem::path> Renderable::GetModelPaths(const std::filesystem::path& renderablesPath) noexcept
    {
        std::vector<std::filesystem::path> modelPaths;
		std::queue<std::filesystem::path> dirs;
		dirs.push(renderablesPath);
		while (!dirs.empty())
		{
			auto currentDir = dirs.front();
			dirs.pop();

			for (const auto& entry : std::filesystem::directory_iterator(currentDir))
			{
				if (entry.is_directory())
				{
					dirs.push(entry.path());
				}
				else if (entry.is_regular_file())
				{
					if (entry.path().extension() == ".obj")
					{
                        modelPaths.push_back(entry.path()); // Add the model file path to the list if it has a .obj extension
					}
				}
			}
		}

        return modelPaths; // Return the list of model file paths
    }
} // namespace cgs::graphics