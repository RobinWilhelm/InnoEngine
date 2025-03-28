#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/pipelines/Primitive2DPipeline.h"

#include "InnoEngine/graphics/Renderer.h"
#include "InnoEngine/graphics/Window.h"
#include "InnoEngine/graphics/Shader.h"

#include "InnoEngine/AssetManager.h"

namespace InnoEngine
{
    Primitive2DPipeline::~Primitive2DPipeline()
    {
        if ( m_QuadPipeline ) {
            SDL_ReleaseGPUGraphicsPipeline( m_Device, m_QuadPipeline );
            m_QuadPipeline = nullptr;
        }

        if ( m_LinePipeline ) {
            SDL_ReleaseGPUGraphicsPipeline( m_Device, m_LinePipeline );
            m_LinePipeline = nullptr;
        }

        if ( m_CirclePipeline ) {
            SDL_ReleaseGPUGraphicsPipeline( m_Device, m_CirclePipeline );
            m_CirclePipeline = nullptr;
        }
    }

    Result Primitive2DPipeline::initialize( GPURenderer* renderer, AssetManager* asset_manager )
    {
        IE_ASSERT( renderer != nullptr && renderer->has_window() );

        if ( m_Initialized ) {
            IE_LOG_WARNING( "Pipeline already initialized!" );
            return Result::AlreadyInitialized;
        }

        m_Device           = renderer->get_gpudevice();
        SDL_Window* window = renderer->get_window()->get_sdlwindow();

        auto shaderRepo = asset_manager->get_repository<Shader>();
        IE_ASSERT( shaderRepo != nullptr );

        Result res = Result::Success;

        res = load_quad_pipeline( window, shaderRepo.get() );
        RETURN_RESULT_IF_FAILED( res );

        res = load_line_pipeline( window, shaderRepo.get() );
        RETURN_RESULT_IF_FAILED( res );

        res = load_circle_pipeline( window, shaderRepo.get() );
        RETURN_RESULT_IF_FAILED( res );

        m_Initialized = true;
        return res;
    }

    void Primitive2DPipeline::prepare_render( const QuadCommandList&   quad_command_list,
                                              const LineCommandList&   line_command_list,
                                              const CircleCommandList& circle_command_list )
    {
        IE_ASSERT( m_Device != nullptr );
        sort_quad_commands( quad_command_list );
        sort_line_commands( line_command_list );
        sort_circle_commands( circle_command_list );
        m_QuadGPUBatch->clear();
        m_LineGPUBatch->clear();
        m_CircleGPUBatch->clear();

        if ( quad_command_list.size() == 0 && line_command_list.size() == 0 && circle_command_list.size() == 0 )
            return;

        SDL_GPUCommandBuffer* gpu_copy_cmd_buf = SDL_AcquireGPUCommandBuffer( m_Device );
        if ( gpu_copy_cmd_buf == nullptr ) {
            IE_LOG_ERROR( "AcquireGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass( gpu_copy_cmd_buf );

        BatchData* current = nullptr;
        for ( const QuadCommand* command : m_SortedQuadCommands ) {
            if ( m_QuadGPUBatch->current_batch_full() ||
                 current == nullptr ||
                 current->ContextIndex != command->ContextIndex ) {

                current               = m_QuadGPUBatch->upload_and_add_batch( copy_pass );
                current->ContextIndex = command->ContextIndex;
            }

            QuadStorageBufferLayout* buffer_data = m_QuadGPUBatch->next_data();

            buffer_data->ContextIndex = command->ContextIndex;
            buffer_data->Color        = command->Color;
            buffer_data->Depth        = command->Depth;
            buffer_data->Position     = command->Position;
            buffer_data->Rotation     = command->Rotation;
            buffer_data->Size         = command->Size;
        }
        m_QuadGPUBatch->upload_last( copy_pass );

        current = nullptr;
        for ( const LineCommand* command : m_SortedLineCommands ) {
            if ( m_LineGPUBatch->current_batch_full() ||
                 current == nullptr ||
                 current->ContextIndex != command->ContextIndex ) {

                current               = m_LineGPUBatch->upload_and_add_batch( copy_pass );
                current->ContextIndex = command->ContextIndex;
            }

            LineStorageBufferLayout* buffer_data = m_LineGPUBatch->next_data();

            buffer_data->ContextIndex = command->ContextIndex;
            buffer_data->Color        = command->Color;
            buffer_data->Thickness    = command->Thickness;
            buffer_data->EdgeFade     = command->EdgeFade;
            buffer_data->Start        = command->Start;
            buffer_data->End          = command->End;
            buffer_data->Depth        = command->Depth;
        }
        m_LineGPUBatch->upload_last( copy_pass );

        current = nullptr;
        for ( const CircleCommand* command : m_SortedCircleCommands ) {
            if ( m_CircleGPUBatch->current_batch_full() ||
                 current == nullptr ||
                 current->ContextIndex != command->ContextIndex ) {

                current               = m_CircleGPUBatch->upload_and_add_batch( copy_pass );
                current->ContextIndex = command->ContextIndex;
            }
            CircleStorageBufferLayout* buffer_data = m_CircleGPUBatch->next_data();

            buffer_data->ContextIndex = command->ContextIndex;
            buffer_data->Color        = command->Color;
            buffer_data->Depth        = command->Depth;
            buffer_data->Position     = command->Position;
            buffer_data->Fade         = command->Fade;
            buffer_data->Thickness    = command->Thickness;
            buffer_data->Radius       = command->Radius;
        }
        m_CircleGPUBatch->upload_last( copy_pass );

        SDL_EndGPUCopyPass( copy_pass );

        if ( SDL_SubmitGPUCommandBuffer( gpu_copy_cmd_buf ) == false ) {
            IE_LOG_ERROR( "SDL_SubmitGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }
    }

    uint32_t Primitive2DPipeline::swapchain_render( const RenderContext* render_ctx, SDL_GPURenderPass* render_pass )
    {
        IE_ASSERT( m_Device != nullptr );
        IE_ASSERT( render_pass != nullptr );

        SDL_BindGPUVertexBuffers( render_pass, 0, nullptr, 0 );

        const auto&     vp = render_ctx->get_viewport();
        SDL_GPUViewport viewport( vp.LeftOffset, vp.TopOffset, vp.Width, vp.Height, vp.MinDepth, vp.MaxDepth );
        SDL_SetGPUViewport( render_pass, &viewport );

        uint32_t draw_calls = 0;

        if ( m_QuadGPUBatch->size() > 0 ) {
            SDL_BindGPUGraphicsPipeline( render_pass, m_QuadPipeline );
            for ( const auto& batch_data : m_QuadGPUBatch->get_batchlist() ) {
                SDL_BindGPUVertexStorageBuffers( render_pass, 1, &batch_data.GPUBuffer, 1 );
                SDL_DrawGPUPrimitives( render_pass, batch_data.Count * 6, 1, 0, 0 );
                ++draw_calls;
            }
        }

        if ( m_LineGPUBatch->size() > 0 ) {
            SDL_BindGPUGraphicsPipeline( render_pass, m_LinePipeline );
            for ( const auto& batch_data : m_LineGPUBatch->get_batchlist() ) {
                SDL_BindGPUVertexStorageBuffers( render_pass, 1, &batch_data.GPUBuffer, 1 );
                SDL_DrawGPUPrimitives( render_pass, batch_data.Count * 6, 1, 0, 0 );
                ++draw_calls;
            }
        }

        if ( m_CircleGPUBatch->size() > 0 ) {
            SDL_BindGPUGraphicsPipeline( render_pass, m_CirclePipeline );
            for ( const auto& batch_data : m_CircleGPUBatch->get_batchlist() ) {
                SDL_BindGPUVertexStorageBuffers( render_pass, 1, &batch_data.GPUBuffer, 1 );
                SDL_DrawGPUPrimitives( render_pass, batch_data.Count * 6, 1, 0, 0 );
                ++draw_calls;
            }
        }

        return draw_calls;
    }

    void Primitive2DPipeline::sort_quad_commands( const QuadCommandList& quad_command_list )
    {
        m_SortedQuadCommands.clear();

        if ( quad_command_list.size() > m_SortedQuadCommands.size() )
            m_SortedQuadCommands.reserve( quad_command_list.size() );

        for ( size_t i = 0; i < quad_command_list.size(); ++i ) {
            m_SortedQuadCommands.push_back( &quad_command_list[ i ] );
        }

        std::sort( m_SortedQuadCommands.begin(), m_SortedQuadCommands.end(), []( const QuadCommand* a, const QuadCommand* b ) {
            if ( a->ContextIndex > b->ContextIndex )
                return true;

            if ( a->Depth > b->Depth )
                return true;
            return false;
        } );
    }

    void Primitive2DPipeline::sort_line_commands( const LineCommandList& line_command_list )
    {
        m_SortedLineCommands.clear();

        if ( line_command_list.size() > m_SortedLineCommands.size() )
            m_SortedLineCommands.reserve( line_command_list.size() );

        for ( size_t i = 0; i < line_command_list.size(); ++i ) {
            m_SortedLineCommands.push_back( &line_command_list[ i ] );
        }

        std::sort( m_SortedLineCommands.begin(), m_SortedLineCommands.end(), []( const LineCommand* a, const LineCommand* b ) {
            if ( a->ContextIndex > b->ContextIndex )
                return true;

            if ( a->Depth > b->Depth )
                return true;

            return false;
        } );
    }

    void Primitive2DPipeline::sort_circle_commands( const CircleCommandList& circle_command_list )
    {
        m_SortedCircleCommands.clear();

        if ( circle_command_list.size() > m_SortedCircleCommands.size() )
            m_SortedCircleCommands.reserve( circle_command_list.size() );

        for ( size_t i = 0; i < circle_command_list.size(); ++i ) {
            m_SortedCircleCommands.push_back( &circle_command_list[ i ] );
        }

        std::sort( m_SortedCircleCommands.begin(), m_SortedCircleCommands.end(), []( const CircleCommand* a, const CircleCommand* b ) {
            if ( a->ContextIndex > b->ContextIndex )
                return true;

            if ( a->Depth > b->Depth )
                return true;

            return false;
        } );
    }

    Result Primitive2DPipeline::load_quad_pipeline( SDL_Window* sdl_window, AssetRepository<Shader>* shader_repo )
    {
        // load shaders
        auto vertexShaderAsset = shader_repo->require_asset( "QuadBatch.vert" );
        if ( vertexShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Vertex Shader not found: {}", "QuadBatch.vert" );
            return Result::InitializationError;
        }

        auto fragmentShaderAsset = shader_repo->require_asset( "Color.frag" );
        if ( fragmentShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Fragment Shader not found: {}", "Color.frag" );
            return Result::InitializationError;
        }

        AssetView<Shader>& vertexShader   = vertexShaderAsset.value();
        AssetView<Shader>& fragmentShader = fragmentShaderAsset.value();

        // Create the pipeline
        SDL_GPUColorTargetDescription colorTargets[ 1 ]     = {};
        colorTargets[ 0 ].format                            = SDL_GetGPUSwapchainTextureFormat( m_Device, sdl_window );
        colorTargets[ 0 ].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
        colorTargets[ 0 ].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
        colorTargets[ 0 ].blend_state.enable_blend          = true;

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo     = {};
        pipelineCreateInfo.vertex_shader                         = vertexShader.get()->get_sdlshader();
        pipelineCreateInfo.fragment_shader                       = fragmentShader.get()->get_sdlshader();
        pipelineCreateInfo.primitive_type                        = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineCreateInfo.target_info.color_target_descriptions = colorTargets;
        pipelineCreateInfo.target_info.num_color_targets         = 1;

        pipelineCreateInfo.target_info.depth_stencil_format     = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
        pipelineCreateInfo.target_info.has_depth_stencil_target = true;

        pipelineCreateInfo.depth_stencil_state.compare_op          = SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
        pipelineCreateInfo.depth_stencil_state.enable_depth_test   = true;
        pipelineCreateInfo.depth_stencil_state.enable_depth_write  = true;
        pipelineCreateInfo.depth_stencil_state.enable_stencil_test = false;
        pipelineCreateInfo.depth_stencil_state.write_mask          = 0xFF;

        m_QuadPipeline = SDL_CreateGPUGraphicsPipeline( m_Device, &pipelineCreateInfo );
        if ( m_QuadPipeline == nullptr ) {
            IE_LOG_ERROR( "Failed to create pipeline!" );
            return Result::InitializationError;
        }

        m_QuadGPUBatch = GPUBatchStorageBuffer<QuadStorageBufferLayout, BatchData>::create( m_Device, QuadBatchSize );
        return Result::Success;
    }

    Result Primitive2DPipeline::load_line_pipeline( SDL_Window* sdl_window, AssetRepository<Shader>* shader_repo )
    {
        // load shaders
        auto vertexShaderAsset = shader_repo->require_asset( "LineBatch.vert" );
        if ( vertexShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Vertex Shader not found: {}", "LineBatch.vert" );
            return Result::InitializationError;
        }

        auto fragmentShaderAsset = shader_repo->require_asset( "Line.frag" );
        if ( fragmentShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Fragment Shader not found: {}", "Line.frag" );
            return Result::InitializationError;
        }

        AssetView<Shader>& vertexShader   = vertexShaderAsset.value();
        AssetView<Shader>& fragmentShader = fragmentShaderAsset.value();

        // Create the pipeline
        SDL_GPUColorTargetDescription colorTargets[ 1 ]     = {};
        colorTargets[ 0 ].format                            = SDL_GetGPUSwapchainTextureFormat( m_Device, sdl_window );
        colorTargets[ 0 ].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
        colorTargets[ 0 ].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
        colorTargets[ 0 ].blend_state.enable_blend          = true;

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo     = {};
        pipelineCreateInfo.vertex_shader                         = vertexShader.get()->get_sdlshader();
        pipelineCreateInfo.fragment_shader                       = fragmentShader.get()->get_sdlshader();
        pipelineCreateInfo.primitive_type                        = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineCreateInfo.target_info.color_target_descriptions = colorTargets;
        pipelineCreateInfo.target_info.num_color_targets         = 1;

        pipelineCreateInfo.target_info.depth_stencil_format     = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
        pipelineCreateInfo.target_info.has_depth_stencil_target = true;

        pipelineCreateInfo.depth_stencil_state.compare_op          = SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
        pipelineCreateInfo.depth_stencil_state.enable_depth_test   = true;
        pipelineCreateInfo.depth_stencil_state.enable_depth_write  = true;
        pipelineCreateInfo.depth_stencil_state.enable_stencil_test = false;
        pipelineCreateInfo.depth_stencil_state.write_mask          = 0xFF;

        m_LinePipeline = SDL_CreateGPUGraphicsPipeline( m_Device, &pipelineCreateInfo );
        if ( m_LinePipeline == nullptr ) {
            IE_LOG_ERROR( "Failed to create pipeline!" );
            return Result::InitializationError;
        }

        m_LineGPUBatch = GPUBatchStorageBuffer<LineStorageBufferLayout, BatchData>::create( m_Device, LineBatchSize );
        return Result::Success;
    }

    Result Primitive2DPipeline::load_circle_pipeline( SDL_Window* sdl_window, AssetRepository<Shader>* shader_repo )
    {
        // load shaders
        auto vertexShaderAsset = shader_repo->require_asset( "CircleBatch.vert" );
        if ( vertexShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Vertex Shader not found: {}", "CircleBatch.vert" );
            return Result::InitializationError;
        }

        auto fragmentShaderAsset = shader_repo->require_asset( "Circle.frag" );
        if ( fragmentShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Fragment Shader not found: {}", "Circle.frag" );
            return Result::InitializationError;
        }

        AssetView<Shader>& vertexShader   = vertexShaderAsset.value();
        AssetView<Shader>& fragmentShader = fragmentShaderAsset.value();

        // Create the pipeline
        SDL_GPUColorTargetDescription colorTargets[ 1 ]     = {};
        colorTargets[ 0 ].format                            = SDL_GetGPUSwapchainTextureFormat( m_Device, sdl_window );
        colorTargets[ 0 ].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
        colorTargets[ 0 ].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        colorTargets[ 0 ].blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
        colorTargets[ 0 ].blend_state.enable_blend          = true;

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo     = {};
        pipelineCreateInfo.vertex_shader                         = vertexShader.get()->get_sdlshader();
        pipelineCreateInfo.fragment_shader                       = fragmentShader.get()->get_sdlshader();
        pipelineCreateInfo.primitive_type                        = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipelineCreateInfo.target_info.color_target_descriptions = colorTargets;
        pipelineCreateInfo.target_info.num_color_targets         = 1;

        pipelineCreateInfo.target_info.depth_stencil_format     = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
        pipelineCreateInfo.target_info.has_depth_stencil_target = true;

        pipelineCreateInfo.depth_stencil_state.compare_op          = SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
        pipelineCreateInfo.depth_stencil_state.enable_depth_test   = true;
        pipelineCreateInfo.depth_stencil_state.enable_depth_write  = true;
        pipelineCreateInfo.depth_stencil_state.enable_stencil_test = false;
        pipelineCreateInfo.depth_stencil_state.write_mask          = 0xFF;

        m_CirclePipeline = SDL_CreateGPUGraphicsPipeline( m_Device, &pipelineCreateInfo );
        if ( m_CirclePipeline == nullptr ) {
            IE_LOG_ERROR( "Failed to create pipeline!" );
            return Result::InitializationError;
        }

        m_CircleGPUBatch = GPUBatchStorageBuffer<CircleStorageBufferLayout, BatchData>::create( m_Device, CircleBatchSize );
        return Result::Success;
    }
}    // namespace InnoEngine
