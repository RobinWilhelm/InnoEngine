#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/OrthographicCamera.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/InputSystem.h"
#include "InnoEngine/graphics/Window.h"

namespace InnoEngine
{
    auto OrthographicCamera::create( const DXSM::Vector2& view_port_size, Origin origin ) -> Ref<OrthographicCamera>
    {
        Ref<OrthographicCamera> pOrthoCamera( new OrthographicCamera() );
        pOrthoCamera->m_ViewPortSize     = view_port_size;
        pOrthoCamera->m_HalfViewPortSize = view_port_size / 2.0f;
        pOrthoCamera->m_Origin           = origin;

        switch ( origin ) {
        case Origin::Middle:
            pOrthoCamera->m_Left   = -pOrthoCamera->m_HalfViewPortSize.x;
            pOrthoCamera->m_Right  = pOrthoCamera->m_HalfViewPortSize.x;
            pOrthoCamera->m_Bottom = -pOrthoCamera->m_HalfViewPortSize.y;
            pOrthoCamera->m_Top    = pOrthoCamera->m_HalfViewPortSize.y;
            break;
        case Origin::TopLeft:
            pOrthoCamera->m_Left   = 0.0f;
            pOrthoCamera->m_Right  = pOrthoCamera->m_ViewPortSize.x;
            pOrthoCamera->m_Bottom = pOrthoCamera->m_ViewPortSize.y;
            pOrthoCamera->m_Top    = 0.0f;
            break;
        case Origin::BottomLeft:
            pOrthoCamera->m_Left   = 0.0f;
            pOrthoCamera->m_Right  = pOrthoCamera->m_ViewPortSize.x;
            pOrthoCamera->m_Bottom = 0.0f;
            pOrthoCamera->m_Top    = pOrthoCamera->m_ViewPortSize.y;
            break;
        }

        pOrthoCamera->set_position( { 0, 0, 1.0f } );
        pOrthoCamera->update();
        return pOrthoCamera;
    }

    void OrthographicCamera::update()
    {
        if ( m_Dirty ) {
            m_Projection = DXSM::Matrix::CreateOrthographicOffCenter( m_Left * m_Position.z,
                                                                      m_Right * m_Position.z,
                                                                      m_Bottom * m_Position.z,
                                                                      m_Top * m_Position.z,
                                                                      1.0f,
                                                                      0.0f );
            auto eye_pos = m_Position;
            eye_pos.x /= m_HalfViewPortSize.x * m_Position.z;
            eye_pos.y /= m_HalfViewPortSize.y * m_Position.z;

            auto target_pos = eye_pos;
            target_pos.z -= 1.0f;
            m_View = DXSM::Matrix::CreateLookAt( eye_pos, target_pos, { 0.0f, 1.0f, 0.0f } );

            m_ViewProjection         = m_Projection * m_View;
            m_InvertedViewProjection = m_ViewProjection.Invert();

            m_Dirty = false;
        }
    }

    const DXSM::Vector3& OrthographicCamera::get_position()
    {
        return m_Position;
    }

    void OrthographicCamera::set_position( const DXSM::Vector3& position )
    {
        if ( m_Position != position ) {
            m_Position = position;
            m_Dirty    = true;
        }
    }

    const DXSM::Matrix& OrthographicCamera::get_viewmatrix() const
    {
        return m_View;
    }

    const DXSM::Matrix& OrthographicCamera::get_projectionmatrix() const
    {
        return m_Projection;
    }

    const DXSM::Matrix& OrthographicCamera::get_viewprojectionmatrix() const
    {
        return m_ViewProjection;
    }

    const DXSM::Matrix& OrthographicCamera::get_inverted_viewprojectionmatrix() const
    {
        return m_InvertedViewProjection;
    }
}    // namespace InnoEngine
