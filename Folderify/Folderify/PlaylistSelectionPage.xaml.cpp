// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "PlaylistSelectionPage.xaml.h"
#if __has_include("PlaylistSelectionPage.g.cpp")
#include "PlaylistSelectionPage.g.cpp"
#endif

#include "MusicController.h"
#include "SharedWindowVariables.h"

//Consider putting this in PCH (right now it results in compiler running out of virtual memory)

using namespace winrt;
using namespace Microsoft::UI::Xaml;
namespace fs = std::filesystem;

namespace winrt::Folderify::implementation
{
    PlaylistSelectionPage::PlaylistSelectionPage()
    {
        InitializeComponent();

        //Load in the Playlists
		//ControllerObject->GetPlaylistNames(PlaylistList);
    }

    int32_t PlaylistSelectionPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void PlaylistSelectionPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    //https://github.com/microsoft/WinUI-3-Demos/blob/a31e37746404629a4ca6721f2e3f3b415f7da7dc/src/Build2020Demo/DemoBuildCpp/DemoBuildCpp/DemoBuildCpp/MainWindow.xaml.cpp
    void PlaylistSelectionPage::AddPlaylistButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e)
    {
        /*
		//Use folder picker and prefer it to start in the music folder. Then prompt the user to select a folder for the playlist
        auto picker = winrt::Windows::Storage::Pickers::FolderPicker();

        //Initialize file picker to current window
        HWND hwnd = ControllerObject->GetWindowHandle();
        winrt::check_hresult(picker.as<IInitializeWithWindow>()->Initialize(hwnd));
        
		//Suggest the picker open in the music folder
		picker.SuggestedStartLocation(winrt::Windows::Storage::Pickers::PickerLocationId::MusicLibrary);
		picker.FileTypeFilter().Append(L"*");

		//Get the folder the user selected (get function waits synchronously)
		auto folder = picker.PickSingleFolderAsync().get();

		//If the user selected a folder, ensure the folder actually exists and then create a new playlist in it
		if (folder == nullptr)
		{
			//TODO: Do something if finding folder failed
            return;
		}

        //Get the path to the folder
        std::wstring folderPath = folder.Path().c_str();

        //Check if folder actually exists at all
        if (!fs::exists(folderPath))
        {
            //TODO: Do something since folder selected is invalid
            return;
        }

        //Create a playlist in this folder
        if (!ControllerObject->CreateNewPlaylist(folderPath))
        {
            //TODO: Notify user playlist already exists
            return;
        }

		//Refresh the playlist list
		ControllerObject->GetPlaylistNames(PlaylistList);
		ControllerObject->GetPlaylistSongNames(PlaylistList.size()-1, SongList);
        */
    }

    void PlaylistSelectionPage::RefreshButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e)
    {

    }
}