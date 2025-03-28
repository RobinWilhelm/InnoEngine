#pragma once
#include "SDL3/SDL_gpu.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/GPUDeviceRef.h"

#include "InnoEngine/graphics/Texture2D.h"
#include "InnoEngine/graphics/GPUBatchBuffer.h"

#include <string>
#include <memory>

namespace InnoEngine
{
    class AssetManager;
    class GPURenderer;

    class Sprite2DPipeline
    {
    public:
        struct BatchData
        {
            RenderCommandBufferIndexType ContextIndex = InvalidRenderCommandBufferIndex;
            RenderCommandBufferIndexType TextureIndex = InvalidRenderCommandBufferIndex;
        };

        struct Command : RenderCommandBase
        {
            RenderCommandBufferIndexType TextureIndex;

            DXSM::Vector4 SourceRect;
            DXSM::Color   Color;
            DXSM::Vector2 Position;
            DXSM::Vector2 Size;
            DXSM::Vector2 OriginOffset;    // for rotation, in texels
            float         Rotation;        // in radians
        };

        struct StructuredBufferLayout
        {
            DXSM::Vector4 SourceRect;
            DXSM::Color   Color;
            DXSM::Vector2 Position;
            DXSM::Vector2 Size;
            DXSM::Vector2 OriginOffset;    // for rotation, in texels
            float         Depth;
            float         Rotation;    // in radians
            uint32_t      ContextIndex;
            float         pad[ 3 ];
        };

        using CommandList = std::vector<Command>;

    public:
        Sprite2DPipeline() = default;
        ~Sprite2DPipeline();

        Result   initialize( GPURenderer* renderer, AssetManager* assetmanager );
        void     prepare_render( const CommandList& command_list );
        uint32_t swapchain_render( const RenderContext* render_ctx,
                                   const TextureList& texture_list,
                                   SDL_GPURenderPass* renderPass );

    private:
        void sort_commands( const CommandList& command_list );

    private:
        bool                     m_Initialized    = false;
        GPUDeviceRef             m_Device         = nullptr;
        SDL_GPUGraphicsPipeline* m_Pipeline       = nullptr;
        SDL_GPUSampler*          m_DefaultSampler = nullptr;

        std::vector<const Command*> m_SortedCommands;    // objects owned by the RenderCommandBuffer

        static constexpr uint32_t MaxBatchSize = 20000;

        Ref<GPUBatchStorageBuffer<StructuredBufferLayout, BatchData>> m_GPUBatch;
    };

    using SpriteCommandBuffer = Sprite2DPipeline::CommandList;
}    // namespace InnoEngine
