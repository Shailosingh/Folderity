// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "HistoryPage.g.h"
#include "HistoryPageViewModel.h"

namespace winrt::Folderify::implementation
{
    struct HistoryPage : HistoryPageT<HistoryPage>
    {
        HistoryPage();

		winrt::Folderify::HistoryPageViewModel MainViewModel();

    private:
        winrt::Folderify::HistoryPageViewModel m_mainViewModel{ nullptr };
		
        //Thread stuff
        std::thread m_historyEventThreadObject;
        void HistoryEventThreadProc();

        //Dispatcher Methods
		void DispatchSongList();
        
    public:
        void OnNavigatingFrom(Microsoft::UI::Xaml::Navigation::NavigatingCancelEventArgs const& e);
        void HistoryToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ClearHistoryButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e);
    };
}

namespace winrt::Folderify::factory_implementation
{
    struct HistoryPage : HistoryPageT<HistoryPage, implementation::HistoryPage>
    {
    };
}
