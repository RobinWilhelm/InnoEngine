#pragma once
#include "SDL3/SDL_gpu.h"

#include "SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

#include <memory>
#include <string>
#include <vector>

class GPURenderer;
class RenderCommandQueue;

enum PipelineCommand
{
    Render  = 1,
    Copy    = 2,
    Compute = 4
};

class GPUPipeline
{
public:
    GPUPipeline() = default;

    // not needed for now
    GPUPipeline( const GPUPipeline& other )            = delete;
    GPUPipeline( GPUPipeline&& other )                 = delete;
    GPUPipeline& operator=( const GPUPipeline& other ) = delete;
    GPUPipeline& operator=( GPUPipeline&& other )      = delete;
    virtual ~GPUPipeline();

    virtual bool                   init( GPURenderer* pRenderer ) = 0;
    virtual void                   release();
    virtual void                   submit();
    virtual const std::string_view get_name() const         = 0;
    virtual uint32_t               needs_processing() const = 0;
    virtual void                   sort_commands() { };
    virtual void                   dispatch_copycommands( SDL_GPUCommandBuffer* cmdbuf, SDL_GPUCopyPass* renderPass )                                         = 0;
    virtual void                   dispatch_rendercommands( const DXSM::Matrix& viewProjection, SDL_GPUCommandBuffer* cmdbuf, SDL_GPURenderPass* renderPass ) = 0;

protected:
    bool                                m_initialized = false;
    GPURenderer*                        m_renderer    = nullptr;
    SDL_GPUGraphicsPipeline*            m_pipeline    = nullptr;
    std::shared_ptr<RenderCommandQueue> m_renderCmd;
};
