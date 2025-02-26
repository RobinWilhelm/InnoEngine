#include "iepch.h"
#include "Shader.h"

#include "CoreAPI.h"
#include "Renderer.h"

Shader::~Shader()
{
    if ( m_sdlShader != nullptr )
        SDL_ReleaseGPUShader( m_renderer->get_gpudevice(), m_sdlShader );
}

SDL_GPUShader* Shader::get_sdlshader() const
{
    return m_sdlShader;
}

bool Shader::load_from_file( std::filesystem::path fullPath )
{
    // Auto-detect the shader stage from the file name for convenience
    std::string fileName = fullPath.filename().string();
    if ( fileName.find( ".vert" ) != std::string::npos ) {
        m_stage = SDL_GPU_SHADERSTAGE_VERTEX;
    } else if ( fileName.find( ".frag" ) != std::string::npos ) {
        m_stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    } else {
        IE_LOG_ERROR( "Invalid shader stage!" );
        return false;
    }

    m_data = SDL_LoadFile( fullPath.string().c_str(), &m_dataSize );
    if ( m_data == nullptr ) {
        IE_LOG_ERROR( "Failed to load shader from disk! %s", fullPath.string().c_str() );
        return false;
    }

    return true;
}

bool Shader::create_device_ressources( GPURenderer* pRenderer, const ShaderCreateInfo& createInfo )
{
    IE_ASSERT( pRenderer != nullptr );

    // check if already created
    if ( m_sdlShader != nullptr )
        return true;

    IE_ASSERT( m_dataSize != 0 );
    IE_ASSERT( m_data != nullptr );

    m_renderer        = pRenderer;
    auto shaderFormat = m_renderer->get_needed_shaderformat();

    SDL_GPUShaderCreateInfo sdl_shadercreateinfo = {};
    sdl_shadercreateinfo.code_size               = m_dataSize;
    sdl_shadercreateinfo.code                    = static_cast<Uint8*>( m_data );
    sdl_shadercreateinfo.entrypoint              = shaderFormat.EntryPoint.data();
    sdl_shadercreateinfo.format                  = shaderFormat.Format;
    sdl_shadercreateinfo.stage                   = m_stage;
    sdl_shadercreateinfo.num_samplers            = createInfo.SamplerCount;
    sdl_shadercreateinfo.num_storage_textures    = createInfo.StorageTextureCount;
    sdl_shadercreateinfo.num_storage_buffers     = createInfo.StorageBufferCount;
    sdl_shadercreateinfo.num_uniform_buffers     = createInfo.UniformBufferCount;

    m_sdlShader = SDL_CreateGPUShader( pRenderer->get_gpudevice(), &sdl_shadercreateinfo );
    if ( m_sdlShader == nullptr ) {
        IE_LOG_ERROR( "Failed to create shader!" );
        SDL_free( m_data );
        return false;
    }

    SDL_free( m_data );
    m_data = nullptr;
    return true;
}
