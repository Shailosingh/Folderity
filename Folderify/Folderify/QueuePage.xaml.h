// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "QueuePage.g.h"
#include "QueuePageViewModel.h"
#include <thread>

namespace winrt::Folderify::implementation
{
    struct QueuePage : QueuePageT<QueuePage>
    {
        QueuePage();

		winrt::Folderify::QueuePageViewModel MainViewModel();

	private:
		winrt::Folderify::QueuePageViewModel m_mainViewModel{ nullptr };
		bool IsSongBeingDragged;
        
        //Event thread
        std::thread m_queueEventThreadObject;
        void QueueEventThreadProc();

        //Dispatcher Methods
        void DispatchSelectedIndex(int32_t newIndex);
        void DispatchSongNames();
        
    public:
        //Public event handlers
        void OnNavigatingFrom(Microsoft::UI::Xaml::Navigation::NavigatingCancelEventArgs const& e);
        void QueueListView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        void QueueListView_DragItemsStarting(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::DragItemsStartingEventArgs const& e);
        void QueueListView_DragItemsCompleted(winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender, winrt::Microsoft::UI::Xaml::Controls::DragItemsCompletedEventArgs const& args);
    };
}

namespace winrt::Folderify::factory_implementation
{
    struct QueuePage : QueuePageT<QueuePage, implementation::QueuePage>
    {
    };
}
