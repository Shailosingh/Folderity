// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "PlaylistSelectionPage.xaml.h"
#if __has_include("PlaylistSelectionPage.g.cpp")
#include "PlaylistSelectionPage.g.cpp"
#endif
#include <thread>

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

        //Initialize the selected index as -1 (nothing is selected yet)
        CurrentlySelectedIndex = -1;

        //No playlists have been selected yet (when it is selected, the playlist tracking thread shall begin)
		ControllerObject->PlaylistThreadRunning = false;

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
    
    void PlaylistSelectionPage::OnNavigatingFrom(Microsoft::UI::Xaml::Navigation::NavigatingCancelEventArgs const& e)
    {
        //Check if the playlist thread is even open
		if (ControllerObject->PlaylistThreadRunning)
		{
            //Signal that the playlist page is closing up
            SetEvent(ControllerObject->PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::PageClosing)]);

            //Wait for the playlist page to close
            WaitForSingleObject(ControllerObject->PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::PageClosed)], INFINITE);

            //Close up all normal events
            for (int index = 0; index < static_cast<int>(PlaylistPageEventEnums::NumberOfOrdinaryEvents); index++)
            {
				CloseHandle(ControllerObject->PlaylistPageEvents[index]);
            }

            //Close up the change notification
			bool successfullyClosed = FindCloseChangeNotification(ControllerObject->PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::RefreshRequired)]);
            if (!successfullyClosed)
            {
                assert(false);
                exit(-1);
            }
		}
    }

    void PlaylistSelectionPage::PlaylistEventProc()
    {
        OutputDebugString(L"PLAYLIST PAGE THREAD ENTERING\n");
        ControllerObject->PlaylistThreadRunning = true;
        
        //Create the normal events
        for (int index = 0; index < static_cast<int>(PlaylistPageEventEnums::NumberOfOrdinaryEvents); index++)
        {
            ControllerObject->PlaylistPageEvents[index] = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (ControllerObject->PlaylistPageEvents[index] == nullptr)
            {
                //Failed to create PlaylistPageEvents
                assert(false);
                exit(-1);
            }
        }

        //Get the playlist path
        const wchar_t* playlistPath = m_mainViewModel.Playlists().GetAt(CurrentlySelectedIndex).PlaylistPath().c_str();

        //Set the change notification on the playlist directory
		ControllerObject->PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::RefreshRequired)] = FindFirstChangeNotification(playlistPath, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
		if (ControllerObject->PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::RefreshRequired)] == INVALID_HANDLE_VALUE)
		{
			//Failed to set change notification
			assert(false);
			exit(-1);
		}

        //Wait on all events
        bool isSuccess = true;
        while (ControllerObject->PlaylistThreadRunning)
        {
            //Wait for any of the events to be signaled (except for PageClosed and PlaylistSwitched)
			DWORD waitResult = WaitForMultipleObjects(static_cast<DWORD>(PlaylistPageEventEnums::NumberOfEvents), ControllerObject->PlaylistPageEvents, FALSE, INFINITE);
            switch (waitResult)
            {
                case WAIT_OBJECT_0 + static_cast<int>(PlaylistPageEventEnums::PlaylistSwitching) :
                    OutputDebugString(L"PLAYLIST PAGE EVENT: Playlist Switching\n");
					//Close up the current notification handle
                    isSuccess = FindCloseChangeNotification(ControllerObject->PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::RefreshRequired)]);
                    if (!isSuccess)
                    {
						//Failed to close the change notification
						assert(false);
						exit(-1);
                    }
                    
                    //Get the newly selected playlist path
                    playlistPath = m_mainViewModel.Playlists().GetAt(CurrentlySelectedIndex).PlaylistPath().c_str();

					//Set the change notification on the new playlist directory
					ControllerObject->PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::RefreshRequired)] = FindFirstChangeNotification(playlistPath, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
                    if (ControllerObject->PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::RefreshRequired)] == INVALID_HANDLE_VALUE)
                    {
						//Failed to set change notification
						assert(false);
						exit(-1);
                    }

                    //Signal to the main thread that the playlist is switched
					SetEvent(ControllerObject->PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::PlaylistSwitched)]);
                    break;
                    
                case WAIT_OBJECT_0 + static_cast<int>(PlaylistPageEventEnums::RefreshRequired) :
                    OutputDebugString(L"PLAYLIST PAGE EVENT: Refresh Required\n");
                    //Refresh the playlist (Dispatch it)
                    DispatchRefreshPlaylist();

                    //Reset the change notification back to on
					isSuccess = FindNextChangeNotification(ControllerObject->PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::RefreshRequired)]);
					if (!isSuccess)
					{
						//Failed to reset the change notification
						assert(false);
						exit(-1);
					}
					break;

				case WAIT_OBJECT_0 + static_cast<int>(PlaylistPageEventEnums::PageClosing) :
                    OutputDebugString(L"PLAYLIST PAGE EVENT: Page Closing\n");
					//Exit the thread (The event handles will be cleaned later)
					ControllerObject->PlaylistThreadRunning = false;
					break;

				default:
					//Unexpected result
					assert(false);
					exit(-1);
					break;
			}
		}

        //Signal thread exit
		SetEvent(ControllerObject->PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::PageClosed)]);
        OutputDebugString(L"PLAYLIST PAGE THREAD EXITING\n");
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
        if (AllPlaylistsListView().SelectedIndex() < 0 || CurrentlySelectedIndex == AllPlaylistsListView().SelectedIndex())
        {
            return;
        }
        CurrentlySelectedIndex = AllPlaylistsListView().SelectedIndex();
        ControllerObject->GetPlaylistSongNames(CurrentlySelectedIndex, m_mainViewModel);

        //If the thread isn't already on, put it on and detach it.
		if (!ControllerObject->PlaylistThreadRunning)
		{
			std::thread playlistThread(&PlaylistSelectionPage::PlaylistEventProc, this);
			playlistThread.detach();
		}
        
        else
        {
            //Signal the playlist thread that the playlist is switching
			SetEvent(ControllerObject->PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::PlaylistSwitching)]);

			//Wait for the playlist thread to signal that it has switched
			WaitForSingleObject(ControllerObject->PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::PlaylistSwitched)], INFINITE);
        }

        //Refresh the playlist
        RefreshPlaylist();
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

    void PlaylistSelectionPage::RefreshPlaylist()
    {
        int32_t selectedIndex = AllPlaylistsListView().SelectedIndex();;
        bool isRefreshed = ControllerObject->RefreshPlaylistSongNames(selectedIndex, m_mainViewModel);
        if (isRefreshed)
        {
            //If refresh succeeded reselect the playlist
            AllPlaylistsListView().SelectedIndex(selectedIndex);
        }
    }

    //Dispatcher methods---------------------------------------------------------------------------
    void PlaylistSelectionPage::DispatchRefreshPlaylist()
    {
        if (this->DispatcherQueue().HasThreadAccess())
        {
            RefreshPlaylist();
        }
        else
        {
            bool isQueued = this->DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, [this]()
                {
                    RefreshPlaylist();
                });
        }
    }
}
