#pragma once
#include "SDL3/SDL_gpu.h"

#include "GPUPipeline.h"

namespace InnoEngine
{
    class ImGuiPipeline : public GPUPipeline
    {

    public:
        ImGuiPipeline() = default;
        virtual ~ImGuiPipeline();

        // Inherited via GPUPipeline
        bool                   init( GPURenderer* pRenderer ) override;
        virtual void           release() override;
        virtual void           submit() override;
        const std::string_view get_name() const override;
        uint32_t               needs_processing() const override;
        void                   prepare_render( SDL_GPUDevice* gpudevice ) override;
        void                   swapchain_render( const DXSM::Matrix& viewProjection, SDL_GPUCommandBuffer* cmdbuf, SDL_GPURenderPass* renderPass ) override;

        void collect( ImDrawData* drawData );

    private:
        void create_or_resize_buffer(SDL_GPUBuffer** buffer, uint32_t* old_size, uint32_t new_size, SDL_GPUBufferUsageFlags usage);

    private:
        bool m_initialized = false;

        // using ImVector here for ease of use
        struct RenderCommandList
        {
            ImVector<ImDrawCmd>  CommandBuffer;
            ImVector<ImDrawIdx>  IndexBuffer;
            ImVector<ImDrawVert> VertexBuffer;
        };

        ImDrawData*                    m_drawData         = nullptr;
        int                            m_totalVertexCount = 0;
        int                            m_totalIndexCount  = 0;
        ImVec2                         m_displayPos;
        ImVec2                         m_displaySize;
        ImVec2                         m_frameBufferScale;
        std::vector<RenderCommandList> m_renderCommandLists;
    };
}    // namespace InnoEngine
