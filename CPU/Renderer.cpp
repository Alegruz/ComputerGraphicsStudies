#include "pch.hpp"

#include "Common/Renderer.cpp"

#if defined(CGS_GRAPHICS_API_CPU)
#include "CPU/Renderer.hpp"
#include "Common/Thread.h"

#include <iostream>
#include <random>

namespace cgs
{
    static GlobalRenderContext gGlobalRenderContext;
    static std::vector<Texture> gBackBuffers(BACK_BUFFERS_COUNT, Texture(Texture::CreateInfo{ .Format = RenderResource::eFormat::BGRA8_UNORM, .Width = cgs::gWidth, .Height = cgs::gHeight, .Depth = 1, .Name = std::string("Back Buffer") }));
    static std::vector<Texture> gDepthBuffers(BACK_BUFFERS_COUNT, Texture(Texture::CreateInfo{ .Format = RenderResource::eFormat::D32_UNORM, .Width = cgs::gWidth, .Height = cgs::gHeight, .Depth = 1, .Name = std::string("Depth Buffer") }));

    std::vector<SubRenderThreadInfo> gSubRenderThreads;
    static std::mutex gBackBufferLock;
    static std::mutex gDepthBufferLock;

    static void
    SubRenderThreadMain(ThreadProcessArgument& arg) noexcept;

    static void
    AddQuadVertices(VertexBuffer& vertexBuffer, std::vector<uint16>& inoutIndices, const Coordinate<eCoordinateSpace::WORLD>& v0, const Coordinate<eCoordinateSpace::WORLD>& v1, const Coordinate<eCoordinateSpace::WORLD>& v2, const Coordinate<eCoordinateSpace::WORLD>& v3)
    {
        const Coordinate<eCoordinateSpace::WORLD> normal = Normalize(Cross(v1 - v0, v2 - v0));
        bool vertexAddResult = vertexBuffer.AddVertex(VertexPN{ v0, normal });
        vertexAddResult &= vertexBuffer.AddVertex(VertexPN{ v1, normal });
        vertexAddResult &= vertexBuffer.AddVertex(VertexPN{ v2, normal });
        vertexAddResult &= vertexBuffer.AddVertex(VertexPN{ v3, normal });
        if (vertexAddResult)
        {
            inoutIndices.push_back(static_cast<uint16>(vertexBuffer.GetVertexCount() - 4));
            inoutIndices.push_back(static_cast<uint16>(vertexBuffer.GetVertexCount() - 3));
            inoutIndices.push_back(static_cast<uint16>(vertexBuffer.GetVertexCount() - 2));
            inoutIndices.push_back(static_cast<uint16>(vertexBuffer.GetVertexCount() - 4));
            inoutIndices.push_back(static_cast<uint16>(vertexBuffer.GetVertexCount() - 2));
            inoutIndices.push_back(static_cast<uint16>(vertexBuffer.GetVertexCount() - 1));
        }
        else
        {
            assert(false && "Failed to add vertices");
        }
    }

    bool
    CreateCornellBoxScene(const eRenderMethod, std::vector<std::unique_ptr<Geometry>>& outGeometries) noexcept
    {
        [[maybe_unused]] Camera& mainCamera = InitializeCornellBoxCamera();

        outGeometries.clear();
        outGeometries.reserve(8);

        // Floor
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Floor")));
        Geometry& floor = *outGeometries.back();
        {
            VertexBuffer floorVertexBuffer(
                RenderResource::CreateInfo
                {
                    .Formats = std::vector<RenderResource::eFormat>{ RenderResource::eFormat::RGB32_FLOAT, RenderResource::eFormat::RGB32_FLOAT },
                    .DataOrEmpty = std::vector<byte>{},
                    .Name = std::string("Floor Vertex Buffer"),
                }
                );
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 0.0f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -552.8f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -549.6f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 0.0f, 559.2f };
            std::vector<uint16> floorIndices;
            floorIndices.reserve(6);
            AddQuadVertices(floorVertexBuffer, floorIndices, v0, v1, v2, v3);
            floor.SetVertexBuffer(std::move(floorVertexBuffer));
            floor.SetIndices(std::move(floorIndices));
            floor.SetColor(WHITE);
        }

        // Light
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Light")));
        Geometry& light = *outGeometries.back();
        {
            VertexBuffer lightVertexBuffer(
                RenderResource::CreateInfo
                {
                    .Formats = std::vector<RenderResource::eFormat>{ RenderResource::eFormat::RGB32_FLOAT, RenderResource::eFormat::RGB32_FLOAT },
                    .DataOrEmpty = std::vector<byte>{},
                    .Name = std::string("Light Vertex Buffer"),
                }
                );
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -343.0f, 548.8f, 332.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -343.0f, 548.8f, 227.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -213.0f, 548.8f, 227.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { -213.0f, 548.8f, 332.0f };
            std::vector<uint16> lightIndices;
            lightIndices.reserve(6);
            AddQuadVertices(lightVertexBuffer, lightIndices, v0, v1, v2, v3);
            light.SetVertexBuffer(std::move(lightVertexBuffer));
            light.SetIndices(std::move(lightIndices));
            light.SetColor(WHITE);
            light.SetIsEmissive(true);
        }

        // Ceiling
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Ceiling")));
        Geometry& ceiling = *outGeometries.back();
        {
            VertexBuffer ceilingVertexBuffer(
                RenderResource::CreateInfo
                {
                    .Formats = std::vector<RenderResource::eFormat>{ RenderResource::eFormat::RGB32_FLOAT, RenderResource::eFormat::RGB32_FLOAT },
                    .DataOrEmpty = std::vector<byte>{},
                    .Name = std::string("Ceiling Vertex Buffer"),
                }
            );

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -556.0f, 548.8f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -556.0f, 548.8f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 0.0f, 548.8f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 548.8f, 559.2f };
            std::vector<uint16> ceilingIndices;
            ceilingIndices.reserve(6);
            AddQuadVertices(ceilingVertexBuffer, ceilingIndices, v0, v1, v2, v3);
            ceiling.SetVertexBuffer(std::move(ceilingVertexBuffer));
            ceiling.SetIndices(std::move(ceilingIndices));
            ceiling.SetColor(WHITE);
        }
        
        // Back wall
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Back Wall")));
        Geometry& backWall = *outGeometries.back();
        {
            VertexBuffer backWallVertexBuffer(
                RenderResource::CreateInfo
                {
                    .Formats = std::vector<RenderResource::eFormat>{ RenderResource::eFormat::RGB32_FLOAT, RenderResource::eFormat::RGB32_FLOAT },
                    .DataOrEmpty = std::vector<byte>{},
                    .Name = std::string("Back Wall Vertex Buffer"),
                }
            );

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 0.0f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -549.6f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -556.0f, 548.8f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 548.8f, 559.2f };
            std::vector<uint16> backWallIndices;
            backWallIndices.reserve(6);
            AddQuadVertices(backWallVertexBuffer, backWallIndices, v0, v1, v2, v3);
            backWall.SetVertexBuffer(std::move(backWallVertexBuffer));
            backWall.SetIndices(std::move(backWallIndices));
            backWall.SetColor(WHITE);
        }

        // Right wall
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Right Wall")));
        Geometry& rightWall = *outGeometries.back();
        {
            VertexBuffer rightWallVertexBuffer(
                RenderResource::CreateInfo
                {
                    .Formats = std::vector<RenderResource::eFormat>{ RenderResource::eFormat::RGB32_FLOAT, RenderResource::eFormat::RGB32_FLOAT },
                    .DataOrEmpty = std::vector<byte>{},
                    .Name = std::string("Right Wall Vertex Buffer"),
                }
                );
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 0.0f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { 0.0f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 0.0f, 548.8f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 548.8f, 0.0f };
            std::vector<uint16> rightWallIndices;
            rightWallIndices.reserve(6);
            AddQuadVertices(rightWallVertexBuffer, rightWallIndices, v0, v1, v2, v3);
            rightWall.SetVertexBuffer(std::move(rightWallVertexBuffer));
            rightWall.SetIndices(std::move(rightWallIndices));
            rightWall.SetColor(GREEN);
        }

        // Left wall
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Left Wall")));
        Geometry& leftWall = *outGeometries.back();
        {
            VertexBuffer leftWallVertexBuffer(
                RenderResource::CreateInfo
                {
                    .Formats = std::vector<RenderResource::eFormat>{ RenderResource::eFormat::RGB32_FLOAT, RenderResource::eFormat::RGB32_FLOAT },
                    .DataOrEmpty = std::vector<byte>{},
                    .Name = std::string("Left Wall Vertex Buffer"),
                }
            );

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -549.6f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -552.8f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -556.0f, 548.8f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { -556.0f, 548.8f, 559.2f };
            std::vector<uint16> leftWallIndices;
            leftWallIndices.reserve(6);
            AddQuadVertices(leftWallVertexBuffer, leftWallIndices, v0, v1, v2, v3);
            leftWall.SetVertexBuffer(std::move(leftWallVertexBuffer));
            leftWall.SetIndices(std::move(leftWallIndices));
            leftWall.SetColor(RED);
        }

        // Short block
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Short Block")));
        Geometry& shortBlock = *outGeometries.back();
        {
            VertexBuffer shortBlockVertexBuffer(
                RenderResource::CreateInfo
                {
                    .Formats = std::vector<RenderResource::eFormat>{ RenderResource::eFormat::RGB32_FLOAT, RenderResource::eFormat::RGB32_FLOAT },
                    .DataOrEmpty = std::vector<byte>{},
                    .Name = std::string("Short Block Vertex Buffer"),
                }
            );

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -82.0f, 165.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -130.0f, 165.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -290.0f, 165.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { -240.0f, 165.0f, 272.0f };

            std::vector<uint16> shortBlockIndices;
            shortBlockIndices.reserve(6 * 5);
            AddQuadVertices(shortBlockVertexBuffer, shortBlockIndices, v0, v1, v2, v3);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v4 = { -290.0f, 165.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v5 = { -290.0f, 0.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v6 = { -240.0f, 0.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v7 = { -240.0f, 165.0f, 272.0f };
            AddQuadVertices(shortBlockVertexBuffer, shortBlockIndices, v4, v5, v6, v7);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v8 = { -130.0f, 165.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v9 = { -130.0f, 0.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v10 = { -290.0f, 0.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v11 = { -290.0f, 165.0f, 114.0f };
            AddQuadVertices(shortBlockVertexBuffer, shortBlockIndices, v8, v9, v10, v11);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v12 = { -82.0f, 165.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v13 = { -82.0f, 0.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v14 = { -130.0f, 0.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v15 = { -130.0f, 165.0f, 65.0f };
            AddQuadVertices(shortBlockVertexBuffer, shortBlockIndices, v12, v13, v14, v15);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v16 = { -240.0f, 165.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v17 = { -240.0f, 0.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v18 = { -82.0f, 0.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v19 = { -82.0f, 165.0f, 225.0f };
            AddQuadVertices(shortBlockVertexBuffer, shortBlockIndices, v16, v17, v18, v19);

            shortBlock.SetVertexBuffer(std::move(shortBlockVertexBuffer));
            shortBlock.SetIndices(std::move(shortBlockIndices));
            shortBlock.SetColor(WHITE);
        }

        // Tall block
        outGeometries.push_back(std::make_unique<Geometry>(std::string("Tall Block")));
        Geometry& tallBlock = *outGeometries.back();
        {
            VertexBuffer tallBlockVertexBuffer(
                RenderResource::CreateInfo
                {
                    .Formats = std::vector<RenderResource::eFormat>{ RenderResource::eFormat::RGB32_FLOAT, RenderResource::eFormat::RGB32_FLOAT },
                    .DataOrEmpty = std::vector<byte>{},
                    .Name = std::string("Tall Block Vertex Buffer"),
                }
            );

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { -265.0f, 330.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { -423.0f, 330.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { -472.0f, 330.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { -314.0f, 330.0f, 456.0f };

            std::vector<uint16> tallBlockIndices;
            tallBlockIndices.reserve(6 * 5);
            AddQuadVertices(tallBlockVertexBuffer, tallBlockIndices, v0, v1, v2, v3);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v4 = { -423.0f, 330.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v5 = { -423.0f, 0.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v6 = { -472.0f, 0.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v7 = { -472.0f, 330.0f, 406.0f };
            AddQuadVertices(tallBlockVertexBuffer, tallBlockIndices, v4, v5, v6, v7);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v8 = { -472.0f, 330.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v9 = { -472.0f, 0.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v10 = { -314.0f, 0.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v11 = { -314.0f, 330.0f, 456.0f };
            AddQuadVertices(tallBlockVertexBuffer, tallBlockIndices, v8, v9, v10, v11);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v12 = { -314.0f, 330.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v13 = { -314.0f, 0.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v14 = { -265.0f, 0.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v15 = { -265.0f, 330.0f, 296.0f };
            AddQuadVertices(tallBlockVertexBuffer, tallBlockIndices, v12, v13, v14, v15);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v16 = { -265.0f, 330.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v17 = { -265.0f, 0.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v18 = { -423.0f, 0.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v19 = { -423.0f, 330.0f, 247.0f };
            AddQuadVertices(tallBlockVertexBuffer, tallBlockIndices, v16, v17, v18, v19);

            tallBlock.SetVertexBuffer(std::move(tallBlockVertexBuffer));
            tallBlock.SetIndices(std::move(tallBlockIndices));
            tallBlock.SetColor(WHITE);
        }

        return true;
    }

    CornellBoxVertexShaderOutput
    CornellBoxVertexShader(const VertexPN& input) noexcept
    {
        const HomogenousCoordinate<eCoordinateSpace::WORLD> wsInput(input.Position, 1.0f);
        const HomogenousCoordinate<eCoordinateSpace::VIEW> vsInput = gMainCamera.GetBuffer().ViewMatrix * wsInput;
        const HomogenousCoordinate<eCoordinateSpace::PERSPECTIVE> psInput = gMainCamera.GetBuffer().ProjectionMatrix * vsInput;
        const Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE> ndcInput = psInput.GetXYZ() / psInput.W;
        return { ndcInput, wsInput.GetXYZ(), input.Normal };
    }

    Rgba8
    CornellBoxFragmentShader(const CornellBoxFragmentShaderInput& input) noexcept
    {
        // Simple Lambertian
        // Sample a random point from the emissive geometry
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> distribution(0.0f, 1.0f);

        Rgba8 outputColor = input.Color;
        float3 radiance;
        float4 emissiveBarycentricCoordinates;
        float sum = 0.0f;
        emissiveBarycentricCoordinates.X = static_cast<float>(distribution(gen));
        sum += emissiveBarycentricCoordinates.X;
        emissiveBarycentricCoordinates.Y = static_cast<float>(distribution(gen)) * (1.0f - sum);
        sum += emissiveBarycentricCoordinates.Y;
        emissiveBarycentricCoordinates.Z = static_cast<float>(distribution(gen)) * (1.0f - sum);
        sum += emissiveBarycentricCoordinates.Z;
        emissiveBarycentricCoordinates.W = 1.0f - sum;
        const VertexPN* emissiveVertex0OrNull = nullptr;
        const VertexPN* emissiveVertex1OrNull = nullptr;
        const VertexPN* emissiveVertex2OrNull = nullptr;
        const VertexPN* emissiveVertex3OrNull = nullptr;
        input.EmissiveGeometry.GetVertexBuffer().GetVertexOrNull(emissiveVertex0OrNull, 0);
        if(emissiveVertex0OrNull != nullptr)
        {
            const VertexPN& emissiveVertex0 = *emissiveVertex0OrNull;
            input.EmissiveGeometry.GetVertexBuffer().GetVertexOrNull(emissiveVertex1OrNull, 1);
            if(emissiveVertex1OrNull != nullptr)
            {
                const VertexPN& emissiveVertex1 = *emissiveVertex1OrNull;
                input.EmissiveGeometry.GetVertexBuffer().GetVertexOrNull(emissiveVertex2OrNull, 2);
                if(emissiveVertex2OrNull != nullptr)
                {
                    const VertexPN& emissiveVertex2 = *emissiveVertex2OrNull;
                    input.EmissiveGeometry.GetVertexBuffer().GetVertexOrNull(emissiveVertex3OrNull, 3);
                    if(emissiveVertex3OrNull != nullptr)
                    {
                        const VertexPN& emissiveVertex3 = *emissiveVertex3OrNull;
                        // All four vertices are available, we can sample the emissive color
                        const Coordinate<eCoordinateSpace::WORLD> emissivePoint =
                            (emissiveVertex0.Position * emissiveBarycentricCoordinates[0]) +
                            (emissiveVertex1.Position * emissiveBarycentricCoordinates[1]) +
                            (emissiveVertex2.Position * emissiveBarycentricCoordinates[2]) +
                            (emissiveVertex3.Position * emissiveBarycentricCoordinates[3]);

                        const Direction<eCoordinateSpace::WORLD> emissiveDirection = Normalize(emissivePoint - input.VSOutput.WsPosition);
                        const Rgba8& emissiveGeometryColor = input.EmissiveGeometry.GetColor();
                        const float4 emissiveColor = float4(
                            static_cast<float>(emissiveGeometryColor.R) / 255.0f,
                            static_cast<float>(emissiveGeometryColor.G) / 255.0f,
                            static_cast<float>(emissiveGeometryColor.B) / 255.0f,
                            static_cast<float>(emissiveGeometryColor.A) / 255.0f
                        );
                        constexpr float intensityOfIncomingLight = 1.0f;
                        const float3 lambertian = Dot(emissiveDirection, input.VSOutput.Normal) * emissiveColor.GetXYZ() * intensityOfIncomingLight;
                        radiance = lambertian;
                    }
                }
            }
        }

        outputColor = Rgba8
        {
            .R = static_cast<uint8>(radiance.X * static_cast<float>(outputColor.R)),
            .G = static_cast<uint8>(radiance.Y * static_cast<float>(outputColor.G)),
            .B = static_cast<uint8>(radiance.Z * static_cast<float>(outputColor.B)),
            .A = outputColor.A,
        };

        return outputColor;
    }
    
    bool
    InitializeRenderer(const RendererCreateInfo&) noexcept
    {
        for (uint32 i = 0; i < BACK_BUFFERS_COUNT; ++i)
        {
            Texture& texture = gBackBuffers[i];
            texture.SetName("Back Buffer " + std::to_string(i));

            Texture& depthTexture = gDepthBuffers[i];
            depthTexture.SetName("Depth Buffer " + std::to_string(i));
        }

        gGlobalRenderContext.RenderDeviceType = eRenderDeviceType::CPU;
        return true;
    }

    void
    Render(const float deltaTime, uint64& inoutWorkIndex, RenderThreadInfo& inoutRenderThreadInfo, const std::vector<std::unique_ptr<Geometry>>& geometries) noexcept
    {
        const uint32 currentFrameIndexToRender = static_cast<uint32>(inoutWorkIndex % 3);
        bool isFirstFrame = false;
        while (true)
        {
            const uint64 lastCompleteWorkIndex = inoutRenderThreadInfo.LastCompleteWorkIndex.load();
            isFirstFrame = inoutWorkIndex < cgs::BACK_BUFFERS_COUNT;
            const bool hasCompletedWork = lastCompleteWorkIndex != std::numeric_limits<uint64>::max();

            if (isFirstFrame == true || (hasCompletedWork && lastCompleteWorkIndex >= static_cast<uint64>(static_cast<int64>(inoutWorkIndex) - static_cast<int64>(cgs::BACK_BUFFERS_COUNT))))
            {
                break;
            }
            cgs::Yield();
        }

        {
            if (isFirstFrame == false)
            {
                gPresentFunc(cgs::gBackBuffers[currentFrameIndexToRender]);
            }

            const std::lock_guard lock(inoutRenderThreadInfo.RenderWorksMutex);
            inoutRenderThreadInfo.RenderWorksPerFrame.push(
                cgs::RenderWork
                {
                    .Geometries = geometries,
                    .WorkIndex = inoutWorkIndex++,
                    .FrameIndex = currentFrameIndexToRender,
                    .DeltaTime = deltaTime,
                });
        }
    }

    void
    RenderThreadMain(ThreadProcessArgument& arg) noexcept
    {
        if (arg.Argument == nullptr)
        {
            assert(false && "RenderInfo argument is null");
            return;
        }

        if (gSubRenderThreads.empty() == true)
        {
            const uint32 availableProcessorsCount = GetLogicalProcessorsCount() - 2;  // Leave 2 cores free
            gSubRenderThreads.reserve(static_cast<size_t>(availableProcessorsCount));

            for (uint32 i = 0; i < availableProcessorsCount; ++i)
            {
                gSubRenderThreads.emplace_back();
                SubRenderThreadInfo& threadInfo = gSubRenderThreads.back();
                ThreadCreateInfo threadCreateInfo =
                {
                    .Name = "SubRenderThread",
                    .StackSize = 0,
                    .Process = &SubRenderThreadMain,
                    .Argument = &threadInfo,
                };
                bool threadCreateResult = Create(threadInfo.CurrentThreadHandle, threadCreateInfo);
                if (threadCreateResult == false)
                {
                    assert(false && "Failed to create rasterization thread");
                }
            }
        }

        RenderThreadInfo& renderThreadInfo = *static_cast<RenderThreadInfo*>(arg.Argument);
        while (renderThreadInfo.IsActive.load())
        {
            std::unique_lock<std::mutex> uniqueLock(renderThreadInfo.RenderWorksMutex, std::defer_lock);
            uniqueLock.lock();
            if (renderThreadInfo.RenderWorksPerFrame.empty() == false)
            {
                RenderWork renderWork = std::move(renderThreadInfo.RenderWorksPerFrame.front());
                renderThreadInfo.RenderWorksPerFrame.pop();
                uniqueLock.unlock();

                // Process the render work
                CpuRenderWork cpuRenderWork = 
                {
                    .OutTexture = gBackBuffers[renderWork.FrameIndex],
                    .OutDepthBuffer = gDepthBuffers[renderWork.FrameIndex],
                    .Work = renderWork,
                };
                cpuRenderWork.OutTexture.Clear();
                cpuRenderWork.OutDepthBuffer.Clear(std::numeric_limits<float>::max());
                if (renderThreadInfo.RenderMethod == eRenderMethod::RASTERIZATION)
                {
                    renderThreadInfo.CurrentWorkIndex.store(renderWork.WorkIndex);
                    Rasterize(cpuRenderWork);
                    renderThreadInfo.LastCompleteWorkIndex.store(renderWork.WorkIndex);
                    // std::cout << "RenderWork completed: " << renderWork.WorkIndex << std::endl;
                }
                else
                {
                    assert(false && "Unsupported render method in RenderThreadMain");
                }
            }
            else
            {
                uniqueLock.unlock();
            }
        }

        for(SubRenderThreadInfo& threadInfo : gSubRenderThreads)
        {
            threadInfo.IsActive.store(false);
            Join(*threadInfo.CurrentThreadHandle);
            threadInfo.CurrentThreadHandle = nullptr;
        }
    }

    void
    SubRenderThreadMain(ThreadProcessArgument& arg) noexcept
    {
        if (arg.Argument == nullptr)
        {
            assert(false && "SubRenderThreadInfo argument is null");
            return;
        }

        SubRenderThreadInfo& threadInfo = *static_cast<SubRenderThreadInfo*>(arg.Argument);
        while (threadInfo.IsActive.load())
        {
            std::unique_lock<std::mutex> uniqueLock(threadInfo.RenderWorksMutex, std::defer_lock);
            uniqueLock.lock();
            if (threadInfo.SubRenderWorks.empty() == false)
            {
                SubRenderWork renderWork = std::move(threadInfo.SubRenderWorks.front());
                threadInfo.SubRenderWorks.pop();
                uniqueLock.unlock();

                // Process the render work
                if (threadInfo.RenderMethod == eRenderMethod::RASTERIZATION)
                {
                    SubRasterize(renderWork);
                    threadInfo.LastCompleteWorkIndex = renderWork.WorkIndex;
                    // std::cout << "SubRenderWork completed: " << renderWork.WorkIndex << std::endl;
                }
                else
                {
                    assert(false && "Unsupported render method in SubRenderThreadMain");
                }
            }
            else
            {
                uniqueLock.unlock();
            }
        }
    }

    void
    SubRasterize(SubRenderWork& work) noexcept
    {
        const uint32 width = work.ParentRenderWork.OutTexture.GetWidth();
        const uint32 height = work.ParentRenderWork.OutTexture.GetHeight();

        for (uint32 y = work.MinY; y < work.MaxY; ++y)
        {
            for (uint32 x = work.MinX; x < work.MaxX; ++x)
            {
                Coordinate<eCoordinateSpace::NORMALIZED_DEVICE_COORDINATE> point{
                    static_cast<float>(x) / static_cast<float>(width) * 2.0f - 1.0f,
                    static_cast<float>(y) / static_cast<float>(height) * 2.0f - 1.0f,
                    0.0f
                };
                const float3 barycentricCoords = ComputeBarycentricCoordinates(work.V0.NdcPosition, work.V1.NdcPosition, work.V2.NdcPosition, point);
                const bool isInTriangle = 0.0f <= barycentricCoords.X && barycentricCoords.X <= 1.0f &&
                    0.0f <= barycentricCoords.Y && barycentricCoords.Y <= 1.0f &&
                    0.0f <= barycentricCoords.Z && barycentricCoords.Z <= 1.0f;
                if (isInTriangle)
                {
#if 1
                    point.Z = work.V0.NdcPosition.Z * barycentricCoords.X + work.V1.NdcPosition.Z * barycentricCoords.Y + work.V2.NdcPosition.Z * barycentricCoords.Z;
                    float depthValue;
                    work.ParentRenderWork.OutDepthBuffer.GetFragment(depthValue, RenderResource::eFormat::D32_UNORM, x, y);
                    if (point.Z >= depthValue)
                    {
                        continue;
                    }
                    else
                    {
                        std::lock_guard<std::mutex> lock(gDepthBufferLock);
                        work.ParentRenderWork.OutDepthBuffer.SetFragmentValue(x, y, point.Z, RenderResource::eFormat::D32_UNORM);
                    }
                    // Simple rasterization logic: set every pixel to a color
                    // In a real application, you would perform actual rasterization here
                    Rgba8 fragmentValue = work.CurrentGeometry.GetColor();
                    if(work.CurrentGeometry.IsEmissive() == false)
                    {
                        CornellBoxFragmentShaderInput fsInput =
                        {
                            .VSOutput =
                            {
                                .NdcPosition = point,
                                .WsPosition = work.V0.WsPosition * barycentricCoords.X + work.V1.WsPosition * barycentricCoords.Y + work.V2.WsPosition * barycentricCoords.Z,
                                .Normal = work.V0.Normal * barycentricCoords.X + work.V1.Normal * barycentricCoords.Y + work.V2.Normal * barycentricCoords.Z,
                            },
                            .Color = work.CurrentGeometry.GetColor(),
                            .EmissiveGeometry = work.EmissiveGeometry,
                        };
                        fragmentValue = CornellBoxFragmentShader(fsInput);
                    }
#else
                    const Rgba8 fragmentValue = RED;
#endif                    

                    std::lock_guard<std::mutex> lock(gBackBufferLock);
                    work.ParentRenderWork.OutTexture.SetFragmentValue(x, y, fragmentValue, RenderResource::eFormat::RGBA8_UNORM);
                }
            }
        }
    }
}
#endif  // defined(CGS_GRAPHICS_API_CPU)