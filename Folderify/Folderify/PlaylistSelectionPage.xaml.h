// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "PlaylistSelectionPage.g.h"
#include "PlaylistSelectionPageViewModel.h"

namespace winrt::Folderify::implementation
{
    struct PlaylistSelectionPage : PlaylistSelectionPageT<PlaylistSelectionPage>
    {
        PlaylistSelectionPage();
        winrt::Folderify::PlaylistSelectionPageViewModel MainViewModel();
       
	private:
		winrt::Folderify::PlaylistSelectionPageViewModel m_mainViewModel{ nullptr };
        
        //Coroutines for async operations
        winrt::Windows::Foundation::IAsyncAction AddPlaylist_Coroutine();
        
    public:      
        //Event Handlers
        void AddPlaylistButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e);
        void RefreshButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e);
        void AllPlaylistsListView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
    };
}

namespace winrt::Folderify::factory_implementation
{
    struct PlaylistSelectionPage : PlaylistSelectionPageT<PlaylistSelectionPage, implementation::PlaylistSelectionPage>
    {
    };
}
