#include "iepch.h"
#include "Tetromino.h"

Tetromino::Tetromino( TetrominoType type, Orientation orientation, uint16_t x, uint16_t y )
{
    m_type        = type;
    m_orientation = orientation;
    m_structure   = get_prototype_structure( m_type, m_orientation );

    // move into spawn position
    move_to_position( x, y );
}

void Tetromino::rotate_once()
{
    set_orientation( static_cast<Orientation>( ( static_cast<int>( m_orientation ) + 1 ) % 4 /* wrap around */ ) );
}

void Tetromino::set_orientation( Orientation orientation )
{
    m_structure   = get_prototype_structure( m_type, orientation );
    m_orientation = orientation;
    move_to_position( m_x, m_y );
}

void Tetromino::move_one( Direction orientation )
{
    for ( Element& elem : m_structure.Elements ) {
        switch ( orientation ) {
        case Direction::Up:
        {
            IE_ASSERT( "Doesnt work this way!" );
            break;
        }
        case Direction::Right: elem.x += 1; break;
        case Direction::Down: elem.y += 1; break;
        case Direction::Left: elem.x -= 1; break;
        default: break;
        }
    }
    switch ( orientation ) {
    case Direction::Up:
    {
        IE_ASSERT( "Doesnt work this way!" );
        break;
    }
    case Direction::Down: m_y += 1; break;
    case Direction::Left: m_x -= 1; break;
    case Direction::Right: m_x += 1; break;
    }
}

void Tetromino::move_to_position(int x, int y )
{
    for ( Element& elem : m_structure.Elements ) {
        elem.x += x;
        elem.y += y;
    }

    m_x = x;
    m_y = y;
}

TetrominoType Tetromino::get_type()
{
    return m_type;
}

const Structure& Tetromino::get_structure()
{
    return m_structure;
}

Orientation Tetromino::get_orientation()
{
    return m_orientation;
}

Structure Tetromino::get_prototype_structure( TetrominoType type, Orientation orientation )
{
    // https://tetris.fandom.com/wiki/Orientation
    switch ( type ) {
    case TetrominoType::O: return { { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } } };
    case TetrominoType::I:
        switch ( orientation ) {
        case Orientation::Up: return { { { -1, 0 }, { 0, 0 }, { 1, 0 }, { 2, 0 } } };
        case Orientation::Right: return { { { 1, -1 }, { 1, 0 }, { 1, 1 }, { 1, 2 } } };
        case Orientation::Down: return { { { -1, 1 }, { 0, 1 }, { 1, 1 }, { 2, 1 } } };
        case Orientation::Left: return { { { 0, -1 }, { 0, 0 }, { 0, 1 }, { 0, 2 } } };
        }
        break;
    case TetrominoType::T:
        switch ( orientation ) {
        case Orientation::Up: return { { { -1, 0 }, { 0, 0 }, { 0, -1 }, { 1, 0 } } };
        case Orientation::Right: return { { { 0, -1 }, { 0, 0 }, { 0, 1 }, { 1, 0 } } };
        case Orientation::Down: return { { { -1, 0 }, { 0, 0 }, { 0, 1 }, { 1, 0 } } };
        case Orientation::Left: return { { { 0, -1 }, { 0, 0 }, { -1, 0 }, { 0, 1 } } };
        }
        break;
    case TetrominoType::S:
        switch ( orientation ) {
        case Orientation::Up: return { { { -1, 0 }, { 0, 0 }, { 0, -1 }, { 1, -1 } } };
        case Orientation::Right: return { { { 0, -1 }, { 0, 0 }, { 1, 0 }, { 1, 1 } } };
        case Orientation::Down: return { { { -1, 1 }, { 0, 0 }, { 0, 1 }, { 1, 0 } } };
        case Orientation::Left: return { { { -1, -1 }, { 0, 0 }, { -1, 0 }, { 0, 1 } } };
        }
        break;
    case TetrominoType::Z:
        switch ( orientation ) {
        case Orientation::Up: return { { { -1, -1 }, { 0, 0 }, { 0, -1 }, { 1, 0 } } };
        case Orientation::Right: return { { { 1, -1 }, { 0, 0 }, { 0, 1 }, { 1, 0 } } };
        case Orientation::Down: return { { { -1, 0 }, { 0, 0 }, { 0, 1 }, { 1, 1 } } };
        case Orientation::Left: return { { { 0, -1 }, { 0, 0 }, { -1, 0 }, { -1, 1 } } };
        }
        break;
    case TetrominoType::J:
        switch ( orientation ) {
        case Orientation::Up: return { { { -1, 0 }, { 0, 0 }, { 1, 0 }, { -1, -1 } } };
        case Orientation::Right: return { { { 0, -1 }, { 0, 0 }, { 0, 1 }, { 1, -1 } } };
        case Orientation::Down: return { { { -1, 0 }, { 0, 0 }, { 1, 0 }, { 1, 1 } } };
        case Orientation::Left: return { { { 0, 1 }, { 1, 1 }, { 1, 0 }, { 1, -1 } } };
        }
        break;
    case TetrominoType::L:
        switch ( orientation ) {
        case Orientation::Up: return { { { -1, 0 }, { 0, 0 }, { 1, 0 }, { 1, -1 } } };
        case Orientation::Right: return { { { 0, -1 }, { 0, 0 }, { 0, 1 }, { 1, 1 } } };
        case Orientation::Down: return { { { -1, 0 }, { 0, 0 }, { 1, 0 }, { -1, 1 } } };
        case Orientation::Left: return { { { 0, -1 }, { 0, 0 }, { 0, 1 }, { -1, -1 } } };
        }
        break;
    default: break;
    }

    return Structure();
}

DXSM::Color Tetromino::get_color( TetrominoType type )
{
    switch ( type ) {
    case TetrominoType::O: return { 1.0f, 1.0f, 0.0f, 1.0f };
    case TetrominoType::I: return { 1.0f / 255, 1.0f, 1.0f, 1.0f };
    case TetrominoType::T: return { 1.0f, 0.0f, 1.0f, 1.0f };
    case TetrominoType::S: return { 1.0f / 255, 1.0f, 0.0f, 1.0f };
    case TetrominoType::Z: return { 1.0f, 0.0f, 0.0f, 1.0f };
    case TetrominoType::L: return { 1.0f, 0.5f, 0.0f, 1.0f };
    case TetrominoType::J: return { 0.0f, 0.0f, 1.0f, 1.0f };
    }
    return { 1.0f, 1.0f, 1.0f, 1.0f };
}
