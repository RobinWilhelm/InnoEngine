#include "iepch.h"
#include "TetrisGameScene.h"

#include "CoreAPI.h"

#include "Application.h"
#include "Renderer.h"
#include "Sprite2DPipeline.h"
#include "AssetManager.h"
#include "Window.h"

TetrisGameScene::TetrisGameScene()
{
    std::random_device rd;
    m_rngEngine.seed( rd() );
    auto param = m_tetrominoDistribution.param();
    param._Init( static_cast<std::underlying_type_t<TetrominoType>>( TetrominoType::O ), static_cast<std::underlying_type_t<TetrominoType>>( TetrominoType::J ) );
    m_tetrominoDistribution.param( param );

    auto tileSpriteOpt = CoreAPI::get_assetmanager()->require_asset<Sprite>( "tile.png" );
    m_tileSprite       = tileSpriteOpt.value();
    m_tileSprite.get()->create_device_ressources( CoreAPI::get_gpurenderer() );

    create_playingfield( 10, 20 );
    m_nextTetromino = static_cast<TetrominoType>( m_tetrominoDistribution( m_rngEngine ) );
}

TetrisGameScene::~TetrisGameScene()
{
    destroy_playingfield();
}

void TetrisGameScene::create_playingfield( uint16_t width, uint16_t height, uint16_t spawnAreaHeight )
{
    m_gameField       = new FieldElement[width * ( height + spawnAreaHeight /* spawnarea for the active pieces */ )];
    m_spawnAreaHeight = spawnAreaHeight;
    m_fieldWidth      = width;
    m_fieldHeight     = height;

    m_removedElements.reserve( width * height );    // just reserve the maximum possible space here
}

void TetrisGameScene::destroy_playingfield()
{
    delete[] m_gameField;
}

void TetrisGameScene::process_input()
{
    if ( m_activeTetromino != nullptr ) {
        if ( m_keyDown_R ) {
            Orientation next = static_cast<Orientation>( ( static_cast<int>( m_activeTetromino->get_orientation() ) + 1 ) % 4 );
            if ( check_collision_static( next ) == false ) {
                m_activeTetromino->rotate_once();
                m_keyDown_R = false;
            }
        }

        if ( m_keyDown_A && !check_collision( Direction::Left ) ) {
            m_activeTetromino->move_one( Direction::Left );
            m_keyDown_A = false;
        }

        if ( m_keyDown_D && !check_collision( Direction::Right ) ) {
            m_activeTetromino->move_one( Direction::Right );
            m_keyDown_D = false;
        }

        if ( m_keyDown_S && !check_collision( Direction::Down ) ) {
            m_activeTetromino->move_one( Direction::Down );
            m_keyDown_S = false;
        }
    }
}

void TetrisGameScene::create_random_tetromino()
{
    TetrominoType newType = m_nextTetromino;
    m_activeTetromino     = std::make_unique<Tetromino>( newType, Orientation::Up, static_cast<uint16_t>( get_field_width() / 2 - 1 ), static_cast<uint16_t>( 1 ) );
    m_nextTetromino       = static_cast<TetrominoType>( m_tetrominoDistribution( m_rngEngine ) );
}

void TetrisGameScene::fuse_to_field()
{
    IE_ASSERT( m_activeTetromino != nullptr );

    for ( const Element& elem : m_activeTetromino->get_structure().Elements ) {
        FieldElement& fieldElem = m_gameField[( elem.y * get_field_width() ) + elem.x];
        fieldElem.Active        = true;
        fieldElem.Color         = Tetromino::get_color( m_activeTetromino->get_type() );

        // check if we lost by checking if the tetromino got partially fused into the spawnarea
        if ( elem.y < m_spawnAreaHeight ) {
            // TODO: show some proper end game screen
            CoreAPI::get_application()->shutdown();
        }
    }

    // delete the active tetromino after collision so a new one can be created
    m_activeTetromino.reset();
}

void TetrisGameScene::check_row_completion()
{
    for ( int y = m_spawnAreaHeight; y < get_field_height(); ++y ) {
        bool completedRow = true;
        for ( int x = 0; x < m_fieldWidth; ++x ) {
            if ( m_gameField[( y * get_field_width() ) + x].Active == false ) {
                completedRow = false;
                break;
            }
        }
        if ( completedRow ) {
            int width = get_field_width();

            // store the removed elements for a falling out effect
            for ( int x = 0; x < width; ++x ) {
                RemovedElement relem;
                relem.Color        = m_gameField[y * width + x].Color;
                relem.PositionNext = DXSM::Vector2( get_element_x_coord( x ), get_element_y_coord( y ) );
                relem.Velocity     = DXSM::Vector2( static_cast<float>( ( x - 5 ) * ( 10 + SDL_rand( 10 ) ) ), static_cast<float>( -100 - SDL_rand( 100 ) ) );
                m_removedElements.push_back( relem );
            }

            // move all rows above down by one
            memcpy( &m_gameField[width], &m_gameField[0], y * width * sizeof( FieldElement ) );
            // TODO: player should got some points here
        }
    }
}

void TetrisGameScene::update_fallout_effect( double deltaTime )
{
    int  min    = 0 - m_tileSprite.get()->get_width();
    auto elemIt = m_removedElements.begin();
    while ( elemIt != m_removedElements.end() ) {
        if ( elemIt->PositionNext.y > CoreAPI::get_application()->get_window()->get_height() || elemIt->PositionNext.x > CoreAPI::get_application()->get_window()->get_width() ||
             elemIt->PositionNext.x < min ) {
            elemIt = m_removedElements.erase( elemIt );
            continue;
        }
        elemIt->Position = elemIt->PositionNext;
        elemIt->PositionNext += elemIt->Velocity * deltaTime;
        elemIt->Velocity += m_gravityAccel * deltaTime;
        ++elemIt;
    }
}

void TetrisGameScene::render_field()
{
    const std::shared_ptr<Sprite> sprite = m_tileSprite.get();

    // render playing field borders
    DXSM::Color borderColorModifier { 0.8f, 0.8f, 0.8f, 1.0f };
    for ( int x = 1; x < get_total_width() - 1; x++ ) {
        sprite->render( static_cast<float>( x * m_tileSprite.get()->get_width() ), 0.0f, borderColorModifier );
        sprite->render( static_cast<float>( x * m_tileSprite.get()->get_width() ), static_cast<float>( ( get_total_height() - 1 ) * m_tileSprite.get()->get_height() ), borderColorModifier );
    }

    for ( int y = 0; y < get_total_height(); y++ ) {
        sprite->render( 0.0f, static_cast<float>( y * m_tileSprite.get()->get_height() ), borderColorModifier );
        sprite->render( static_cast<float>( ( get_total_width() - 1 ) * m_tileSprite.get()->get_width() ), static_cast<float>( y * m_tileSprite.get()->get_height() ), borderColorModifier );
    }

    // render static elements
    for ( int y = 0; y < get_field_height(); y++ ) {
        for ( int x = 0; x < get_field_width(); x++ ) {
            if ( m_gameField[y * get_field_width() + x].Active ) {
                sprite->render( static_cast<float>( ( x + m_borderThickness ) * m_tileSprite.get()->get_width() ), static_cast<float>( ( y + m_borderThickness ) * m_tileSprite.get()->get_height() ),
                                m_gameField[y * get_field_width() + x].Color );
            }
        }
    }
}

int TetrisGameScene::get_field_width() const
{
    return m_fieldWidth;
}

int TetrisGameScene::get_field_height() const
{
    return m_fieldHeight + m_spawnAreaHeight;
}

int TetrisGameScene::get_total_width() const
{
    return m_fieldWidth + 2 * m_borderThickness;
}

int TetrisGameScene::get_total_height() const
{
    return m_fieldHeight + m_spawnAreaHeight + 2 * m_borderThickness;
}

float TetrisGameScene::get_element_x_coord( int elem_x )
{
    return static_cast<float>( ( elem_x + m_borderThickness ) * m_tileSprite.get()->get_width() );
}

float TetrisGameScene::get_element_y_coord( int elem_y )
{
    return static_cast<float>( ( elem_y + m_borderThickness ) * m_tileSprite.get()->get_height() );
}

bool TetrisGameScene::check_collision( Direction dir ) const
{
    for ( const Element& elem : m_activeTetromino->get_structure().Elements ) {
        switch ( dir ) {
        case Direction::Down:
        {
            if ( elem.y + 1 >= get_field_height() ||                                                                         // check for collision with south border
                 ( elem.y + 1 < get_field_height() && m_gameField[( elem.y + 1 ) * get_field_width() + elem.x].Active ) )    // check for collision with field element directly underneath
            {
                return true;
            }
            break;
        }
        case Direction::Left:
        {
            if ( elem.x - 1 < 0 ||                                                                      // check for collision with left border
                 ( elem.x - 1 > 0 && m_gameField[elem.y * get_field_width() + elem.x - 1].Active ) )    // check for collision with field element on the left
            {
                return true;
            }
            break;
        }
        case Direction::Right:
        {
            if ( elem.x + 1 >= get_field_width() ||                                                                     // check for collision with left border
                 ( elem.x + 1 < get_field_width() && m_gameField[elem.y * get_field_width() + elem.x + 1].Active ) )    // check for collision with field element on the left
            {
                return true;
            }
            break;
        }
        case Direction::Up: break;
        }
    }
    return false;
}

bool TetrisGameScene::check_collision_static( Orientation orientation ) const
{
    IE_ASSERT( m_activeTetromino != nullptr );

    Orientation temp = m_activeTetromino->get_orientation();
    m_activeTetromino->set_orientation( orientation );

    bool collide = false;
    for ( const Element& elem : m_activeTetromino->get_structure().Elements ) {
        if ( elem.y >= get_field_height() || elem.x >= get_field_width() || elem.x < 0 ||    // check for collision with borders
             m_gameField[elem.y * get_field_width() + elem.x].Active )                       // check for collision with field element directly
        {
            collide = true;
            break;
        }
    }

    m_activeTetromino->set_orientation( temp );
    return collide;
}

void TetrisGameScene::fixed_update( double deltaTime )
{
    process_input();

    m_nextTetrominoActionTime -= deltaTime;
    if ( m_nextTetrominoActionTime <= 0.0 ) {
        if ( m_activeTetromino == nullptr ) {
            create_random_tetromino();
        }
        else {
            // fuse tetromino to the playing field if we collided
            if ( check_collision( Direction::Down ) ) {
                fuse_to_field();
                check_row_completion();
            }
            else {
                m_activeTetromino->move_one( Direction::Down );
            }
        }
        m_nextTetrominoActionTime = 0.5f;
    }

    update_fallout_effect( deltaTime );
}

void TetrisGameScene::interpolate_and_create_rendercommands( float interpFactor, GPURenderer* pRenderer )
{
    (void)pRenderer;

    render_field();

    // render dynamic elements
    const std::shared_ptr<Sprite> sprite = m_tileSprite.get();

    // render next tetromino preview
    for ( const Element& elem : Tetromino::get_prototype_structure( m_nextTetromino, Orientation::Up ).Elements ) {
        sprite->render( static_cast<float>( ( elem.x + get_total_width() + 2 ) * m_tileSprite.get()->get_width() ), static_cast<float>( ( elem.y + 4 ) * m_tileSprite.get()->get_height() ),
                        Tetromino::get_color( m_nextTetromino ) );
    }

    if ( m_activeTetromino ) {
        for ( const Element& elem : m_activeTetromino->get_structure().Elements ) {
            sprite->render( static_cast<float>( ( elem.x + m_borderThickness ) * m_tileSprite.get()->get_width() ),
                            static_cast<float>( ( elem.y + m_borderThickness ) * m_tileSprite.get()->get_height() ), Tetromino::get_color( m_activeTetromino->get_type() ) );
        }
    }

    // render effects
    for ( const auto& elem : m_removedElements ) {
        DXSM::Vector2 interpolated = DXSM::Vector2::Lerp( elem.Position, elem.PositionNext, interpFactor );
        sprite->render( static_cast<float>( interpolated.x ), static_cast<float>( interpolated.y ), elem.Color );
    }
}

bool TetrisGameScene::handle_event( SDL_Event* pEvent )
{
    switch ( pEvent->type ) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
    {
        if ( pEvent->key.scancode == SDL_SCANCODE_A )
            m_keyDown_A = pEvent->key.down;
        if ( pEvent->key.scancode == SDL_SCANCODE_S )
            m_keyDown_S = pEvent->key.down;
        if ( pEvent->key.scancode == SDL_SCANCODE_D )
            m_keyDown_D = pEvent->key.down;
        if ( pEvent->key.scancode == SDL_SCANCODE_R )
            m_keyDown_R = pEvent->key.down;
        break;
    }
    }
    return true;
}
