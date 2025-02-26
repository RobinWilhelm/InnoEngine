#include "iepch.h"
#include "OrthographicCamera.h"

std::unique_ptr<OrthographicCamera> OrthographicCamera::create( float left, float right, float bottom, float top )
{
    std::unique_ptr<OrthographicCamera> pOrthoCamera( new OrthographicCamera() );
    pOrthoCamera->m_projection     = DXSM::Matrix::CreateOrthographicOffCenter( left, right, bottom, top, 0.01f, 1.0f );
    pOrthoCamera->m_view           = DXSM::Matrix::Identity;
    pOrthoCamera->m_viewProjection = pOrthoCamera->m_projection * pOrthoCamera->m_view;
    return pOrthoCamera;
}

void OrthographicCamera::update()
{
    if ( m_dirty ) {
        m_view = DXSM::Matrix::CreateLookAt( m_position, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f } );
        m_view.Invert();
        m_viewProjection = m_projection * m_view;
        m_dirty          = false;
    }
}

const DXSM::Vector3& OrthographicCamera::get_position()
{
    return m_position;
}

void OrthographicCamera::set_position( const DXSM::Vector3& position )
{
    m_position = position;
    m_dirty    = true;
}

const DXSM::Matrix& OrthographicCamera::get_viewmatrix() const
{
    return m_view;
}

const DXSM::Matrix& OrthographicCamera::get_projectionmatrix() const
{
    return m_projection;
}

const DXSM::Matrix& OrthographicCamera::get_viewprojectionmatrix() const
{
    return m_viewProjection;
}
