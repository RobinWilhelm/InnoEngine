#pragma once
#include "SDL3/SDL_gpu.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/GPUDeviceRef.h"

#include "imgui.h"

namespace InnoEngine
{
    class GPURenderer;

    class ImGuiPipeline
    {
    public:
        // using ImVector here for ease of use
        struct RenderCommandList
        {
            ImVector<ImDrawCmd>  CommandBuffer;
            ImVector<ImDrawIdx>  IndexBuffer;
            ImVector<ImDrawVert> VertexBuffer;
        };

        struct CommandData
        {
            int                            TotalVertexCount = 0;
            int                            TotalIndexCount  = 0;
            ImVec2                         DisplayPos       = { 0, 0 };
            ImVec2                         DisplaySize      = { 0, 0 };
            ImVec2                         FrameBufferScale = { 0, 0 };
            std::vector<RenderCommandList> RenderCommandLists;
        };

    public:
        ImGuiPipeline() = default;
        virtual ~ImGuiPipeline();

        // Inherited via GPUPipeline
        Result initialize( GPURenderer* renderer );
        void   prepare_render( const CommandData& command_data );
        void   swapchain_render( const CommandData& command_data, SDL_GPUCommandBuffer* gpu_cmd_buf, SDL_GPURenderPass* render_pass );

    private:
        void create_or_resize_buffer( SDL_GPUBuffer** buffer, uint32_t* old_size, uint32_t new_size, SDL_GPUBufferUsageFlags usage );

    private:
        GPUDeviceRef m_device      = nullptr;
        bool         m_initialized = false;
    };
}    // namespace InnoEngine
