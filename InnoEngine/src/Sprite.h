#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_gpu.h"

#include "SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

#include "Asset.h"

#include <filesystem>

class Sprite2DPipeline;
class GPURenderer;

class Sprite : public Asset<Sprite>
{
    friend class Sprite2DPipeline;

public:
    virtual ~Sprite();

    bool load_from_file( std::filesystem::path fullPath ) override;
    bool create_device_ressources( GPURenderer* pRenderer );
    void release_device_ressources();

    SDL_PixelFormat get_format() const;
    uint32_t        get_width() const;
    uint32_t        get_height() const;

    SDL_GPUTexture* get_sdltexture();
    SDL_GPUBuffer*  get_sdlvertexbuffer();
    SDL_GPUBuffer*  get_sdlindexbuffer();

    void render( float x, float y, DXSM::Color color = { 1.0f, 1.0f, 1.0f, 1.0f }, uint16_t layer = 0 );
    void render( float x, float y, float angle, float scale, DXSM::Color color = { 1.0f, 1.0f, 1.0f, 1.0f }, uint16_t layer = 0 );

private:
    bool                              m_ready     = false;
    GPURenderer*                      m_renderer  = nullptr;
    std::shared_ptr<Sprite2DPipeline> m_pipeline  = nullptr;
    SDL_Surface*                      m_imageData = nullptr;
    std::filesystem::path             m_textureFilePath;
    SDL_PixelFormat                   m_format = SDL_PIXELFORMAT_UNKNOWN;
    uint32_t                          m_width = 0, m_height = 0;

    SDL_GPUTexture* m_texture      = nullptr;
    SDL_GPUSampler* m_sampler      = nullptr;
    SDL_GPUBuffer*  m_vertexBuffer = nullptr;
    SDL_GPUBuffer*  m_indexBuffer  = nullptr;

    SDL_GPUBufferBinding         m_vertexBufferBinding   = {};
    SDL_GPUBufferBinding         m_indexBufferBinding    = {};
    SDL_GPUTextureSamplerBinding m_textureSamplerBinding = {};
};
