#pragma once

#include "Circle.g.h"

namespace winrt::InnoEngine::implementation
{
    struct Circle : CircleT<Circle>
    {
        Circle() 
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::InnoEngine::factory_implementation
{
    struct Circle : CircleT<Circle, implementation::Circle>
    {
    };
}
