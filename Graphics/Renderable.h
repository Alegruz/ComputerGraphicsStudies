#pragma once

namespace cgs::graphics
{
    class Renderable final
    {
    public:
        struct CreateInfo final
        {
            std::filesystem::path ModelPath; // Path to the model file
			const VertexLayoutsMap&    VertexLayouts; // Map of vertex layouts for the renderable
        };

    public:
        static std::unique_ptr<Renderable> CreateOrNull(const CreateInfo& createInfo) noexcept;
        static std::vector<std::filesystem::path> GetModelPaths(const std::filesystem::path& renderablesPath) noexcept;

    public:
        Renderable() = default;
        Renderable(const Renderable&) = delete; // Disable copy constructor
        Renderable(Renderable&&) noexcept = default; // Enable move constructor
        Renderable& operator=(const Renderable&) = delete; // Disable copy assignment
        Renderable& operator=(Renderable&&) noexcept = default; // Enable move assignment
        ~Renderable() noexcept = default; // Default destructor

        const std::string& GetName() const noexcept { return mName; } // Get the name of the renderable

    private:
        std::string mName; // Name of the renderable
    };
} // namespace cgs::graphics
