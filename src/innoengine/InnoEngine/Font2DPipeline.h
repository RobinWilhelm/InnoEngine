#pragma once
#include "SDL3/SDL_gpu.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/GPUDeviceRef.h"

#include "InnoEngine/Font.h"
#include "InnoEngine/StringArena.h"

namespace InnoEngine
{
    class AssetManager;
    class GPURenderer;

    class Font2DPipeline
    {
    public:
        struct BatchData
        {
            FrameBufferIndex font_fbidx   = -1;
            uint32_t         buffer_index = 0;
            uint32_t         count        = 0;
        };

        struct Command
        {
            Command() = default;

            Command( Command&& other )
            {
                font_fbidx = other.font_fbidx;
                info       = other.info;
            }

            FrameBufferIndex font_fbidx;
            StringArenaIndex string_arena_index;
            uint32_t         string_size;

            struct VertexUniform
            {
                float         x, y;
                uint32_t      z;
                float         rotation;                            // radians
                float         origin_offset_x, origin_offset_y;    // for rotation
                float         width, height;
                DXSM::Vector4 source;
                DXSM::Color   color;
            } info;
        };

        using CommandList = std::vector<Command>;

    public:
        Font2DPipeline() = default;
        ~Font2DPipeline();

        Result initialize( GPURenderer* renderer, AssetManager* assetmanager );
        void   prepare_render( const CommandList& command_list, const FontList& texture_list, const StringArena& string_buffer );
        void   swapchain_render( const DXSM::Matrix&   view_projection,
                                 const CommandList&    command_list,
                                 const FontList&       texture_list,
                                 SDL_GPUCommandBuffer* cmdbuf,
                                 SDL_GPURenderPass*    renderPass );

    private:
        BatchData* add_batch();
        void       clear_batches();

        uint32_t       find_free_gpubuffer();
        SDL_GPUBuffer* get_gpubuffer_by_index( uint32_t index ) const;

        void sort_commands( const CommandList& command_list );

    private:
        bool                     m_initialized = false;
        GPUDeviceRef             m_device      = nullptr;
        SDL_GPUGraphicsPipeline* m_pipeline    = nullptr;
        SDL_GPUSampler*          m_fontSampler = nullptr;

        std::vector<const Command*> m_sortedCommands;    // objects owned by the RenderCommandBuffer
        SDL_GPUTransferBuffer*      m_transferBuffer = nullptr;

        static constexpr uint32_t MaxBatchSize = 20000;

        std::vector<SDL_GPUBuffer*> m_gpuBuffer;
        uint32_t                    m_gpuBuffer_used = 0;
        std::vector<BatchData>      m_batches;
    };

    using FontCommandBuffer = Font2DPipeline::CommandList;
}    // namespace InnoEngine
