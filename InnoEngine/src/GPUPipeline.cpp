#include "iepch.h"
#include "GPUPipeline.h"

#include "Renderer.h"
#include "RenderCommandBuffer.h"

GPUPipeline::~GPUPipeline()
{
    release();
}

void GPUPipeline::release()
{
    if ( m_pipeline ) {
        SDL_ReleaseGPUGraphicsPipeline( m_renderer->get_gpudevice(), m_pipeline );
        m_pipeline = nullptr;
    }
    m_initialized = false;
}

void GPUPipeline::submit()
{
    m_renderCmd->on_submit();
}
