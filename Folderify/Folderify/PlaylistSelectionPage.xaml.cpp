// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "PlaylistSelectionPage.xaml.h"
#if __has_include("PlaylistSelectionPage.g.cpp")
#include "PlaylistSelectionPage.g.cpp"
#endif

#include "MusicController.h"
#include "SharedWindowVariables.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
namespace fs = std::filesystem;


//TODO: Think about monitoring the folder where the playlist is, to see if it changes and then refresh the playlist if so
//https://learn.microsoft.com/en-us/windows/win32/fileio/obtaining-directory-change-notifications

namespace winrt::Folderify::implementation
{
    PlaylistSelectionPage::PlaylistSelectionPage()
    {
        InitializeComponent();

		//Initialize the view model that will manage song and playlist lists
		m_mainViewModel = winrt::make<PlaylistSelectionPageViewModel>();

        //Initialize drag state bool
		IsSongBeingDragged = false;

        //Get all playlist info
		ControllerObject->GetPlaylistNames(m_mainViewModel);

        //Update number of playlists TextBlock
		if (m_mainViewModel.Playlists().Size() == 1)
		{
			NumberOfPlaylistsTextBlock().Text(L"1 Playlist");
		}
		else
		{
			NumberOfPlaylistsTextBlock().Text(std::to_wstring(m_mainViewModel.Playlists().Size()) + L" Playlists");
		}
    }

    winrt::Folderify::PlaylistSelectionPageViewModel PlaylistSelectionPage::MainViewModel()
    {
		return m_mainViewModel;
    }

    //https://github.com/microsoft/WinUI-3-Demos/blob/a31e37746404629a4ca6721f2e3f3b415f7da7dc/src/Build2020Demo/DemoBuildCpp/DemoBuildCpp/DemoBuildCpp/MainWindow.xaml.cpp
    void PlaylistSelectionPage::AddPlaylistButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e)
    {
        AddPlaylist_Coroutine();
    }

    winrt::Windows::Foundation::IAsyncAction PlaylistSelectionPage::AddPlaylist_Coroutine()
    {
        //Use folder picker and prefer it to start in the music folder. Then prompt the user to select a folder for the playlist
        auto picker = winrt::Windows::Storage::Pickers::FolderPicker();

        //Initialize file picker to current window
        HWND hwnd = ControllerObject->GetWindowHandle();
        winrt::check_hresult(picker.as<IInitializeWithWindow>()->Initialize(hwnd));

        //Suggest the picker open in the music folder
        picker.SuggestedStartLocation(winrt::Windows::Storage::Pickers::PickerLocationId::MusicLibrary);
        picker.FileTypeFilter().Append(L"*");

        //Get the folder the user selected
        Windows::Storage::StorageFolder folder{ co_await picker.PickSingleFolderAsync() };

        //If the user selected a folder, ensure the folder actually exists and then create a new playlist in it
        if (folder == nullptr)
        {
            //TODO: Do something if finding folder failed
            co_return;
        }

        //Get the path to the folder
        std::wstring folderPath = folder.Path().c_str();

        //Check if folder actually exists at all
        if (!fs::exists(folderPath))
        {
            //TODO: Do something since folder selected is invalid
            co_return;
        }

        //Create a playlist in this folder
        if (!ControllerObject->CreateNewPlaylist(folderPath, m_mainViewModel))
        {
            //TODO: Notify user playlist already exists
            co_return;
        }

        //Load the song list by selecting the recently made playlist in the AllPlaylistsListView
		AllPlaylistsListView().SelectedIndex(m_mainViewModel.Playlists().Size() - 1);

        //Update number of playlists TextBlock
        if (m_mainViewModel.Playlists().Size() == 1)
        {
            NumberOfPlaylistsTextBlock().Text(L"1 Playlist");
        }
        else
        {
            NumberOfPlaylistsTextBlock().Text(std::to_wstring(m_mainViewModel.Playlists().Size()) + L" Playlists");
        }
    }

    void PlaylistSelectionPage::AllPlaylistsListView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e)
    {
        //Load the song list
        ControllerObject->GetPlaylistSongNames(AllPlaylistsListView().SelectedIndex(), m_mainViewModel);
        
        //Enable the refresh button
		RefreshButton().IsEnabled(true);
    }
    
    void PlaylistSelectionPage::PlaylistItemsListView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e)
    {
        //Load the selected song and its playlist in the queue only if a song isn't being dragged
        if (!IsSongBeingDragged)
        {
            ControllerObject->AddPlaylistToQueue(AllPlaylistsListView().SelectedIndex(), PlaylistItemsListView().SelectedIndex());
        }
    }

    void PlaylistSelectionPage::PlaylistItemsListView_DragItemsStarting(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::DragItemsStartingEventArgs const& e)
    {
		IsSongBeingDragged = true;
    }

    void PlaylistSelectionPage::PlaylistItemsListView_DragItemsCompleted(winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender, winrt::Microsoft::UI::Xaml::Controls::DragItemsCompletedEventArgs const& args)
    {
		ControllerObject->UpdatePlaylist(AllPlaylistsListView().SelectedIndex(), m_mainViewModel);
        IsSongBeingDragged = false;
    }
    
    void PlaylistSelectionPage::RefreshButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e)
    {
		int32_t selectedIndex = AllPlaylistsListView().SelectedIndex();
		bool isRefreshed = ControllerObject->RefreshPlaylistSongNames(selectedIndex, m_mainViewModel);
        if (isRefreshed)
        {
            //If refresh succeeded reselect the playlist
			AllPlaylistsListView().SelectedIndex(selectedIndex);
        }
        else
        {
			//If refresh failed, the playlist will be deselected so, disable the refresh button
            RefreshButton().IsEnabled(false);
        }
    }
}
