#include "InnoEngine/iepch.h"
#include "InnoEngine/graphics/OrthographicCamera.h"

#include "InnoEngine/CoreAPI.h"
#include "InnoEngine/graphics/Window.h"

namespace InnoEngine
{
    auto OrthographicCamera::create( const DXSM::Vector2& view_port_size ) -> Ref<OrthographicCamera>
    {
        Ref<OrthographicCamera> pOrthoCamera( new OrthographicCamera() );
        pOrthoCamera->m_ViewPortSize = view_port_size;
        pOrthoCamera->m_View         = DXSM::Matrix::CreateLookAt( { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f } );
        pOrthoCamera->set_position( { view_port_size.x / 2, view_port_size.y / 2, 1.0f } );
        pOrthoCamera->update();
        return pOrthoCamera;
    }

    void OrthographicCamera::update()
    {
        if ( m_Dirty ) {
            m_Projection     = DXSM::Matrix::CreateOrthographicOffCenter( m_Position.x - ( m_ViewPortSize.x / 2 * m_Position.z ),
                                                                          m_Position.x + ( m_ViewPortSize.x / 2 * m_Position.z ),
                                                                          m_Position.y - ( m_ViewPortSize.y / 2 * m_Position.z ),
                                                                          m_Position.y + ( m_ViewPortSize.y / 2 * m_Position.z ),
                                                                          1.0f,
                                                                          0.0f );
            m_ViewProjection = m_Projection * m_View;
            m_Dirty          = false;
        }
    }

    const DXSM::Vector3& OrthographicCamera::get_position()
    {
        return m_Position;
    }

    void OrthographicCamera::set_position( const DXSM::Vector3& position )
    {
        m_Position = position;
        m_Dirty    = true;
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
}    // namespace InnoEngine
