#include "pch.h"
#include "Circle.h"
#if __has_include("Circle.g.cpp")
#include "Circle.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::InnoEngine::implementation
{
    int32_t Circle::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void Circle::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void Circle::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        Button().Content(box_value(L"Clicked"));
    }
}
