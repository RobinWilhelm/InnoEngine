#pragma once

#include "BaseTypes.h"

#include "SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

#include <memory>

namespace InnoEngine
{
    class OrthographicCamera
    {
        OrthographicCamera() = default;

    public:
        [[nodiscard]]
        static auto create( float left, float right, float bottom, float top ) -> Owned<OrthographicCamera>;

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
}    // namespace InnoEngine
