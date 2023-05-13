// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "HistoryPage.xaml.h"
#if __has_include("HistoryPage.g.cpp")
#include "HistoryPage.g.cpp"
#endif

#include "MusicController.h"
#include "SharedWindowVariables.h"
#include <cassert>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// TODO: Implement clear history button

namespace winrt::Folderify::implementation
{
    HistoryPage::HistoryPage()
    {
        InitializeComponent();

        //Initialize the view model that will manage the history song list
		m_mainViewModel = winrt::make<HistoryPageViewModel>(ControllerObject->HistoryEnabled());

		//Get all songs in the history
		ControllerObject->GetHistory(m_mainViewModel);

		//Start the event thread
		m_historyEventThreadObject = std::thread(&HistoryPage::HistoryEventThreadProc, this);
		m_historyEventThreadObject.detach();
    }

    winrt::Folderify::HistoryPageViewModel HistoryPage::MainViewModel()
    {
        return m_mainViewModel;
    }

    void HistoryPage::HistoryEventThreadProc()
    {
        //Signal thread entrance
		ControllerObject->HistoryThreadRunning = true;

        //Wait on all events
        while (ControllerObject->HistoryThreadRunning)
        {
            //Wait for any of the events to be signaled (except for PageClosed)
			DWORD waitResult = WaitForMultipleObjects(static_cast<int>(HistoryPageEventEnums::NumberOfEvents), ControllerObject->HistoryPageEvents, FALSE, INFINITE);
            switch (waitResult)
            {  
                case WAIT_OBJECT_0 + static_cast<int>(HistoryPageEventEnums::SongListChanged) :
                    DispatchSongList();
                    OutputDebugString(L"HISTORY PAGE EVENT: Song list changed\n");
                    break;
                    
                case WAIT_OBJECT_0 + static_cast<int>(HistoryPageEventEnums::PageClosing):
                    ControllerObject->HistoryThreadRunning = false;
                    OutputDebugString(L"HISTORY PAGE EVENT: Page closing\n");
                    break;

                //This should never happen and I want to know why if so
			    default:
				    assert(false);
				    break;
            }
        }

		//Signal thread exit
		SetEvent(ControllerObject->HistoryPageEvents[static_cast<int>(HistoryPageEventEnums::PageClosed)]);
    }

    void HistoryPage::OnNavigatingFrom(Microsoft::UI::Xaml::Navigation::NavigatingCancelEventArgs const& e)
    {
        //Signal the event thread to exit
		SetEvent(ControllerObject->HistoryPageEvents[static_cast<int>(HistoryPageEventEnums::PageClosing)]);
        
		//Wait for page to be closed
		WaitForSingleObject(ControllerObject->HistoryPageEvents[static_cast<int>(HistoryPageEventEnums::PageClosed)], INFINITE);
    }

    void HistoryPage::HistoryToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        ControllerObject->SetHistoryTrackingState(HistoryToggleSwitch().IsOn());
    }

    void HistoryPage::ClearHistoryButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e)
    {
        ControllerObject->ClearHistory();
    }

    //Dispatcher Methods---------------------------------------------------------------------------
    void HistoryPage::DispatchSongList()
    {
        if (this->DispatcherQueue().HasThreadAccess())
        {
            ControllerObject->GetHistory(m_mainViewModel);
        }
        else
        {
            bool isQueued = this->DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, [this]()
                {
                    ControllerObject->GetHistory(m_mainViewModel);
                });
        }
    }
}