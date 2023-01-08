// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "PlaylistSelectionPage.xaml.h"
#if __has_include("PlaylistSelectionPage.g.cpp")
#include "PlaylistSelectionPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Folderify::implementation
{
    PlaylistSelectionPage::PlaylistSelectionPage()
    {
        InitializeComponent();
    }

    int32_t PlaylistSelectionPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void PlaylistSelectionPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}
