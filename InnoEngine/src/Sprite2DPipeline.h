#pragma once
#include "SDL3/SDL_gpu.h"

#include "BaseTypes.h"
#include "Renderer.h"

#include "Sprite.h"
#include "Asset.h"
#include "AssetView.h"
#include "AssetRepository.h"

#include <string>
#include <memory>

namespace InnoEngine
{
    class Sprite2DPipeline
    {
    public:
        struct BatchData
        {
            AssetUID<Texture2D> texture;
            uint16_t            bufferIdx = 0;
            uint16_t            count     = 0;
        };

        struct Command
        {
            Command() = default;

            Command( Command&& other )
            {
                texture = other.texture;
                info    = other.info;
            }

            AssetUID<Texture2D> texture;

            struct VertexUniform
            {
                float         x, y;
                uint32_t      z;
                float         rotation;
                float         scale_w, scale_h, padding_a, padding_b;
                DXSM::Vector4 source;
                DXSM::Color   color;
            } info;
        };

        using CommandList = std::vector<Command>;

    public:
        Sprite2DPipeline() = default;
        virtual ~Sprite2DPipeline();

        Result initialize( GPURenderer* renderer, AssetManager* assetmanager );
        void   prepare_render( const CommandList& command_list, SDL_GPUDevice* gpudevice );
        void   swapchain_render( const DXSM::Matrix& view_projection, const CommandList& command_list, SDL_GPUCommandBuffer* cmdbuf, SDL_GPURenderPass* renderPass );

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

        Ref<AssetRepository<Texture2D>> m_textureAssets;

        std::vector<const Command*> m_sortedCommands;    // objects owned by the RenderCommandBuffer

        SDL_GPUTransferBuffer* m_spriteTransferBuffer = nullptr;

        std::vector<SDL_GPUBuffer*>              m_gpuBuffer;
        uint16_t                                 m_gpuBuffer_used = 0;
        std::vector<Sprite2DPipeline::BatchData> m_batches;
    };

}    // namespace InnoEngine
