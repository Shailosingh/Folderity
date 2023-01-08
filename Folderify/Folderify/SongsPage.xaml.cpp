// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "SongsPage.xaml.h"
#if __has_include("SongsPage.g.cpp")
#include "SongsPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Folderify::implementation
{
    SongsPage::SongsPage()
    {
        InitializeComponent();
    }

    int32_t SongsPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void SongsPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}
