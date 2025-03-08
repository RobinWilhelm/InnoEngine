#include "iepch.h"
#include "Shader.h"

#include "CoreAPI.h"
#include "Renderer.h"

namespace InnoEngine
{
    ShaderFormatInfo Shader::ms_shaderFormat;

    Shader::~Shader()
    {
        if ( m_sdlShader != nullptr ) {
            SDL_ReleaseGPUShader( m_device, m_sdlShader );
            m_sdlShader = nullptr;
        }
    }

    SDL_GPUShader* Shader::get_sdlshader() const
    {
        return m_sdlShader;
    }

    bool Shader::load_from_file( const std::filesystem::path& full_path, std::string_view file_name )
    {
        // Auto-detect the shader stage from the file name for convenience
        if ( file_name.find( ".vert" ) != std::string::npos ) {
            m_stage = SDL_GPU_SHADERSTAGE_VERTEX;
        }
        else if ( file_name.find( ".frag" ) != std::string::npos ) {
            m_stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        }
        else {
            IE_LOG_ERROR( "Invalid shader stage!" );
            return false;
        }

        m_data = SDL_LoadFile( full_path.string().c_str(), &m_dataSize );
        if ( m_data == nullptr ) {
            IE_LOG_ERROR( "Failed to load shader from disk! %s", full_path.string() );
            return false;
        }

        return true;
    }

    std::filesystem::path Shader::build_path(const std::filesystem::path& folder, std::string_view file_name)
    {
        IE_ASSERT(ms_shaderFormat.Format != SDL_GPU_SHADERFORMAT_INVALID);
        return (folder / ms_shaderFormat.SubDirectory).append(file_name).concat(ms_shaderFormat.FileNameExtension);
    }

    bool Shader::create_device_ressources( GPUDeviceRef device, const ShaderCreateInfo& create_info )
    {
        IE_ASSERT( device != nullptr );
        IE_ASSERT( ms_shaderFormat.Format != SDL_GPU_SHADERFORMAT_INVALID );

        // check if already created
        if ( m_sdlShader != nullptr )
            return true;

        IE_ASSERT( m_dataSize != 0 );
        IE_ASSERT( m_data != nullptr );

        m_device = device;

        SDL_GPUShaderCreateInfo sdl_shadercreateinfo = {};
        sdl_shadercreateinfo.code_size               = m_dataSize;
        sdl_shadercreateinfo.code                    = static_cast<Uint8*>( m_data );
        sdl_shadercreateinfo.entrypoint              = ms_shaderFormat.EntryPoint.data();
        sdl_shadercreateinfo.format                  = ms_shaderFormat.Format;
        sdl_shadercreateinfo.stage                   = m_stage;
        sdl_shadercreateinfo.num_samplers            = create_info.SamplerCount;
        sdl_shadercreateinfo.num_storage_textures    = create_info.StorageTextureCount;
        sdl_shadercreateinfo.num_storage_buffers     = create_info.StorageBufferCount;
        sdl_shadercreateinfo.num_uniform_buffers     = create_info.UniformBufferCount;

        m_sdlShader = SDL_CreateGPUShader( m_device, &sdl_shadercreateinfo );
        if ( m_sdlShader == nullptr ) {
            IE_LOG_ERROR( "Failed to create shader!" );
            SDL_free( m_data );
            m_data = nullptr;
            return false;
        }

        SDL_free( m_data );
        m_data = nullptr;
        return true;
    }
}    // namespace InnoEngine
