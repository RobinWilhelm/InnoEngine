#pragma once
#include "SDL3/SDL_gpu.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/GPUDeviceRef.h"

#include "InnoEngine/graphics/Font.h"
#include "InnoEngine/utility/StringArena.h"
#include "InnoEngine/graphics/GPUBatchBuffer.h"

namespace InnoEngine
{
    class AssetManager;
    class GPURenderer;

    class Font2DPipeline
    {
    public:
        struct BatchData
        {
            FrameBufferIndex FontFBIndex = -1;
        };

        struct Command
        {
            FrameBufferIndex FontFBIndex      = -1;
            StringArenaIndex StringIndex    = 0;
            uint32_t         StringLength     = 0;
            uint32_t         FontSize       = 0;
            DXSM::Vector2    Position        = {};
            DXSM::Color      ForegroundColor = {};
            float            Depth           = 0.0f;
        };

        struct StructuredBufferLayout
        {
            DXSM::Vector2 Position         = {};
            DXSM::Vector2 Size             = {};
            DXSM::Vector4 SourceRect       = {};
            DXSM::Color   ForegroundColor  = {};
            float         Depth            = 0.0f;
            float         ScreenPixelWidth = 0.0f;
            float pad [2];
        };

        using CommandList = std::vector<Command>;

    public:
        Font2DPipeline() = default;
        ~Font2DPipeline();

        Result   initialize( GPURenderer* renderer, AssetManager* assetmanager );
        void     prepare_render( const CommandList& command_list, const FontList& texture_list, const StringArena& string_buffer );
        uint32_t swapchain_render( const DXSM::Matrix&   view_projection,
                                   const FontList&       texture_list,
                                   SDL_GPUCommandBuffer* gpu_cmd_buf,
                                   SDL_GPURenderPass*    render_pass );

    private:
        void sort_commands( const CommandList& command_list );

    private:
        bool                     m_Initialized = false;
        GPUDeviceRef             m_Device      = nullptr;
        SDL_GPUGraphicsPipeline* m_Pipeline    = nullptr;
        SDL_GPUSampler*          m_FontSampler = nullptr;

        std::vector<const Command*> m_SortedCommands;    // objects owned by the RenderCommandBuffer

        static constexpr uint32_t                                     MaxBatchSize = 20000;
        Ref<GPUBatchStorageBuffer<StructuredBufferLayout, BatchData>> m_GPUBatch;
    };

    using FontCommandBuffer = Font2DPipeline::CommandList;
}    // namespace InnoEngine
