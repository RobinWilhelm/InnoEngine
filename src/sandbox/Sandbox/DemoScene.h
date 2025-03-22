#pragma once
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/Layer.h"
#include "InnoEngine/graphics/Renderer.h"

#include "InnoEngine/graphics/Font.h"
#include "InnoEngine/graphics/Texture2D.h"
#include "InnoEngine/graphics/Sprite.h"
#include "InnoEngine/graphics/Camera.h"

#include "InnoEngine/graphics/Viewport.h"
#include "InnoEngine/graphics/DefaultCameraController.h"
#include "InnoEngine/graphics/RenderContext.h"

namespace IE = InnoEngine;

constexpr int sprite_count = 1000000;

class Sandbox;

class Demoscene : public IE::Layer
{
public:
    Demoscene( Sandbox* parent );

    // Inherited via Layer
    void update( double deltaTime ) override;
    void render( float interpFactor, IE::GPURenderer* pRenderer ) override;
    bool handle_event( const SDL_Event& pEvent ) override;

private:
    IE::Ref<IE::Font>      m_testFont;
    IE::Ref<IE::Texture2D> m_testTexture;
    IE::Sprite             m_testSprite;

    std::vector<DXSM::Vector2> m_positions;
    std::vector<float>         m_rotations;
    std::vector<DXSM::Color>   m_colors;
    std::vector<float>         m_rotationSpeeds;
    std::vector<float>         m_scales;

    uint64_t    m_colorAnimStart = 0;
    DXSM::Color m_textColor { 0.0f, 1.0f, 0.0f, 1.0f };
    float       m_color_h = 0;
    float       m_color_s = 1.0;
    float       m_color_v = 1.0;

    IE::Ref<IE::RenderContext> m_SceneRenderCtx;
};
