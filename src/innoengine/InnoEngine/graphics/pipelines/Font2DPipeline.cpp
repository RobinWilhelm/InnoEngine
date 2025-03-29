#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/pipelines/Font2DPipeline.h"

#include "InnoEngine/graphics/Renderer.h"
#include "InnoEngine/AssetManager.h"
#include "InnoEngine/graphics/Shader.h"
#include "InnoEngine/graphics/Window.h"
#include "InnoEngine/graphics/RenderCommandBuffer.h"

#include "InnoEngine/graphics/Font.h"

#include "InnoEngine/graphics/MSDFData.h"

namespace InnoEngine
{
    Font2DPipeline::~Font2DPipeline()
    {
        if ( m_Device != nullptr ) {
            if ( m_FontSampler ) {
                SDL_ReleaseGPUSampler( m_Device, m_FontSampler );
                m_FontSampler = nullptr;
            }

            if ( m_Pipeline ) {
                SDL_ReleaseGPUGraphicsPipeline( m_Device, m_Pipeline );
                m_Pipeline = nullptr;
            }
        }
    }

    Result Font2DPipeline::initialize( GPURenderer* renderer, AssetManager* assetmanager )
    {
        IE_ASSERT( renderer != nullptr && renderer->has_window() && assetmanager != nullptr );

        if ( m_Initialized ) {
            IE_LOG_WARNING( "Pipeline already initialized!" );
            return Result::AlreadyInitialized;
        }

        m_Device           = renderer->get_gpudevice();
        SDL_Window* window = renderer->get_window()->get_sdlwindow();

        auto shaderRepo = assetmanager->get_repository<Shader>();
        IE_ASSERT( shaderRepo != nullptr );

        // load shaders
        auto vertexShaderAsset = shaderRepo->require_asset( "MSDFText2DBatch.vert" );
        if ( vertexShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Vertex Shader not found!" );
            return Result::InitializationError;
        }

        auto fragmentShaderAsset = shaderRepo->require_asset( "MSDFText2D.frag" );
        if ( fragmentShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Fragment Shader not found!" );
            return Result::InitializationError;
        }

        AssetView<Shader>& vertexShader   = vertexShaderAsset.value();
        AssetView<Shader>& fragmentShader = fragmentShaderAsset.value();

        // Create the pipeline
        SDL_GPUColorTargetDescription colorTargets[ 1 ]     = {};
        colorTargets[ 0 ].format                            = SDL_GetGPUSwapchainTextureFormat( m_Device, window );
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

        m_Pipeline = SDL_CreateGPUGraphicsPipeline( m_Device, &pipelineCreateInfo );
        if ( m_Pipeline == nullptr ) {
            IE_LOG_ERROR( "Failed to create pipeline!" );
            return Result::InitializationError;
        }

        SDL_GPUSamplerCreateInfo sampler_create_info = {};
        sampler_create_info.min_filter               = SDL_GPU_FILTER_LINEAR;
        sampler_create_info.mag_filter               = SDL_GPU_FILTER_LINEAR;
        sampler_create_info.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        sampler_create_info.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler_create_info.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler_create_info.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        m_FontSampler = SDL_CreateGPUSampler( m_Device, &sampler_create_info );
        if ( m_FontSampler == nullptr ) {
            IE_LOG_ERROR( "Failed to create GPUSampler!" );
            return Result::InitializationError;
        }

        m_GPUBatch = GPUBatchStorageBuffer<StructuredBufferLayout, BatchData>::create( m_Device, MaxBatchSize );

        m_Initialized = true;
        return Result::Success;
    }

    void Font2DPipeline::prepare_render( const CommandList& command_list, const FontList& font_list, const StringArena& string_buffer )
    {
        IE_ASSERT( m_Device != nullptr );
        sort_commands( command_list );
        m_GPUBatch->clear();

        if ( command_list.size() == 0 )
            return;
        SDL_GPUCommandBuffer* gpu_copy_cmd_buf = SDL_AcquireGPUCommandBuffer( m_Device );
        if ( gpu_copy_cmd_buf == nullptr ) {
            IE_LOG_ERROR( "AcquireGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass( gpu_copy_cmd_buf );

        // current font data
        Ref<Font>                       font                = nullptr;
        Ref<MSDFData>                   msdf_data           = nullptr;
        float                           texel_width         = 0.0f;
        float                           texel_height        = 0.0f;
        double                          space_glyph_advance = 0.0f;
        double                          scale               = 0.0f;
        const msdf_atlas::FontGeometry* font_geometry       = nullptr;
        const msdfgen::FontMetrics*     metrics             = nullptr;

        // each command represents one string
        BatchData* current = nullptr;
        for ( const Command* command : m_SortedCommands ) {

            // check if have to switch to a new batch
            // reasons might be: change in texture, batch is full
            if ( m_GPUBatch->current_batch_full() ||
                 current == nullptr ||
                 current->ContextIndex != command->ContextIndex ||
                 current->FontFBIndex != command->FontFBIndex ||
                 m_GPUBatch->get_current_batch_remaining_size() < command->StringLength ) {

                current               = m_GPUBatch->upload_and_add_batch( copy_pass );
                current->ContextIndex = command->ContextIndex;
                current->FontFBIndex  = command->FontFBIndex;

                // font change?
                font          = font_list[ command->FontFBIndex ];
                msdf_data     = font->get_msdf_data();
                font_geometry = &msdf_data->FontGeo;
                metrics       = &font_geometry->getMetrics();

                texel_width  = 1.0f / font->get_atlas_texture()->get_specs().Width;
                texel_height = 1.0f / font->get_atlas_texture()->get_specs().Width;

                space_glyph_advance = msdf_data->get_glyph( ' ' )->getAdvance();
                scale               = 1.0 / ( metrics->ascenderY - metrics->descenderY ) * command->FontSize;
            }

            double x = static_cast<double>( command->Position.x );
            double y = static_cast<double>( command->Position.y );

            // now retrieve the string back and iterate it
            const char* text = string_buffer.get_string( command->StringIndex );

            for ( uint32_t i = 0; i < command->StringLength; ++i ) {
                char character = text[ i ];

                IE_ASSERT( character != '\0' );

                if ( character == '\n' ) {
                    x = command->Position.x;
                    y += scale * metrics->lineHeight;
                    continue;
                }

                if ( character == ' ' ) {
                    double advance = space_glyph_advance;
                    if ( i < command->StringLength - 1 ) {
                        char nextCharacter = text[ i + 1 ];
                        msdf_data->get_advance( advance, character, nextCharacter );
                    }

                    x += scale * advance;
                    continue;
                }

                if ( character == '\t' ) {
                    x += 4.0 * ( scale * space_glyph_advance );
                    continue;
                }

                const msdf_atlas::GlyphGeometry* glyph = msdf_data->get_glyph( character );
                if ( !glyph )
                    glyph = msdf_data->get_glyph( '?' );
                if ( !glyph )
                    continue;

                StructuredBufferLayout* buffer_data = m_GPUBatch->next_data();
                // remember that the atlas y grows in bottom-up and our renderer expects it to grow top-down
                double                  al, ab, ar, at;
                glyph->getQuadAtlasBounds( al, ab, ar, at );
                buffer_data->SourceRect.x = static_cast<float>( al * texel_width );
                buffer_data->SourceRect.y = static_cast<float>( ab * texel_height );
                buffer_data->SourceRect.z = static_cast<float>( ar * texel_width );
                buffer_data->SourceRect.w = static_cast<float>( at * texel_height );

                double pl, pb, pr, pt;
                glyph->getQuadPlaneBounds( pl, pb, pr, pt );
                buffer_data->Position.x = static_cast<float>( x + pl * scale );
                buffer_data->Position.y = static_cast<float>( y + ( pb * scale ) * -1 );
                buffer_data->Size.x     = static_cast<float>( ( pr - pl ) * scale );
                buffer_data->Size.y     = static_cast<float>( ( ( pb - pt ) * scale ) );

                buffer_data->ForegroundColor = command->ForegroundColor;
                buffer_data->Depth           = command->Depth;
                buffer_data->ContextIndex    = command->ContextIndex;

                if ( i < command->StringLength - 1 ) {
                    double advance       = glyph->getAdvance();
                    char   nextCharacter = text[ i + 1 ];
                    msdf_data->get_advance( advance, character, nextCharacter );
                    // fontGeometry->getAdvance(advance, character, nextCharacter);
                    x += scale * advance;
                }
            }
        }
        m_GPUBatch->upload_last( copy_pass );

        SDL_EndGPUCopyPass( copy_pass );

        if ( SDL_SubmitGPUCommandBuffer( gpu_copy_cmd_buf ) == false ) {
            IE_LOG_ERROR( "SDL_SubmitGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }
    }

    uint32_t Font2DPipeline::swapchain_render( const RenderContextFrameData& render_ctx_data, const FontList& font_list, SDL_GPURenderPass* render_pass )
    {
        IE_ASSERT( m_Device != nullptr );
        IE_ASSERT( render_pass != nullptr );

        SDL_BindGPUGraphicsPipeline( render_pass, m_Pipeline );
        SDL_BindGPUVertexBuffers( render_pass, 0, nullptr, 0 );
        SDL_SetGPUViewport( render_pass, &render_ctx_data.Viewport );

        RenderCommandBufferIndexType current_font = InvalidRenderCommandBufferIndex;

        uint32_t draw_calls = 0;
        for ( const auto& batch_data : m_GPUBatch->get_batchlist() ) {
            if ( batch_data.CustomData.FontFBIndex != current_font ) {
                SDL_GPUTextureSamplerBinding texture_sampler_binding = {};
                texture_sampler_binding.sampler                      = m_FontSampler;
                texture_sampler_binding.texture                      = font_list[ batch_data.CustomData.FontFBIndex ]->get_atlas_texture()->get_sdltexture();
                SDL_BindGPUFragmentSamplers( render_pass, 0, &texture_sampler_binding, 1 );
                current_font = batch_data.CustomData.FontFBIndex;
            }

            SDL_BindGPUVertexStorageBuffers( render_pass, 1, &batch_data.GPUBuffer, 1 );
            SDL_DrawGPUPrimitives( render_pass, batch_data.Count * 6, 1, 0, 0 );
            ++draw_calls;
        }
        return draw_calls;
    }

    void Font2DPipeline::sort_commands( const CommandList& command_list )
    {
        m_SortedCommands.clear();

        if ( command_list.size() > m_SortedCommands.size() )
            m_SortedCommands.reserve( command_list.size() );

        for ( size_t i = 0; i < command_list.size(); ++i ) {
            m_SortedCommands.push_back( &command_list[ i ] );
        }

        std::sort( m_SortedCommands.begin(), m_SortedCommands.end(), []( const Command* a, const Command* b ) {
            if ( a->ContextIndex > b->ContextIndex )
                return true;

            if ( a->FontFBIndex > b->FontFBIndex )
                return true;

            if ( a->Depth > b->Depth )
                return true;

            return false;
        } );
    }
}    // namespace InnoEngine
