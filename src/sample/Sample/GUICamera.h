#pragma once
#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/Camera.h"

#include <memory>

namespace InnoEngine
{
    class OrthographicCamera : public Camera
    {
        OrthographicCamera() = default;

    public:
        [[nodiscard]]
        static auto create( const DXSM::Vector2& view_port_size ) -> Ref<OrthographicCamera>;

        virtual void update() override;

        virtual const DXSM::Vector3& get_position() override;
        virtual void                 set_position( const DXSM::Vector3& position ) override;

        virtual const DXSM::Matrix& get_viewmatrix() const override;
        virtual const DXSM::Matrix& get_projectionmatrix() const override;
        virtual const DXSM::Matrix& get_viewprojectionmatrix() const override;
        const DXSM::Matrix&         get_inverted_viewprojectionmatrix() const;

    private:
        DXSM::Matrix m_View;
        DXSM::Matrix m_Projection;
        DXSM::Matrix m_ViewProjection;
        DXSM::Matrix m_InvertedViewProjection;

        DXSM::Vector2 m_ViewPortSize;

        DXSM::Vector3 m_Position;
        bool          m_Dirty = false;
    };
}    // namespace InnoEngine
