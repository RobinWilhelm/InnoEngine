#include "iepch.h"
#include "ImGuiPipeline.h"

#include "Window.h"
#include "Renderer.h"

#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"

namespace InnoEngine
{
    // Reusable buffers used for rendering 1 current in-flight frame, for ImGui_ImplSDLGPU3_RenderDrawData()
    struct ImGui_ImplSDLGPU3_FrameData
    {
        SDL_GPUBuffer* VertexBuffer     = nullptr;
        SDL_GPUBuffer* IndexBuffer      = nullptr;
        uint32_t       VertexBufferSize = 0;
        uint32_t       IndexBufferSize  = 0;
    };

    struct ImGui_ImplSDLGPU3_Data
    {
        ImGui_ImplSDLGPU3_InitInfo InitInfo;

        // Graphics pipeline & shaders
        SDL_GPUShader*           VertexShader   = nullptr;
        SDL_GPUShader*           FragmentShader = nullptr;
        SDL_GPUGraphicsPipeline* Pipeline       = nullptr;

        // Font data
        SDL_GPUSampler*              FontSampler = nullptr;
        SDL_GPUTexture*              FontTexture = nullptr;
        SDL_GPUTextureSamplerBinding FontBinding = { nullptr, nullptr };

        // Frame data for main window
        ImGui_ImplSDLGPU3_FrameData MainWindowFrameData;
    };

    ImGuiPipeline::~ImGuiPipeline()
    {
        if ( m_initialized ) {
            ImGui_ImplSDL3_Shutdown();
            ImGui_ImplSDLGPU3_Shutdown();
            ImGui::DestroyContext();
        }
    }

    Result ImGuiPipeline::initialize( GPURenderer* renderer )
    {
        IE_ASSERT( renderer != nullptr && renderer->get_window() != nullptr );
        if ( m_initialized )
            return Result::AlreadyInitialized;

        // Setup Dear ImGui context
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    // Enable Keyboard Controls
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForSDLGPU( renderer->get_window()->get_sdlwindow() );
        ImGui_ImplSDLGPU3_InitInfo init_info = {};
        init_info.Device                     = renderer->get_gpudevice();
        init_info.ColorTargetFormat          = SDL_GetGPUSwapchainTextureFormat( renderer->get_gpudevice(), renderer->get_window()->get_sdlwindow() );
        init_info.MSAASamples                = SDL_GPU_SAMPLECOUNT_1;
        ImGui_ImplSDLGPU3_Init( &init_info );

        m_initialized = true;
        return Result::Success;
    }

    void ImGuiPipeline::prepare_render( const CommandData& command_data, SDL_GPUDevice* gpudevice )
    {
        IE_ASSERT( gpudevice );

        SDL_GPUCommandBuffer* copyCmdbuf = SDL_AcquireGPUCommandBuffer( gpudevice );
        if ( copyCmdbuf == nullptr ) {
            IE_LOG_ERROR( "AcquireGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }

        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fb_width  = (int)( command_data.DisplaySize.x * command_data.FrameBufferScale.x );
        int fb_height = (int)( command_data.DisplaySize.y * command_data.FrameBufferScale.y );
        if ( fb_width <= 0 || fb_height <= 0 || command_data.TotalVertexCount <= 0 )
            return;

        ImGui_ImplSDLGPU3_Data*      bd = ImGui::GetCurrentContext() ? (ImGui_ImplSDLGPU3_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
        ImGui_ImplSDLGPU3_FrameData* fd = &bd->MainWindowFrameData;

        uint32_t vertex_size = command_data.TotalVertexCount * sizeof( ImDrawVert );
        uint32_t index_size  = command_data.TotalIndexCount * sizeof( ImDrawIdx );
        if ( fd->VertexBuffer == nullptr || fd->VertexBufferSize < vertex_size )
            create_or_resize_buffer( &fd->VertexBuffer, &fd->VertexBufferSize, vertex_size, SDL_GPU_BUFFERUSAGE_VERTEX );
        if ( fd->IndexBuffer == nullptr || fd->IndexBufferSize < index_size )
            create_or_resize_buffer( &fd->IndexBuffer, &fd->IndexBufferSize, index_size, SDL_GPU_BUFFERUSAGE_INDEX );

        // FIXME: It feels like more code could be shared there.
        SDL_GPUTransferBufferCreateInfo vertex_transferbuffer_info = {};
        vertex_transferbuffer_info.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        vertex_transferbuffer_info.size                            = vertex_size;
        SDL_GPUTransferBufferCreateInfo index_transferbuffer_info  = {};
        index_transferbuffer_info.usage                            = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        index_transferbuffer_info.size                             = index_size;

        SDL_GPUTransferBuffer* vertex_transferbuffer = SDL_CreateGPUTransferBuffer( gpudevice, &vertex_transferbuffer_info );
        IM_ASSERT( vertex_transferbuffer != nullptr && "Failed to create the vertex transfer buffer, call SDL_GetError() for more information" );
        SDL_GPUTransferBuffer* index_transferbuffer = SDL_CreateGPUTransferBuffer( gpudevice, &index_transferbuffer_info );
        IM_ASSERT( index_transferbuffer != nullptr && "Failed to create the index transfer buffer, call SDL_GetError() for more information" );

        ImDrawVert* vtx_dst = (ImDrawVert*)SDL_MapGPUTransferBuffer( gpudevice, vertex_transferbuffer, true );
        ImDrawIdx*  idx_dst = (ImDrawIdx*)SDL_MapGPUTransferBuffer( gpudevice, index_transferbuffer, true );
        for ( const auto& cmdList : command_data.RenderCommandLists ) {
            memcpy( vtx_dst, cmdList.VertexBuffer.Data, cmdList.VertexBuffer.Size * sizeof( ImDrawVert ) );
            memcpy( idx_dst, cmdList.IndexBuffer.Data, cmdList.IndexBuffer.Size * sizeof( ImDrawIdx ) );
            vtx_dst += cmdList.VertexBuffer.Size;
            idx_dst += cmdList.IndexBuffer.Size;
        }
        SDL_UnmapGPUTransferBuffer( gpudevice, vertex_transferbuffer );
        SDL_UnmapGPUTransferBuffer( gpudevice, index_transferbuffer );

        SDL_GPUTransferBufferLocation vertex_buffer_location = {};
        vertex_buffer_location.offset                        = 0;
        vertex_buffer_location.transfer_buffer               = vertex_transferbuffer;
        SDL_GPUTransferBufferLocation index_buffer_location  = {};
        index_buffer_location.offset                         = 0;
        index_buffer_location.transfer_buffer                = index_transferbuffer;

        SDL_GPUBufferRegion vertex_buffer_region = {};
        vertex_buffer_region.buffer              = fd->VertexBuffer;
        vertex_buffer_region.offset              = 0;
        vertex_buffer_region.size                = vertex_size;

        SDL_GPUBufferRegion index_buffer_region = {};
        index_buffer_region.buffer              = fd->IndexBuffer;
        index_buffer_region.offset              = 0;
        index_buffer_region.size                = index_size;

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass( copyCmdbuf );
        SDL_UploadToGPUBuffer( copy_pass, &vertex_buffer_location, &vertex_buffer_region, true );
        SDL_UploadToGPUBuffer( copy_pass, &index_buffer_location, &index_buffer_region, true );
        SDL_EndGPUCopyPass( copy_pass );
        SDL_ReleaseGPUTransferBuffer( gpudevice, index_transferbuffer );
        SDL_ReleaseGPUTransferBuffer( gpudevice, vertex_transferbuffer );

        if ( SDL_SubmitGPUCommandBuffer( copyCmdbuf ) == false ) {
            IE_LOG_ERROR( "SDL_SubmitGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }
    }

    void ImGuiPipeline::swapchain_render( const CommandData& command_data, SDL_GPUCommandBuffer* gpu_cmd_buf, SDL_GPURenderPass* render_pass )
    {
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fb_width  = (int)( command_data.DisplaySize.x * command_data.FrameBufferScale.x );
        int fb_height = (int)( command_data.DisplaySize.y * command_data.FrameBufferScale.y );
        if ( fb_width <= 0 || fb_height <= 0 )
            return;

        ImGui_ImplSDLGPU3_Data*      bd = ImGui::GetCurrentContext() ? (ImGui_ImplSDLGPU3_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
        ImGui_ImplSDLGPU3_FrameData* fd = &bd->MainWindowFrameData;

        // Bind graphics pipeline
        SDL_BindGPUGraphicsPipeline( render_pass, bd->Pipeline );

        // Bind Vertex And Index Buffers
        if ( command_data.TotalVertexCount > 0 ) {
            SDL_GPUBufferBinding vertex_buffer_binding = {};
            vertex_buffer_binding.buffer               = fd->VertexBuffer;
            vertex_buffer_binding.offset               = 0;
            SDL_GPUBufferBinding index_buffer_binding  = {};
            index_buffer_binding.buffer                = fd->IndexBuffer;
            index_buffer_binding.offset                = 0;
            SDL_BindGPUVertexBuffers( render_pass, 0, &vertex_buffer_binding, 1 );
            SDL_BindGPUIndexBuffer( render_pass, &index_buffer_binding,
                                    sizeof( ImDrawIdx ) == 2 ? SDL_GPU_INDEXELEMENTSIZE_16BIT : SDL_GPU_INDEXELEMENTSIZE_32BIT );
        }

        // Setup viewport
        SDL_GPUViewport viewport = {};
        viewport.x               = 0;
        viewport.y               = 0;
        viewport.w               = (float)fb_width;
        viewport.h               = (float)fb_height;
        viewport.min_depth       = 0.0f;
        viewport.max_depth       = 1.0f;
        SDL_SetGPUViewport( render_pass, &viewport );

        // Setup scale and translation
        // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos
        // is (0,0) for single viewport apps.
        struct UBO
        {
            float scale[ 2 ];
            float translation[ 2 ];
        } ubo;

        ubo.scale[ 0 ]       = 2.0f / command_data.DisplaySize.x;
        ubo.scale[ 1 ]       = 2.0f / command_data.DisplaySize.y;
        ubo.translation[ 0 ] = -1.0f - command_data.DisplayPos.x * ubo.scale[ 0 ];
        ubo.translation[ 1 ] = -1.0f - command_data.DisplayPos.y * ubo.scale[ 1 ];
        SDL_PushGPUVertexUniformData( gpu_cmd_buf, 0, &ubo, sizeof( UBO ) );

        // Will project scissor/clipping rectangles into framebuffer space
        ImVec2 clip_off   = command_data.DisplayPos;          // (0,0) unless using multi-viewports
        ImVec2 clip_scale = command_data.FrameBufferScale;    // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int global_vtx_offset = 0;
        int global_idx_offset = 0;
        for ( const auto& cmdList : command_data.RenderCommandLists ) {
            for ( const auto& renderCmd : cmdList.CommandBuffer ) {
                /*
                // Usercallbacks are not supported for now
                if ( pcmd->UserCallback != nullptr ) {
                    pcmd->UserCallback( draw_list, pcmd );
                }
                else
                */
                {
                    // Project scissor/clipping rectangles into framebuffer space
                    ImVec2 clip_min( ( renderCmd.ClipRect.x - clip_off.x ) * clip_scale.x, ( renderCmd.ClipRect.y - clip_off.y ) * clip_scale.y );
                    ImVec2 clip_max( ( renderCmd.ClipRect.z - clip_off.x ) * clip_scale.x, ( renderCmd.ClipRect.w - clip_off.y ) * clip_scale.y );

                    // Clamp to viewport as SDL_SetGPUScissor() won't accept values that are off bounds
                    if ( clip_min.x < 0.0f ) {
                        clip_min.x = 0.0f;
                    }
                    if ( clip_min.y < 0.0f ) {
                        clip_min.y = 0.0f;
                    }
                    if ( clip_max.x > fb_width ) {
                        clip_max.x = (float)fb_width;
                    }
                    if ( clip_max.y > fb_height ) {
                        clip_max.y = (float)fb_height;
                    }
                    if ( clip_max.x <= clip_min.x || clip_max.y <= clip_min.y )
                        continue;

                    // Apply scissor/clipping rectangle
                    SDL_Rect scissor_rect = {};
                    scissor_rect.x        = (int)clip_min.x;
                    scissor_rect.y        = (int)clip_min.y;
                    scissor_rect.w        = (int)( clip_max.x - clip_min.x );
                    scissor_rect.h        = (int)( clip_max.y - clip_min.y );
                    SDL_SetGPUScissor( render_pass, &scissor_rect );

                    // Bind DescriptorSet with font or user texture
                    SDL_BindGPUFragmentSamplers( render_pass, 0, (SDL_GPUTextureSamplerBinding*)renderCmd.GetTexID(), 1 );

                    // Draw
                    SDL_DrawGPUIndexedPrimitives( render_pass, renderCmd.ElemCount, 1, renderCmd.IdxOffset + global_idx_offset,
                                                  renderCmd.VtxOffset + global_vtx_offset, 0 );
                }
            }
            global_idx_offset += cmdList.IndexBuffer.Size;
            global_vtx_offset += cmdList.VertexBuffer.Size;
        }

        // Note: at this point both SDL_SetGPUViewport() and SDL_SetGPUScissor() have been called.
        // Our last values will leak into user/application rendering if you forgot to call SDL_SetGPUViewport() and SDL_SetGPUScissor() yourself to
        // explicitly set that state In theory we should aim to backup/restore those values but I am not sure this is possible. We perform a call to
        // SDL_SetGPUScissor() to set back a full viewport which is likely to fix things for 99% users but technically this is not perfect. (See github
        // #4644)
        SDL_Rect scissor_rect { 0, 0, fb_width, fb_height };
        SDL_SetGPUScissor( render_pass, &scissor_rect );
    }

    void ImGuiPipeline::create_or_resize_buffer( SDL_GPUBuffer** buffer, uint32_t* old_size, uint32_t new_size, SDL_GPUBufferUsageFlags usage )
    {
        ImGui_ImplSDLGPU3_Data*     bd = ImGui::GetCurrentContext() ? (ImGui_ImplSDLGPU3_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
        ImGui_ImplSDLGPU3_InitInfo* v  = &bd->InitInfo;

        // Even though this is fairly rarely called.
        SDL_WaitForGPUIdle( v->Device );
        SDL_ReleaseGPUBuffer( v->Device, *buffer );

        SDL_GPUBufferCreateInfo buffer_info = {};
        buffer_info.usage                   = usage;
        buffer_info.size                    = new_size;
        buffer_info.props                   = 0;
        *buffer                             = SDL_CreateGPUBuffer( v->Device, &buffer_info );
        *old_size                           = new_size;
        IM_ASSERT( *buffer != nullptr && "Failed to create GPU Buffer, call SDL_GetError() for more information" );
    }
}    // namespace InnoEngine
