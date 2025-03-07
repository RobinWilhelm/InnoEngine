#pragma once
#include "SDL3/SDL_gpu.h"

#include "SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

#include <memory>
#include <string>
#include <vector>

namespace InnoEngine
{
    class GPURenderer;

    enum PipelineCommand
    {
        Render  = 1,
        Compute = 2
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
        virtual ~GPUPipeline()                             = default;

        virtual bool init( GPURenderer* pRenderer )                                                                                                    = 0;
        virtual void prepare_render( const RenderCommandBuffer& render_command_buffer, SDL_GPUDevice* gpudevice )                                      = 0;
        virtual void swapchain_render( const RenderCommandBuffer& render_command_buffer, SDL_GPUCommandBuffer* cmdbuf, SDL_GPURenderPass* renderPass ) = 0;

    protected:
        GPURenderer* m_renderer = nullptr;
    };
}    // namespace InnoEngine
