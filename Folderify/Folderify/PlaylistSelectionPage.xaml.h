// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "PlaylistSelectionPage.g.h"

namespace winrt::Folderify::implementation
{
    struct PlaylistSelectionPage : PlaylistSelectionPageT<PlaylistSelectionPage>
    {
        PlaylistSelectionPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);
        void AddPlaylistButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e);
        void RefreshButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e);
    };
}

namespace winrt::Folderify::factory_implementation
{
    struct PlaylistSelectionPage : PlaylistSelectionPageT<PlaylistSelectionPage, implementation::PlaylistSelectionPage>
    {
    };
}
