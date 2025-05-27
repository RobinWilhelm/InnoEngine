#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/Bitmap.h"

#include "SDL3_image/SDL_image.h"

#include "InnoEngine/graphics/TextureBase.h"

namespace InnoEngine
{
    Bitmap::~Bitmap()
    {
        if ( m_SDLSurface != nullptr ) {
            SDL_DestroySurface( m_SDLSurface );
            m_SDLSurface = nullptr;
        }
    }

    auto Bitmap::create_empty( const BitmapSpecifications& specs ) -> std::optional<Ref<Bitmap>>
    {
        IE_ASSERT( specs.Format != PixelFormat::Invalid );
        IE_ASSERT( specs.Width != 0 && specs.Height != 0 );

        Ref<Bitmap>     bitmap          = Ref<Bitmap>( new Bitmap() );
        SDL_PixelFormat sdl_pixelformat = TextureBase::convert_pixelformat( specs.Format );
        bitmap->m_SDLSurface            = SDL_CreateSurface( specs.Width, specs.Height, sdl_pixelformat );
        bitmap->m_Specs                 = specs;
        return bitmap;
    }

    auto Bitmap::create_from_file( std::filesystem::path& full_path ) -> std::optional<Ref<Bitmap>>
    {
        Ref<Bitmap> bitmap = Ref<Bitmap>( new Bitmap() );
        if ( IE_FAILED( bitmap->load_from_file( full_path ) ) )
            return std::nullopt;
        return bitmap;
    }

    Result Bitmap::load_from_file( const std::filesystem::path& full_path )
    {
        if ( m_SDLSurface != nullptr )
            SDL_DestroySurface( m_SDLSurface );

        m_SDLSurface = IMG_Load( full_path.string().c_str() );
        if ( m_SDLSurface == nullptr ) {
            IE_LOG_ERROR( "Loading bitmap \"{}\" failed at IMG_Load: {}", full_path.filename().string(), SDL_GetError() );
            return Result::Fail;
        }
        m_Specs.Format = TextureBase::convert_sdlpixelformat( m_SDLSurface->format );
        m_Specs.Width  = m_SDLSurface->w;
        m_Specs.Height = m_SDLSurface->h;
        return Result::Success;
    }

    uint32_t Bitmap::get_width() const
    {
        if ( m_SDLSurface == nullptr )
            return 0;
        return m_SDLSurface->w;
    }

    uint32_t Bitmap::get_height() const
    {
        if ( m_SDLSurface == nullptr )
            return 0;
        return m_SDLSurface->h;
    }

    SDL_Surface* Bitmap::get_sdlsurface() const
    {
        return m_SDLSurface;
    }

    SDRColor Bitmap::get_pixel( uint32_t x, uint32_t y ) const
    {
        IE_ASSERT( m_SDLSurface != nullptr );
        IE_ASSERT( x <= static_cast<uint32_t>( m_SDLSurface->w ) && y <= static_cast<uint32_t>( m_SDLSurface->h ) );

        switch ( m_SDLSurface->format ) {
        case SDL_PIXELFORMAT_RGB24:
        {
            std::byte* pixel = &static_cast<std::byte*>( m_SDLSurface->pixels )[ y * m_SDLSurface->w + m_SDLSurface->pitch + x ];
            SDRColor   color;
            color.U32 = *reinterpret_cast<uint32_t*>( pixel );
            color.A   = 255;
            return color;
        }
        case SDL_PIXELFORMAT_RGBA32:
            return static_cast<SDRColor*>( m_SDLSurface->pixels )[ y * m_SDLSurface->w + m_SDLSurface->pitch + x ];
        }

        IE_LOG_ERROR("Unsupported Surface format: {}", static_cast<uint32_t>(m_SDLSurface->format));
        return SDRColor();
    }

    void Bitmap::set_pixel( uint32_t x, uint32_t y, SDRColor color )
    {
        IE_ASSERT( m_SDLSurface != nullptr );
        IE_ASSERT( x <= static_cast<uint32_t>( m_SDLSurface->w ) && y <= static_cast<uint32_t>( m_SDLSurface->h ) );

        switch ( m_SDLSurface->format ) {
        case SDL_PIXELFORMAT_RGB24:
        {
            SDRColor* pixel = &static_cast<SDRColor*>( m_SDLSurface->pixels )[ y * m_SDLSurface->w + m_SDLSurface->pitch + x ];
            pixel->R        = color.R;
            pixel->G        = color.G;
            pixel->B        = color.B;
            return;
        }
        case SDL_PIXELFORMAT_RGBA32:
            static_cast<SDRColor*>( m_SDLSurface->pixels )[ y * m_SDLSurface->w + m_SDLSurface->pitch + x ] = color;
            return;
        }

        IE_LOG_ERROR( "Unsupported Surface format: {}", static_cast<uint32_t>( m_SDLSurface->format ) );
    }
}    // namespace InnoEngine
