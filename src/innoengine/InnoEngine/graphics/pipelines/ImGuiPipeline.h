#pragma once
#include "SDL3/SDL_gpu.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/GPUDeviceRef.h"

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

            CommandData& operator=( CommandData& other )
            {
                if ( this == &other )
                    return *this;

                TotalVertexCount = other.TotalVertexCount;
                TotalIndexCount  = other.TotalIndexCount;
                DisplayPos       = other.DisplayPos;
                DisplaySize      = other.DisplaySize;
                FrameBufferScale = other.FrameBufferScale;

                RenderCommandLists.reserve( other.RenderCommandLists.size() );
                for ( const auto& rcl : other.RenderCommandLists ) {
                    auto& new_rcl         = RenderCommandLists.emplace_back();
                    new_rcl.CommandBuffer = rcl.CommandBuffer;
                    new_rcl.IndexBuffer   = rcl.IndexBuffer;
                    new_rcl.VertexBuffer  = rcl.VertexBuffer;
                }

                return *this;
            }
        };

    public:
        ImGuiPipeline() = default;
        virtual ~ImGuiPipeline();

        // Inherited via GPUPipeline
        Result   initialize( GPURenderer* renderer );
        void     prepare_render( const CommandData& command_data );
        uint32_t swapchain_render( const CommandData& command_data, SDL_GPUCommandBuffer* gpu_cmd_buf, SDL_GPURenderPass* render_pass );

    private:
        void create_or_resize_buffer( SDL_GPUBuffer** buffer, uint32_t* old_size, uint32_t new_size, SDL_GPUBufferUsageFlags usage );

    private:
        GPUDeviceRef m_Device      = nullptr;
        bool         m_Initialized = false;
    };
}    // namespace InnoEngine
