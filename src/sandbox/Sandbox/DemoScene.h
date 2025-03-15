#include "InnoEngine/Layer.h"
#include "InnoEngine/Renderer.h"

#include "InnoEngine/Font.h"
#include "InnoEngine/Texture2D.h"
#include "InnoEngine/Sprite.h"

namespace IE = InnoEngine;

constexpr int sprite_count = 100000;

class DemoLayer : public IE::Layer
{
public:
    DemoLayer();

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
};
