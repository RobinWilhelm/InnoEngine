#pragma once
#include "InnoEngine/BaseTypes.h"

namespace InnoEngine
{
    class Camera
    {
    protected:
        Camera() = default;
    public:
        virtual void                 update()                                      = 0;
        virtual const DXSM::Vector3& get_position()                                = 0;
        virtual void                 set_position( const DXSM::Vector3& position ) = 0;

        virtual const DXSM::Matrix& get_viewmatrix() const           = 0;
        virtual const DXSM::Matrix& get_projectionmatrix() const     = 0;
        virtual const DXSM::Matrix& get_viewprojectionmatrix() const = 0;
    };

}    // namespace InnoEngine
