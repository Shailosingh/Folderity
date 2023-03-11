#include "pch.h"
#include "PlaylistSelectionPageViewModel.h"
#include "PlaylistSelectionPageViewModel.g.cpp"

namespace winrt::Folderify::implementation
{
    PlaylistSelectionPageViewModel::PlaylistSelectionPageViewModel()
    {
        m_playlists = winrt::single_threaded_observable_vector<winrt::Folderify::PlaylistInfo>();
        m_songs = winrt::single_threaded_observable_vector<winrt::Folderify::SongInfo>();
    }
    
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::Folderify::PlaylistInfo> PlaylistSelectionPageViewModel::Playlists()
    {
		return m_playlists;
    }
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::Folderify::SongInfo> PlaylistSelectionPageViewModel::Songs()
    {
		return m_songs;
    }
}
