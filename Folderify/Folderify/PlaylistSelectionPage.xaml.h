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
        bool IsSongBeingDragged;
        
        //Coroutines for async operations
        winrt::Windows::Foundation::IAsyncAction AddPlaylist_Coroutine();
        
    public:      
        //Event Handlers
        void AddPlaylistButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e);
        void RefreshButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e);
        void AllPlaylistsListView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        void PlaylistItemsListView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        void PlaylistItemsListView_DragItemsCompleted(winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender, winrt::Microsoft::UI::Xaml::Controls::DragItemsCompletedEventArgs const& args);
        void PlaylistItemsListView_DragItemsStarting(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::DragItemsStartingEventArgs const& e);
    };
}

namespace winrt::Folderify::factory_implementation
{
    struct PlaylistSelectionPage : PlaylistSelectionPageT<PlaylistSelectionPage, implementation::PlaylistSelectionPage>
    {
    };
}
