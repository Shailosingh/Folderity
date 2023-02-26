// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "PlaylistSelectionPage.xaml.h"
#if __has_include("PlaylistSelectionPage.g.cpp")
#include "PlaylistSelectionPage.g.cpp"
#endif

#include "MusicController.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

//Source lists for the Playlist list and the song list for the playlist
std::vector<std::wstring> PlaylistList;
std::vector<SongSourceObject> SongList;

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

    void PlaylistSelectionPage::AddPlaylistButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e)
    {

    }

    void PlaylistSelectionPage::RefreshButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e)
    {

    }
}