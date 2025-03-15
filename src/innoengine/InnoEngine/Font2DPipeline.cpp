#include "InnoEngine/iepch.h"
#include "InnoEngine/Font2DPipeline.h"

#include "InnoEngine/Renderer.h"
#include "InnoEngine/AssetManager.h"
#include "InnoEngine/Shader.h"

#include "InnoEngine/Window.h"

#include "InnoEngine/RenderCommandBuffer.h"

#include "InnoEngine/Font.h"

#include "InnoEngine/MSDFData.h"

namespace InnoEngine
{
    static constexpr uint32_t FontBatchSizeMax = 10000;

    Font2DPipeline::~Font2DPipeline()
    {
        if ( m_device != nullptr ) {
            if ( m_transferBuffer ) {
                SDL_ReleaseGPUTransferBuffer( m_device, m_transferBuffer );
                m_transferBuffer = nullptr;
            }

            for ( auto gpubuffer : m_gpuBuffer ) {
                SDL_ReleaseGPUBuffer( m_device, gpubuffer );
            }
            m_gpuBuffer.clear();
            m_batches.clear();

            if ( m_fontSampler ) {
                SDL_ReleaseGPUSampler( m_device, m_fontSampler );
                m_fontSampler = nullptr;
            }

            if ( m_pipeline ) {
                SDL_ReleaseGPUGraphicsPipeline( m_device, m_pipeline );
                m_pipeline = nullptr;
            }
        }
    }

    Result Font2DPipeline::initialize( GPURenderer* renderer, AssetManager* assetmanager )
    {
        IE_ASSERT( renderer != nullptr && renderer->has_window() && assetmanager != nullptr );

        if ( m_initialized ) {
            IE_LOG_WARNING( "Pipeline already initialized!" );
            return Result::AlreadyInitialized;
        }

        m_device           = renderer->get_gpudevice();
        SDL_Window* window = renderer->get_window()->get_sdlwindow();

        auto shaderRepo = assetmanager->get_repository<Shader>();
        IE_ASSERT( shaderRepo != nullptr );

        // load shaders
        auto vertexShaderAsset = shaderRepo->require_asset( "SpriteBatch.vert" );
        if ( vertexShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Vertex Shader not found!" );
            return Result::InitializationError;
        }

        auto fragmentShaderAsset = shaderRepo->require_asset( "MSDFFont.frag" );
        if ( fragmentShaderAsset.has_value() == false ) {
            IE_LOG_ERROR( "Fragment Shader not found!" );
            return Result::InitializationError;
        }

        AssetView<Shader>& vertexShader   = vertexShaderAsset.value();
        AssetView<Shader>& fragmentShader = fragmentShaderAsset.value();

        // Create the pipeline
        SDL_GPUColorTargetDescription colorTargets[ 1 ]     = {};
        colorTargets[ 0 ].format                            = SDL_GetGPUSwapchainTextureFormat( m_device, window );
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

        m_pipeline = SDL_CreateGPUGraphicsPipeline( m_device, &pipelineCreateInfo );
        if ( m_pipeline == nullptr ) {
            IE_LOG_ERROR( "Failed to create pipeline!" );
            return Result::InitializationError;
        }

        SDL_GPUTransferBufferCreateInfo tbufferCreateInfo = {};
        tbufferCreateInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbufferCreateInfo.size                            = MaxBatchSize * sizeof( Command::VertexUniform );

        m_transferBuffer = SDL_CreateGPUTransferBuffer( m_device, &tbufferCreateInfo );
        if ( m_transferBuffer == nullptr ) {
            IE_LOG_ERROR( "Failed to create GPUTransferBuffer!" );
            return Result::InitializationError;
        }

        SDL_GPUSamplerCreateInfo sampler_create_info = {};
        sampler_create_info.min_filter               = SDL_GPU_FILTER_NEAREST;
        sampler_create_info.mag_filter               = SDL_GPU_FILTER_NEAREST;
        sampler_create_info.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        sampler_create_info.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler_create_info.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampler_create_info.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        m_fontSampler = SDL_CreateGPUSampler( m_device, &sampler_create_info );
        if ( m_fontSampler == nullptr ) {
            IE_LOG_ERROR( "Failed to create GPUSampler!" );
            return Result::InitializationError;
        }

        m_batches.reserve( 100 );
        m_initialized = true;
        return Result::Success;
    }

    void Font2DPipeline::prepare_render( const CommandList& command_list, const FontList& font_list, const StringArena& string_buffer )
    {
        IE_ASSERT( m_device != nullptr );
        sort_commands( command_list );

        SDL_GPUCommandBuffer* gpu_copy_cmd_buf = SDL_AcquireGPUCommandBuffer( m_device );
        if ( gpu_copy_cmd_buf == nullptr ) {
            IE_LOG_ERROR( "AcquireGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }

        (void)font_list;

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass( gpu_copy_cmd_buf );

        Command::VertexUniform* uniform_data    = nullptr;
        FrameBufferIndex        current_texture = -1;

        BatchData* current_batch = nullptr;

        Ref<Font>     font      = nullptr;
        Ref<MSDFData> msdf_data = nullptr;

        float texel_width  = 0.0f;
        float texel_height = 0.0f;

        double spaceGlyphAdvance = 0.0f;
        double fsScale           = 0.0f;

        // each command represents one string
        for ( const Command* command : m_sortedCommands ) {

            const auto& fontGeometry = msdf_data->FontGeo;
            const auto& metrics      = fontGeometry.getMetrics();

            // check if have to switch to a new batch
            // reasons might be: change in texture, batch is full
            if ( current_batch == nullptr || current_texture != command->font_fbidx || current_batch->count >= FontBatchSizeMax ) {
                // upload the last batches data only when its not the first batch
                if ( current_batch != nullptr ) {
                    SDL_UnmapGPUTransferBuffer( m_device, m_transferBuffer );
                    SDL_GPUTransferBufferLocation tranferBufferLocation { .transfer_buffer = m_transferBuffer, .offset = 0 };
                    SDL_GPUBufferRegion           bufferRegion { .buffer = get_gpubuffer_by_index( current_batch->buffer_index ),
                                                                 .offset = 0,
                                                                 .size   = static_cast<uint32_t>( current_batch->count * sizeof( Command::VertexUniform ) ) };
                    SDL_UploadToGPUBuffer( copy_pass, &tranferBufferLocation, &bufferRegion, true );
                    uniform_data = nullptr;
                }

                current_batch             = add_batch();
                current_batch->font_fbidx = command->font_fbidx;

                // font change?
                if ( current_texture != command->font_fbidx ) {
                    font      = font_list[ command->font_fbidx ];
                    msdf_data = font->get_msdf_data();

                    texel_width  = 1.0f / font->get_atlas_texture()->width();
                    texel_height = 1.0f / font->get_atlas_texture()->height();

                    spaceGlyphAdvance = fontGeometry.getGlyph( ' ' )->getAdvance();
                    fsScale           = 1.0 / ( metrics.ascenderY - metrics.descenderY );

                    current_texture = command->font_fbidx;
                }

                uniform_data = static_cast<Command::VertexUniform*>( SDL_MapGPUTransferBuffer( m_device, m_transferBuffer, true ) );
            }

            double x = 0.0;
            double y = 0.0;

            // now retrieve the string back and iterate it
            const char* text = string_buffer.get_string( command->string_arena_index );

            for ( uint32_t i = 0; i < command->string_size; ++i ) {
                char character = text[ i ];

                IE_ASSERT( character != '\0' );

                if ( character == '\n' ) {
                    x = 0.0;
                    y -= fsScale * metrics.lineHeight;
                    continue;
                }

                if ( character == ' ' ) {
                    double advance = spaceGlyphAdvance;
                    if ( i < command->string_size - 1 ) {
                        char nextCharacter = text[ i + 1 ];
                        fontGeometry.getAdvance( advance, character, nextCharacter );
                    }

                    x += fsScale * advance;
                    continue;
                }

                if ( character == '\t' ) {
                    x += 4.0 * ( fsScale * spaceGlyphAdvance );
                    continue;
                }

                const msdf_atlas::GlyphGeometry* glyph = fontGeometry.getGlyph( character );
                if ( !glyph )
                    glyph = fontGeometry.getGlyph( '?' );
                if ( !glyph )
                    return;

                Font2DPipeline::Command::VertexUniform& glyph_uniform = uniform_data[ current_batch->count ];

                double al, ab, ar, at;
                glyph->getQuadAtlasBounds( al, ab, ar, at );

                glyph_uniform.source.x = static_cast<float>( al * texel_width );
                glyph_uniform.source.y = static_cast<float>( at * texel_height );
                glyph_uniform.source.z = static_cast<float>( ar * texel_width );
                glyph_uniform.source.w = static_cast<float>( ab * texel_height );

                double pl, pb, pr, pt;
                glyph->getQuadPlaneBounds( pl, pb, pr, pt );

                glyph_uniform.x = static_cast<float>( pl * fsScale );
                glyph_uniform.y = static_cast<float>( pt * fsScale );

                glyph_uniform.width  = static_cast<float>( pr * fsScale ) - glyph_uniform.x;
                glyph_uniform.height = static_cast<float>( pb * fsScale ) - glyph_uniform.y;

                glyph_uniform.x += static_cast<float>( x );
                glyph_uniform.y += static_cast<float>( y );

                glyph_uniform.color = command->info.color;
                glyph_uniform.z     = command->info.z;

                if ( i < command->string_size - 1 ) {
                    double advance       = glyph->getAdvance();
                    char   nextCharacter = text[ i + 1 ];
                    fontGeometry.getAdvance( advance, character, nextCharacter );

                    x += fsScale * advance;
                }

                current_batch->count++;
            }
        }

        // unmap and upload last batch data
        if ( current_batch && current_batch->count > 0 ) {
            SDL_UnmapGPUTransferBuffer( m_device, m_transferBuffer );
            SDL_GPUTransferBufferLocation tranferBufferLocation { .transfer_buffer = m_transferBuffer, .offset = 0 };
            SDL_GPUBufferRegion           bufferRegion { .buffer = get_gpubuffer_by_index( current_batch->buffer_index ),
                                                         .offset = 0,
                                                         .size   = static_cast<uint32_t>( current_batch->count * sizeof( Command::VertexUniform ) ) };
            SDL_UploadToGPUBuffer( copy_pass, &tranferBufferLocation, &bufferRegion, true );
        }

        SDL_EndGPUCopyPass( copy_pass );

        if ( SDL_SubmitGPUCommandBuffer( gpu_copy_cmd_buf ) == false ) {
            IE_LOG_ERROR( "SDL_SubmitGPUCommandBuffer failed: {}", SDL_GetError() );
            return;
        }
    }

    uint32_t Font2DPipeline::swapchain_render( const DXSM::Matrix& view_projection, const CommandList& command_list, const FontList& font_list, SDL_GPUCommandBuffer* cmdbuf, SDL_GPURenderPass* renderPass )
    {
        (void)view_projection;
        (void)command_list;
        (void)font_list;
        (void)cmdbuf;
        (void)renderPass;
        return 0;
    }

    Font2DPipeline::BatchData* Font2DPipeline::add_batch()
    {
        BatchData& newbatch   = m_batches.emplace_back();
        newbatch.buffer_index = static_cast<uint16_t>( find_free_gpubuffer() );
        newbatch.count        = 0;
        return &newbatch;
    }

    void Font2DPipeline::clear_batches()
    {
        m_batches.clear();
        m_gpuBuffer_used = 0;
    }

    uint32_t Font2DPipeline::find_free_gpubuffer()
    {
        if ( m_gpuBuffer_used < m_gpuBuffer.size() )
            return m_gpuBuffer_used++;

        auto&                   buffer     = m_gpuBuffer.emplace_back();
        SDL_GPUBufferCreateInfo createInfo = {};
        createInfo.usage                   = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
        createInfo.size                    = MaxBatchSize * sizeof( Command::VertexUniform );

        buffer = SDL_CreateGPUBuffer( m_device, &createInfo );
        if ( buffer == nullptr ) {
            // TODO: this needs to be handled better
            IE_LOG_ERROR( "SDL_CreateGPUBuffer failed : {0}", SDL_GetError() );
            return 0;
        }
        return m_gpuBuffer_used++;
    }

    SDL_GPUBuffer* Font2DPipeline::get_gpubuffer_by_index( uint32_t index ) const
    {
        IE_ASSERT( index < m_gpuBuffer_used );
        return m_gpuBuffer[ index ];
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

            if ( a->info.z > b->info.z )
                return true;

            return false;
        } );
    }
}    // namespace InnoEngine
