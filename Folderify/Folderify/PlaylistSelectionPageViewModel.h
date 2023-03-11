#pragma once
#include "PlaylistSelectionPageViewModel.g.h"
#include "PlaylistInfo.h"
#include "SongInfo.h"

namespace winrt::Folderify::implementation
{
    struct PlaylistSelectionPageViewModel : PlaylistSelectionPageViewModelT<PlaylistSelectionPageViewModel>
    {
        PlaylistSelectionPageViewModel();

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Folderify::PlaylistInfo> Playlists();
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Folderify::SongInfo> Songs();

	private:
		winrt::Windows::Foundation::Collections::IObservableVector<winrt::Folderify::PlaylistInfo> m_playlists;
		winrt::Windows::Foundation::Collections::IObservableVector<winrt::Folderify::SongInfo> m_songs;
    };
}
namespace winrt::Folderify::factory_implementation
{
    struct PlaylistSelectionPageViewModel : PlaylistSelectionPageViewModelT<PlaylistSelectionPageViewModel, implementation::PlaylistSelectionPageViewModel>
    {
    };
}
