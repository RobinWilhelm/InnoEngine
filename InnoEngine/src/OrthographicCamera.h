#pragma once

#include "SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

#include <memory>

class OrthographicCamera
{
    OrthographicCamera() = default;

public:
    [[nodiscard]]
    static std::unique_ptr<OrthographicCamera> create( float left, float right, float bottom, float top );

    void update();

    const DXSM::Vector3& get_position();
    void                 set_position( const DXSM::Vector3& position );

    const DXSM::Matrix& get_viewmatrix() const;
    const DXSM::Matrix& get_projectionmatrix() const;
    const DXSM::Matrix& get_viewprojectionmatrix() const;

private:
    DXSM::Matrix m_view;
    DXSM::Matrix m_projection;
    DXSM::Matrix m_viewProjection;

    DXSM::Vector3 m_position;
    bool          m_dirty = false;
};
