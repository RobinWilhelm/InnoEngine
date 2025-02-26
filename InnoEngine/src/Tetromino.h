#pragma once

#include "SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

#include <cstdint>
#include <array>

enum class TetrominoType : uint32_t
{
    O,
    I,
    T,
    S,
    Z,
    L,
    J
};

enum class Orientation : uint32_t
{
    Up,
    Right,
    Down,
    Left
};

using Direction = Orientation;

struct Element
{
    int32_t x = 0;
    int32_t y = 0;
};

struct Structure
{
    std::vector<Element> Elements;
};

class Tetromino
{
public:
    Tetromino( TetrominoType type, Orientation orientation, uint16_t x, uint16_t y );
    ~Tetromino() = default;

    void             rotate_once();
    void             set_orientation( Orientation orientation );
    void             move_one( Direction direction );
    void             move_to_position(int x, int y );
    TetrominoType    get_type();
    const Structure& get_structure();
    Orientation      get_orientation();
            
    static Structure   get_prototype_structure( TetrominoType type, Orientation orientation );
    static DXSM::Color get_color( TetrominoType type );

private:
    TetrominoType m_type        = TetrominoType::O;
    Orientation   m_orientation = Orientation::Up;
    Structure     m_structure   = {};
    int      m_x = 0, m_y = 0;
};
