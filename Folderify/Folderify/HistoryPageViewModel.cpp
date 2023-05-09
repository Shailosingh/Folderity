#include "pch.h"
#include "HistoryPageViewModel.h"
#include "HistoryPageViewModel.g.cpp"

namespace winrt::Folderify::implementation
{
    HistoryPageViewModel::HistoryPageViewModel(bool isHistoryEnabled)
    {
        m_historyEnabled = isHistoryEnabled;
		m_songs = winrt::single_threaded_observable_vector<winrt::Folderify::SongInfo>();
    }
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::Folderify::SongInfo> HistoryPageViewModel::Songs()
    {
        return m_songs;
    }
    bool HistoryPageViewModel::HistoryEnabled()
    {
		return m_historyEnabled;
    }
    void HistoryPageViewModel::HistoryEnabled(bool value)
    {
        m_historyEnabled = value;
    }
    winrt::event_token HistoryPageViewModel::PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
    {
        return m_propertyChanged.add(handler);
    }
    void HistoryPageViewModel::PropertyChanged(winrt::event_token const& token) noexcept
    {
		m_propertyChanged.remove(token);
    }
}
