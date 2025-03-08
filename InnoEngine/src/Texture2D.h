#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"

#include "BaseTypes.h"
#include "Asset.h"

#include <string_view>

namespace InnoEngine
{
    class GPURenderer;

    class Texture2D : public Asset<Texture2D>
    {
    public:
        bool                  load_from_file( const std::filesystem::path& full_path, std::string_view file_name ) override;
        std::filesystem::path build_path( const std::filesystem::path& folder, std::string_view file_name ) override;

        Result create_device_ressources( Ref<SDL_GPUDevice> device );
        void   release_device_ressources();

        SDL_PixelFormat get_format() const;
        int             width() const;
        int             height() const;

        SDL_GPUTexture* get_sdltexture() const;

    private:
        Ref<SDL_GPUDevice> m_device      = nullptr;
        SDL_Surface*       m_surface     = nullptr;
        SDL_GPUTexture*    m_texture     = nullptr;
        bool               m_initialized = false;

        int             m_width  = 0;
        int             m_height = 0;
        SDL_PixelFormat m_format = {};
    };
}    // namespace InnoEngine
