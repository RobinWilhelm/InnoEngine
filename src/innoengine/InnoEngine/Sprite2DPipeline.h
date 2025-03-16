#pragma once
#include "SDL3/SDL_gpu.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/GPUDeviceRef.h"

#include "InnoEngine/Texture2D.h"

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
            FrameBufferIndex texture_index = -1;
            uint32_t         buffer_index  = 0;
            uint32_t         command_count         = 0;
        };

        struct Command
        {
            Command() = default;

            Command( Command&& other )
            {
                texture_index = other.texture_index;
                info          = other.info;
            }
            FrameBufferIndex texture_index;
            struct VertexUniform
            {
                float    x, y;
                uint32_t z;
                float    rotation; // radians
                float    origin_offset_x, origin_offset_y;    // for rotation
                float    width, height;
                DXSM::Vector4 source;
                DXSM::Color   color;
            } info;
        };

        using CommandList = std::vector<Command>;

    public:
        Sprite2DPipeline() = default;
        ~Sprite2DPipeline();

        Result initialize( GPURenderer* renderer, AssetManager* assetmanager );
        void   prepare_render( const CommandList& command_list );
        uint32_t swapchain_render( const DXSM::Matrix&   view_projection,
                                   const CommandList&    command_list,
                                   const TextureList&    texture_list,
                                   SDL_GPUCommandBuffer* cmdbuf,
                                   SDL_GPURenderPass*    renderPass );

    private:
        BatchData* add_batch();
        void       clear_batches();

        uint32_t       find_free_gpubuffer();
        SDL_GPUBuffer* get_gpubuffer_by_index( uint32_t index ) const;

        void sort_commands( const CommandList& command_list );

    private:
        bool                     m_initialized    = false;
        GPUDeviceRef             m_device         = nullptr;
        SDL_GPUGraphicsPipeline* m_pipeline       = nullptr;
        SDL_GPUSampler*          m_defaultSampler = nullptr;

        std::vector<const Command*> m_sortedCommands;    // objects owned by the RenderCommandBuffer

        SDL_GPUTransferBuffer* m_spriteTransferBuffer = nullptr;

        std::vector<SDL_GPUBuffer*> m_gpuBuffer;
        uint32_t                    m_gpuBuffer_used = 0;
        std::vector<BatchData>      m_batches;
    };

    using SpriteCommandBuffer = Sprite2DPipeline::CommandList;
}    // namespace InnoEngine
