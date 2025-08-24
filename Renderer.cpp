#include "pch.hpp"
#include "Renderer.hpp"
#include "Renderer.h"

#include <numbers>
#include <random>

namespace cgs
{
    static Camera gMainCamera;
    static float4x4 gViewMatrix;
    static float4x4 gProjectionMatrix;

    static constexpr void
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

    void
    CreateCornellBoxScene(std::vector<Geometry>& outGeometries) noexcept
    {
        gMainCamera = Camera(
            Camera::CreateInfo
            {
                .Position = Coordinate<eCoordinateSpace::WORLD>{ 278.0f, 273.0f, -800.0f },
                .Front = Coordinate<eCoordinateSpace::WORLD>{ 0.0f, 0.0f, 1.0f },
                .Up = Coordinate<eCoordinateSpace::WORLD>{ 0.0f, 1.0f, 0.0f },
            }
            );
        
        float3 zAxis = Normalize(gMainCamera.GetFront());
        float3 xAxis = Normalize(Cross(gMainCamera.GetUp(), zAxis));
        float3 yAxis = Cross(zAxis, xAxis);

        gViewMatrix = float4x4
        {
            .Data =
            {
                float4( xAxis.X, xAxis.Y, xAxis.Z, -Dot(xAxis, gMainCamera.GetPosition()) ),
                float4( yAxis.X, yAxis.Y, yAxis.Z, -Dot(yAxis, gMainCamera.GetPosition()) ),
                float4(zAxis.X, zAxis.Y, zAxis.Z, -Dot(zAxis, gMainCamera.GetPosition())),
                float4( 0.0f, 0.0f, 0.0f, 1.0f ),
            },
        };

        const float fovY = std::numbers::pi_v<float> / 4.0f;
        const float f = 1.0f / std::tan(fovY / 2.0f);
        const float aspectRatio = 1600.0f / 900.0f;
        const float nearPlane = 0.1f;
        const float farPlane = 1000.0f;

        gProjectionMatrix = float4x4
        {
            .Data =
            {
                float4(f / aspectRatio, 0.0f, 0.0f, 0.0f),
                float4(0.0f, f, 0.0f, 0.0f),
                float4(0.0f, 0.0f, farPlane / (farPlane - nearPlane), (-farPlane * nearPlane) / (farPlane - nearPlane)),
                float4(0.0f, 0.0f, 1.0f, 0.0f),
            },
        };

        outGeometries.clear();
        outGeometries.reserve(8);

        // Floor
        outGeometries.push_back(Geometry());
        Geometry& floor = outGeometries.back();
        {
            VertexBuffer floorVertexBuffer(
                VertexBuffer::CreateInfo
                {
                    .HandnessType = eHandnessType::LEFT,
                    .WindingType = eWindingType::COUNTER_CLOCKWISE,
                    .StrideInBytes = sizeof(VertexPN),
                    .Data = std::vector<byte>{}
                }
            );
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 552.8f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { 0.0f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 0.0f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 549.6f, 0.0f, 559.2f };
            std::vector<uint16> floorIndices;
            floorIndices.reserve(6);
            AddQuadVertices(floorVertexBuffer, floorIndices, v0, v1, v2, v3);
            floor.SetVertexBuffer(std::move(floorVertexBuffer));
            floor.SetIndices(std::move(floorIndices));
            floor.SetColor(WHITE);
        }

        // Light
        outGeometries.push_back(Geometry());
        Geometry& light = outGeometries.back();
        {
            VertexBuffer lightVertexBuffer(
                VertexBuffer::CreateInfo
                {
                    .HandnessType = eHandnessType::LEFT,
                    .WindingType = eWindingType::COUNTER_CLOCKWISE,
                    .StrideInBytes = sizeof(VertexPN),
                    .Data = std::vector<byte>{}
                }
            );
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 343.0f, 548.8f, 227.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { 343.0f, 548.8f, 332.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 213.0f, 548.8f, 332.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 213.0f, 548.8f, 227.0f };
            std::vector<uint16> lightIndices;
            lightIndices.reserve(6);
            AddQuadVertices(lightVertexBuffer, lightIndices, v0, v1, v2, v3);
            light.SetVertexBuffer(std::move(lightVertexBuffer));
            light.SetIndices(std::move(lightIndices));
            light.SetColor(WHITE);
            light.SetIsEmissive(true);
        }

        // Ceiling
        outGeometries.push_back(Geometry());
        Geometry& ceiling = outGeometries.back();
        {
            VertexBuffer ceilingVertexBuffer(
                VertexBuffer::CreateInfo
                {
                    .HandnessType = eHandnessType::LEFT,
                    .WindingType = eWindingType::COUNTER_CLOCKWISE,
                    .StrideInBytes = sizeof(VertexPN),
                    .Data = std::vector<byte>{}
                }
            );

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 556.0f, 548.8f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { 556.0f, 548.8f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 0.0f, 548.8f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 548.8f, 0.0f };
            std::vector<uint16> ceilingIndices;
            ceilingIndices.reserve(6);
            AddQuadVertices(ceilingVertexBuffer, ceilingIndices, v0, v1, v2, v3);
            ceiling.SetVertexBuffer(std::move(ceilingVertexBuffer));
            ceiling.SetIndices(std::move(ceilingIndices));
            ceiling.SetColor(WHITE);
        }
        
        // Back wall
        outGeometries.push_back(Geometry());
        Geometry& backWall = outGeometries.back();
        {
            VertexBuffer backWallVertexBuffer(
                VertexBuffer::CreateInfo
                {
                    .HandnessType = eHandnessType::LEFT,
                    .WindingType = eWindingType::COUNTER_CLOCKWISE,
                    .StrideInBytes = sizeof(VertexPN),
                    .Data = std::vector<byte>{}
                }
            );

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 549.6f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { 0.0f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 0.0f, 548.8f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 556.0f, 548.8f, 559.2f };
            std::vector<uint16> backWallIndices;
            backWallIndices.reserve(6);
            AddQuadVertices(backWallVertexBuffer, backWallIndices, v0, v1, v2, v3);
            backWall.SetVertexBuffer(std::move(backWallVertexBuffer));
            backWall.SetIndices(std::move(backWallIndices));
            backWall.SetColor(WHITE);
        }

        // Right wall
        outGeometries.push_back(Geometry());
        Geometry& rightWall = outGeometries.back();
        {
            VertexBuffer rightWallVertexBuffer(
                VertexBuffer::CreateInfo
                {
                    .HandnessType = eHandnessType::LEFT,
                    .WindingType = eWindingType::COUNTER_CLOCKWISE,
                    .StrideInBytes = sizeof(VertexPN),
                    .Data = std::vector<byte>{}
                }
            );
            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 0.0f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { 0.0f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 0.0f, 548.8f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 0.0f, 548.8f, 559.2f };
            std::vector<uint16> rightWallIndices;
            rightWallIndices.reserve(6);
            AddQuadVertices(rightWallVertexBuffer, rightWallIndices, v0, v1, v2, v3);
            rightWall.SetVertexBuffer(std::move(rightWallVertexBuffer));
            rightWall.SetIndices(std::move(rightWallIndices));
            rightWall.SetColor(GREEN);
        }

        // Left wall
        outGeometries.push_back(Geometry());
        Geometry& leftWall = outGeometries.back();
        {
            VertexBuffer leftWallVertexBuffer(
                VertexBuffer::CreateInfo
                {
                    .HandnessType = eHandnessType::LEFT,
                    .WindingType = eWindingType::COUNTER_CLOCKWISE,
                    .StrideInBytes = sizeof(VertexPN),
                    .Data = std::vector<byte>{}
                }
            );

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 552.8f, 0.0f, 0.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { 549.6f, 0.0f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 556.0f, 548.8f, 559.2f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 556.0f, 548.8f, 0.0f };
            std::vector<uint16> leftWallIndices;
            leftWallIndices.reserve(6);
            AddQuadVertices(leftWallVertexBuffer, leftWallIndices, v0, v1, v2, v3);
            leftWall.SetVertexBuffer(std::move(leftWallVertexBuffer));
            leftWall.SetIndices(std::move(leftWallIndices));
            leftWall.SetColor(RED);
        }

        // Short block
        outGeometries.push_back(Geometry());
        Geometry& shortBlock = outGeometries.back();
        {
            VertexBuffer shortBlockVertexBuffer(
                VertexBuffer::CreateInfo
                {
                    .HandnessType = eHandnessType::LEFT,
                    .WindingType = eWindingType::COUNTER_CLOCKWISE,
                    .StrideInBytes = sizeof(VertexPN),
                    .Data = std::vector<byte>{}
                }
            );

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 130.0f, 165.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { 82.0f, 165.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 240.0f, 165.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 290.0f, 165.0f, 114.0f };

            std::vector<uint16> shortBlockIndices;
            shortBlockIndices.reserve(6 * 5);
            AddQuadVertices(shortBlockVertexBuffer, shortBlockIndices, v0, v1, v2, v3);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v4 = { 290.0f, 0.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v5 = { 290.0f, 165.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v6 = { 240.0f, 165.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v7 = { 240.0f, 0.0f, 272.0f };
            AddQuadVertices(shortBlockVertexBuffer, shortBlockIndices, v4, v5, v6, v7);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v8 = { 130.0f, 0.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v9 = { 130.0f, 165.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v10 = { 290.0f, 165.0f, 114.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v11 = { 290.0f, 0.0f, 114.0f };
            AddQuadVertices(shortBlockVertexBuffer, shortBlockIndices, v8, v9, v10, v11);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v12 = { 82.0f, 0.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v13 = { 82.0f, 165.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v14 = { 130.0f, 165.0f, 65.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v15 = { 130.0f, 0.0f, 65.0f };
            AddQuadVertices(shortBlockVertexBuffer, shortBlockIndices, v12, v13, v14, v15);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v16 = { 240.0f, 0.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v17 = { 240.0f, 165.0f, 272.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v18 = { 82.0f, 165.0f, 225.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v19 = { 82.0f, 0.0f, 225.0f };
            AddQuadVertices(shortBlockVertexBuffer, shortBlockIndices, v16, v17, v18, v19);

            shortBlock.SetVertexBuffer(std::move(shortBlockVertexBuffer));
            shortBlock.SetIndices(std::move(shortBlockIndices));
            shortBlock.SetColor(WHITE);
        }

        // Tall block
        outGeometries.push_back(Geometry());
        Geometry& tallBlock = outGeometries.back();
        {
            VertexBuffer tallBlockVertexBuffer(
                VertexBuffer::CreateInfo
                {
                    .HandnessType = eHandnessType::LEFT,
                    .WindingType = eWindingType::COUNTER_CLOCKWISE,
                    .StrideInBytes = sizeof(VertexPN),
                    .Data = std::vector<byte>{}
                }
            );

            constexpr const Coordinate<eCoordinateSpace::WORLD> v0 = { 423.0f, 330.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v1 = { 265.0f, 330.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v2 = { 314.0f, 330.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v3 = { 472.0f, 330.0f, 406.0f };

            std::vector<uint16> tallBlockIndices;
            tallBlockIndices.reserve(6 * 5);
            AddQuadVertices(tallBlockVertexBuffer, tallBlockIndices, v0, v1, v2, v3);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v4 = { 423.0f, 0.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v5 = { 423.0f, 330.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v6 = { 472.0f, 330.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v7 = { 472.0f, 0.0f, 406.0f };
            AddQuadVertices(tallBlockVertexBuffer, tallBlockIndices, v4, v5, v6, v7);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v8 = { 472.0f, 0.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v9 = { 472.0f, 330.0f, 406.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v10 = { 314.0f, 330.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v11 = { 314.0f, 0.0f, 456.0f };
            AddQuadVertices(tallBlockVertexBuffer, tallBlockIndices, v8, v9, v10, v11);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v12 = { 314.0f, 0.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v13 = { 314.0f, 330.0f, 456.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v14 = { 265.0f, 330.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v15 = { 265.0f, 0.0f, 296.0f };
            AddQuadVertices(tallBlockVertexBuffer, tallBlockIndices, v12, v13, v14, v15);

            constexpr const Coordinate<eCoordinateSpace::WORLD> v16 = { 265.0f, 0.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v17 = { 265.0f, 330.0f, 296.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v18 = { 423.0f, 330.0f, 247.0f };
            constexpr const Coordinate<eCoordinateSpace::WORLD> v19 = { 423.0f, 0.0f, 247.0f };
            AddQuadVertices(tallBlockVertexBuffer, tallBlockIndices, v16, v17, v18, v19);

            tallBlock.SetVertexBuffer(std::move(tallBlockVertexBuffer));
            tallBlock.SetIndices(std::move(tallBlockIndices));
            tallBlock.SetColor(WHITE);
        }
    }

    CornellBoxVertexShaderOutput
    CornellBoxVertexShader(const VertexPN& input) noexcept
    {
        const HomogenousCoordinate<eCoordinateSpace::WORLD> wsInput(input.Position, 1.0f);
        const HomogenousCoordinate<eCoordinateSpace::VIEW> vsInput = gViewMatrix * wsInput;
        const HomogenousCoordinate<eCoordinateSpace::PERSPECTIVE> psInput = gProjectionMatrix * vsInput;
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
}
