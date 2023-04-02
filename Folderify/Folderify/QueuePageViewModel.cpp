#include "pch.h"
#include "QueuePageViewModel.h"
#include "QueuePageViewModel.g.cpp"

namespace winrt::Folderify::implementation
{
    QueuePageViewModel::QueuePageViewModel()
    {
		m_songs = winrt::single_threaded_observable_vector<winrt::Folderify::SongInfo>();
    }
    
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::Folderify::SongInfo> QueuePageViewModel::Songs()
    {
        return m_songs;
    }
}
