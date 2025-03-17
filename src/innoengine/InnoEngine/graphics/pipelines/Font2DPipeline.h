#pragma once
#include "SDL3/SDL_gpu.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/GPUDeviceRef.h"

#include "InnoEngine/graphics/Font.h"
#include "InnoEngine/utility/StringArena.h"

namespace InnoEngine
{
    class AssetManager;
    class GPURenderer;

    class Font2DPipeline
    {
    public:
        struct BatchData
        {
            FrameBufferIndex font_fbidx    = -1;
            uint32_t         buffer_index  = 0;
            uint32_t         command_count = 0;
        };

        struct Command
        {
            FrameBufferIndex font_fbidx   = -1;
            StringArenaIndex string_index = 0;
            uint32_t         string_size  = 0;
            uint32_t         font_size    = 0;
            float            x            = 0;
            float            y            = 0;
            DXSM::Color      color        = {};
            float            depth        = 0.0f;
        };

        struct StructuredBufferLayout
        {
            DXSM::Vector4 destination      = {};
            DXSM::Vector4 source           = {};
            DXSM::Color   color            = {};
            float         depth            = 0.0f;
            float         screen_pix_width = 0.0f;
            float         pad[ 2 ]         = {};
        };

        using CommandList = std::vector<Command>;

    public:
        Font2DPipeline() = default;
        ~Font2DPipeline();

        Result   initialize( GPURenderer* renderer, AssetManager* assetmanager );
        void     prepare_render( const CommandList& command_list, const FontList& texture_list, const StringArena& string_buffer );
        uint32_t swapchain_render( const DXSM::Matrix&   view_projection,
                                   const CommandList&    command_list,
                                   const FontList&       texture_list,
                                   SDL_GPUCommandBuffer* gpu_cmd_buf,
                                   SDL_GPURenderPass*    render_pass );

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

        static constexpr uint32_t MaxBatchSize = 200000;

        std::vector<SDL_GPUBuffer*> m_gpuBuffer;
        uint32_t                    m_gpuBuffer_used = 0;
        std::vector<BatchData>      m_batches;
    };

    using FontCommandBuffer = Font2DPipeline::CommandList;
}    // namespace InnoEngine
