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
            if ( m_fontSampler ) {
                SDL_ReleaseGPUSampler( m_Device, m_fontSampler );
                m_fontSampler = nullptr;
            }

            if ( m_pipeline ) {
                SDL_ReleaseGPUGraphicsPipeline( m_Device, m_pipeline );
                m_pipeline = nullptr;
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

        m_pipeline = SDL_CreateGPUGraphicsPipeline( m_Device, &pipelineCreateInfo );
        if ( m_pipeline == nullptr ) {
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

        m_fontSampler = SDL_CreateGPUSampler( m_Device, &sampler_create_info );
        if ( m_fontSampler == nullptr ) {
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

        if ( command_list.size() == 0 )
            return;

        sort_commands( command_list );
        m_GPUBatch->clear();

        SDL_GPUCommandBuffer* gpu_copy_cmd_buf = SDL_AcquireGPUCommandBuffer( m_Device );
        if ( gpu_copy_cmd_buf == nullptr ) {
            IE_LOG_ERROR( "AcquireGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass( gpu_copy_cmd_buf );

        // current font data
        Ref<Font>                       font              = nullptr;
        Ref<MSDFData>                   msdf_data         = nullptr;
        float                           texel_width       = 0.0f;
        float                           texel_height      = 0.0f;
        double                          spaceGlyphAdvance = 0.0f;
        double                          scale             = 0.0f;
        const msdf_atlas::FontGeometry* font_geometry     = nullptr;
        const msdfgen::FontMetrics*     metrics           = nullptr;

        // each command represents one string
        FrameBufferIndex current_texture = -1;
        for ( const Command* command : m_sortedCommands ) {

            // check if have to switch to a new batch
            // reasons might be: change in texture, batch is full
            if ( m_GPUBatch->current_batch_full() || current_texture != command->font_fbidx ) {
                BatchData& batch_data = m_GPUBatch->upload_and_add_batch( copy_pass );
                batch_data.font_index = command->font_fbidx;

                // font change?
                if ( current_texture != command->font_fbidx ) {
                    font          = font_list[ command->font_fbidx ];
                    msdf_data     = font->get_msdf_data();
                    font_geometry = &msdf_data->FontGeo;
                    metrics       = &font_geometry->getMetrics();

                    texel_width  = 1.0f / font->get_atlas_texture()->width();
                    texel_height = 1.0f / font->get_atlas_texture()->height();

                    spaceGlyphAdvance = msdf_data->get_glyph( ' ' )->getAdvance();
                    scale             = 1.0 / ( metrics->ascenderY - metrics->descenderY ) * command->font_size;

                    current_texture = command->font_fbidx;
                }
            }

            double x = static_cast<double>( command->x );
            double y = static_cast<double>( command->y );

            float screen_pix_width = font->calculate_screen_pix_range( static_cast<float>( command->font_size ) );

            // now retrieve the string back and iterate it
            const char* text = string_buffer.get_string( command->string_index );

            for ( uint32_t i = 0; i < command->string_size; ++i ) {
                char character = text[ i ];

                IE_ASSERT( character != '\0' );

                if ( character == '\n' ) {
                    x = command->x;
                    y += scale * metrics->lineHeight;
                    continue;
                }

                if ( character == ' ' ) {
                    double advance = spaceGlyphAdvance;
                    if ( i < command->string_size - 1 ) {
                        char nextCharacter = text[ i + 1 ];
                        msdf_data->get_advance( advance, character, nextCharacter );
                    }

                    x += scale * advance;
                    continue;
                }

                if ( character == '\t' ) {
                    x += 4.0 * ( scale * spaceGlyphAdvance );
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
                buffer_data->source.x = static_cast<float>( al * texel_width );
                buffer_data->source.y = static_cast<float>( ab * texel_height );
                buffer_data->source.z = static_cast<float>( ar * texel_width );
                buffer_data->source.w = static_cast<float>( at * texel_height );

                double pl, pb, pr, pt;
                glyph->getQuadPlaneBounds( pl, pb, pr, pt );
                buffer_data->destination.x = static_cast<float>( x + pl * scale );
                buffer_data->destination.y = static_cast<float>( y + ( pb * scale ) * -1 );
                buffer_data->destination.z = static_cast<float>( x + pr * scale );
                buffer_data->destination.w = static_cast<float>( y + ( pt * scale ) * -1 );

                buffer_data->color            = command->color;
                buffer_data->depth            = command->depth;
                buffer_data->screen_pix_width = screen_pix_width;

                if ( i < command->string_size - 1 ) {
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

    uint32_t Font2DPipeline::swapchain_render( const DXSM::Matrix& view_projection, const FontList& font_list, SDL_GPUCommandBuffer* gpu_cmd_buf, SDL_GPURenderPass* render_pass )
    {
        IE_ASSERT( m_Device != nullptr );
        IE_ASSERT( gpu_cmd_buf != nullptr && render_pass != nullptr );

        SDL_BindGPUGraphicsPipeline( render_pass, m_pipeline );
        SDL_PushGPUVertexUniformData( gpu_cmd_buf, 0, &view_projection, sizeof( DXSM::Matrix ) );
        SDL_BindGPUVertexBuffers( render_pass, 0, nullptr, 0 );

        uint32_t draw_calls = 0;
        for ( const auto& batch_data : m_GPUBatch->get_batchlist() ) {
            SDL_BindGPUVertexStorageBuffers( render_pass, 0, &batch_data.GPUBuffer, 1 );

            SDL_GPUTextureSamplerBinding texture_sampler_binding = {};
            texture_sampler_binding.sampler                      = m_fontSampler;
            texture_sampler_binding.texture                      = font_list[ batch_data.CustomData.font_index ]->get_atlas_texture()->get_sdltexture();
            SDL_BindGPUFragmentSamplers( render_pass, 0, &texture_sampler_binding, 1 );

            SDL_DrawGPUPrimitives( render_pass, batch_data.Count * 6, 1, 0, 0 );
            ++draw_calls;
        }
        return draw_calls;
    }

    void Font2DPipeline::sort_commands( const CommandList& command_list )
    {
        m_sortedCommands.clear();

        if ( command_list.size() > m_sortedCommands.size() )
            m_sortedCommands.resize( command_list.size() );

        for ( size_t i = 0; i < command_list.size(); ++i ) {
            m_sortedCommands[ i ] = &command_list[ i ];
        }

        std::sort( m_sortedCommands.begin(), m_sortedCommands.end(), []( const Command* a, const Command* b ) {
            if ( a->font_fbidx < b->font_fbidx )
                return true;

            if ( a->depth > b->depth )
                return true;

            return false;
        } );
    }
}    // namespace InnoEngine
