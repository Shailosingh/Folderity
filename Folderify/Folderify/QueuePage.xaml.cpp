// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "QueuePage.xaml.h"
#if __has_include("QueuePage.g.cpp")
#include "QueuePage.g.cpp"
#endif

#include "MusicController.h"
#include "SharedWindowVariables.h"
#include <cassert>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::Folderify::implementation
{
    QueuePage::QueuePage()
    {
        InitializeComponent();

		//Initialize song drag state bool
		IsSongBeingDragged = false;

		//Initialize the view model that will manage the queue song list
		m_mainViewModel = winrt::make<QueuePageViewModel>();

		//Get all songs in the queue
		int32_t selectedIndex = ControllerObject->GetQueueSongNames(m_mainViewModel);

		//Set the selected index of the list view to the song that is currently playing
		QueueListView().SelectedIndex(selectedIndex);

		//Update number of songs TextBlock
		if (m_mainViewModel.Songs().Size() == 1)
		{
			NumberOfSongsInQueueTextBlock().Text(L"1 Song");
		}

		else
		{
			NumberOfSongsInQueueTextBlock().Text(std::to_wstring(m_mainViewModel.Songs().Size()) + L" Songs");
		}

		//Start the event thread
		m_queueEventThreadObject = std::thread(&QueuePage::QueueEventThreadProc, this);
		m_queueEventThreadObject.detach();
    }

	void QueuePage::OnNavigatingFrom(Navigation::NavigatingCancelEventArgs const& e)
	{
		//Signal the event thread to exit
		SetEvent(ControllerObject->QueuePageEvents[static_cast<int>(QueuePageEventEnums::PageClosing)]);

		//Wait for page to be closed
		WaitForSingleObject(ControllerObject->QueuePageEvents[static_cast<int>(QueuePageEventEnums::PageClosed)], INFINITE);
	}

	void QueuePage::QueueListView_DragItemsStarting(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::DragItemsStartingEventArgs const& e)
	{
		IsSongBeingDragged = true;
	}


	void QueuePage::QueueListView_DragItemsCompleted(winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender, winrt::Microsoft::UI::Xaml::Controls::DragItemsCompletedEventArgs const& args)
	{
		ControllerObject->UpdateQueue(QueueListView().SelectedIndex(), m_mainViewModel);
		IsSongBeingDragged = false;
	}

	void QueuePage::QueueListView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e)
	{
		if (!IsSongBeingDragged)
		{
			ControllerObject->ChangeSongIndex(QueueListView().SelectedIndex());
		}
	}

	void QueuePage::QueueEventThreadProc()
	{
		//Signal thread entrance
		ControllerObject->QueueThreadRunning = true;

		//Wait on all events
		while (ControllerObject->QueueThreadRunning)
		{
			//Wait for any of the events to be signaled (except for PageClosed)
			DWORD waitResult = WaitForMultipleObjects(static_cast<int>(QueuePageEventEnums::NumberOfEvents), ControllerObject->QueuePageEvents, FALSE, INFINITE);
			switch (waitResult)
			{
			case WAIT_OBJECT_0 + static_cast<int>(QueuePageEventEnums::IndexChanged):
				DispatchSelectedIndex(ControllerObject->GetCurrentSongIndex());
				OutputDebugString(L"QUEUE PAGE EVENT: Index changed\n");
				break;
				
			case WAIT_OBJECT_0 + static_cast<int>(QueuePageEventEnums::SongListChanged):
				DispatchSongNames();
				OutputDebugString(L"QUEUE PAGE EVENT: Song list changed\n");
				break;

			case WAIT_OBJECT_0 + static_cast<int>(QueuePageEventEnums::PageClosing):
				ControllerObject->QueueThreadRunning = false;
				OutputDebugString(L"QUEUE PAGE EVENT: Page closing\n");
				break;

			//This should never happen and I want to know why if so
			default:
				assert(false);
				break;
			}
		}
		
		//Signal thread exit
		SetEvent(ControllerObject->QueuePageEvents[static_cast<int>(QueuePageEventEnums::PageClosed)]);
	}

	winrt::Folderify::QueuePageViewModel QueuePage::MainViewModel()
	{
		return m_mainViewModel;
	}

	//Dispatcher Methods---------------------------------------------------------------------------
	void QueuePage::DispatchSelectedIndex(int32_t newIndex)
	{
		if (this->DispatcherQueue().HasThreadAccess())
		{
			QueueListView().SelectedIndex(newIndex);
		}
		else
		{
			bool isQueued = this->DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, [newIndex, this]()
				{
					QueueListView().SelectedIndex(newIndex);
				});
		}
	}
	
	void QueuePage::DispatchSongNames()
	{
		if (this->DispatcherQueue().HasThreadAccess())
		{
			QueueListView().SelectedIndex(ControllerObject->GetQueueSongNames(m_mainViewModel));
		}
		else
		{
			bool isQueued = this->DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, [this]()
				{
					QueueListView().SelectedIndex(ControllerObject->GetQueueSongNames(m_mainViewModel));
				});
		}
	}
}
