#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/Shader.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/graphics/Renderer.h"

#include "nlohmann/json.hpp"

namespace InnoEngine
{
    ShaderFormatInfo Shader::ms_shaderFormat;

    Shader::~Shader()
    {
        if ( m_sdlShader != nullptr ) {
            SDL_ReleaseGPUShader( m_Device, m_sdlShader );
            m_sdlShader = nullptr;
        }
    }

    SDL_GPUShader* Shader::get_sdlshader() const
    {
        return m_sdlShader;
    }

    Result Shader::load_asset( const std::filesystem::path& full_path )
    {
        IE_ASSERT( m_sdlShader == nullptr );

        // Auto-detect the shader stage from the file name for convenience
        const std::string file_name = full_path.filename().string();
        if ( file_name.find( ".vert" ) != std::string::npos ) {
            m_stage = SDL_GPU_SHADERSTAGE_VERTEX;
        }
        else if ( file_name.find( ".frag" ) != std::string::npos ) {
            m_stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        }
        else {
            IE_LOG_ERROR( "Loading shader \"{}\" failed: {}", full_path.string(), "Invalid shader stage" );
            return Result::Fail;
        }

        size_t data_size   = 0;
        void*  shader_data = SDL_LoadFile( full_path.string().c_str(), &data_size );
        if ( shader_data == nullptr ) {
            IE_LOG_ERROR( "Loading shader \"{}\" failed at SDL_LoadFile: {}", full_path.string(), SDL_GetError() );
            return Result::Fail;
        }

        std::filesystem::path metadata_full_path = full_path.parent_path().parent_path().concat( "/meta/" );
        metadata_full_path.concat( full_path.filename().replace_extension( ".json" ).string() );

        if ( std::filesystem::exists( metadata_full_path ) == false ) {
            IE_LOG_ERROR( "Loading shader \"{}\" failed: {}", full_path.string(), "Couldn't find meta data" );
            return Result::Fail;
        }

        SDL_GPUShaderCreateInfo sdl_shadercreateinfo = {};
        try {
            std::ifstream  ifs( metadata_full_path.string().c_str() );
            nlohmann::json meta_data_json = nlohmann::json::parse( ifs );

            SDL_PropertiesID pid = 0;
#ifdef _DEBUG
            pid = SDL_CreateProperties();
            SDL_SetStringProperty( pid, SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, file_name.c_str() );
#endif

            sdl_shadercreateinfo.code_size            = data_size;
            sdl_shadercreateinfo.code                 = static_cast<Uint8*>( shader_data );
            sdl_shadercreateinfo.entrypoint           = ms_shaderFormat.EntryPoint.data();
            sdl_shadercreateinfo.format               = ms_shaderFormat.Format;
            sdl_shadercreateinfo.stage                = m_stage;
            sdl_shadercreateinfo.num_samplers         = meta_data_json.at( "samplers" );
            sdl_shadercreateinfo.num_storage_textures = meta_data_json.at( "storage_textures" );
            sdl_shadercreateinfo.num_storage_buffers  = meta_data_json.at( "storage_buffers" );
            sdl_shadercreateinfo.num_uniform_buffers  = meta_data_json.at( "uniform_buffers" );
            sdl_shadercreateinfo.props                = pid;

        } catch ( std::exception e ) {
            IE_LOG_ERROR( "Loading shader \"{}\" failed: {}", full_path.string(), "Invalid meta data" );
            return Result::Fail;
        }

        m_sdlShader = SDL_CreateGPUShader( m_Device, &sdl_shadercreateinfo );
        SDL_free( shader_data );

        IE_LOG_DEBUG( "Loaded shader \"{}\"", full_path.string() );
        return Result::Success;
    }

    std::filesystem::path Shader::build_path( const std::filesystem::path& folder, std::string_view file_name )
    {
        IE_ASSERT( ms_shaderFormat.Format != SDL_GPU_SHADERFORMAT_INVALID );
        return ( folder / ms_shaderFormat.SubDirectory ).append( file_name ).concat( ms_shaderFormat.FileNameExtension );
    }
}    // namespace InnoEngine
