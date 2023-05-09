#pragma once
#include "HistoryPageViewModel.g.h"
#include "SongInfo.h"

namespace winrt::Folderify::implementation
{
    struct HistoryPageViewModel : HistoryPageViewModelT<HistoryPageViewModel>
    {
        HistoryPageViewModel() = delete;
        HistoryPageViewModel(bool isHistoryEnabled);
        
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Folderify::SongInfo> Songs();
        
        bool HistoryEnabled();
        void HistoryEnabled(bool value);
        
        winrt::event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
        void PropertyChanged(winrt::event_token const& token) noexcept;

    private:
		winrt::Windows::Foundation::Collections::IObservableVector<winrt::Folderify::SongInfo> m_songs;
		bool m_historyEnabled;
		winrt::event<winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
namespace winrt::Folderify::factory_implementation
{
    struct HistoryPageViewModel : HistoryPageViewModelT<HistoryPageViewModel, implementation::HistoryPageViewModel>
    {
    };
}
