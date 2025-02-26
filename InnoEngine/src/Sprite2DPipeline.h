#pragma once
#include "SDL3/SDL_gpu.h"

#include "SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

#include "Sprite.h"
#include "Asset.h"
#include "AssetView.h"
#include "AssetRepository.h"
#include "GPUPipeline.h"
#include "RenderCommandBuffer.h"

#include <string>
#include <memory>

class Sprite2DPipeline : public GPUPipeline
{
public:
    struct Vertex
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float u = 0.0f, v = 0.0f;
    };

    struct SpriteVertexUniform
    {
        float         x, y, z, rotation;
        float         scale_w, scale_h, padding_a, padding_b;
        DXSM::Vector4 source;
        DXSM::Color   color;
    };

    struct SpriteBatchInfo
    {
        SpriteBatchInfo() = default;

        SpriteBatchInfo( SpriteBatchInfo&& other )
        {
            texture = other.texture;
            info    = other.info;
        }

        AssetUID<Sprite>                      texture;
        Sprite2DPipeline::SpriteVertexUniform info;
    };

    struct BatchData
    {
        AssetUID<Sprite> texture;
        uint16_t         bufferIdx = 0;
        uint16_t         count     = 0;
    };

    using CommandQueue = DoubleBufferedCommandQueue<SpriteBatchInfo>;

public:
    Sprite2DPipeline() = default;
    virtual ~Sprite2DPipeline();

    // Geerbt über GPUPipeline
    bool init( GPURenderer* pRenderer ) override;
    void dispatch_copycommands( SDL_GPUCommandBuffer* cmdbuf, SDL_GPUCopyPass* renderPass ) override;
    void dispatch_rendercommands( const DXSM::Matrix& viewProjection, SDL_GPUCommandBuffer* cmdbuf, SDL_GPURenderPass* renderPass ) override;

    const std::string_view get_name() const override;
    void                   sort_commands() override;
    uint32_t               needs_processing() const override;

    void collect( AssetUID<Sprite> spriteUID, float x, float y, float angle = 0.0f, float scale_x = 1.0f, float scale_y = 1.0f, DXSM::Color color = { 1.0f, 1.0f, 1.0f, 1.0f }, uint16_t layer = 0 );

private:
    BatchData* add_batch();
    void       clear_batches();

    uint32_t       find_free_gpubuffer();
    SDL_GPUBuffer* get_gpubuffer_by_index( uint32_t index ) const;

    Sprite2DPipeline::CommandQueue* get_commandqueue() const;

private:
    bool                                   m_initialized = false;
    std::weak_ptr<AssetRepository<Sprite>> m_spriteAssets;

    SDL_GPUTransferBuffer* m_spriteTransferBuffer = nullptr;

    std::vector<SDL_GPUBuffer*>              m_gpuBuffer;
    uint16_t                                 m_gpuBuffer_used = 0;
    std::vector<Sprite2DPipeline::BatchData> m_batches;
};
