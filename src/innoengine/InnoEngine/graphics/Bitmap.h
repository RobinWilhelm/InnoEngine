#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"

#include "InnoEngine/BaseTypes.h"

#include <filesystem>

namespace InnoEngine
{
    struct BitmapSpecifications
    {
        uint32_t    Width  = 0;
        uint32_t    Height = 0;
        PixelFormat Format = PixelFormat::Invalid;
    };

    class Bitmap
    {
    public:
        virtual ~Bitmap();

        static auto create_empty( const BitmapSpecifications& specs ) -> std::optional<Ref<Bitmap>>;
        static auto create_from_file( std::filesystem::path& full_path ) -> std::optional<Ref<Bitmap>>;

        Result load_from_file( const std::filesystem::path& full_path );

        uint32_t get_width() const;
        uint32_t get_height() const;

        SDL_Surface* get_sdlsurface() const;
        SDRColor     get_pixel( uint32_t x, uint32_t y ) const;
        void         set_pixel( uint32_t x, uint32_t y, SDRColor color );

    private:
        SDL_Surface*         m_SDLSurface = nullptr;
        BitmapSpecifications m_Specs;
    };
}    // namespace InnoEngine
