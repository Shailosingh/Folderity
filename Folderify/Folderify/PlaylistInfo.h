#pragma once
#include "PlaylistInfo.g.h"

namespace winrt::Folderify::implementation
{
    struct PlaylistInfo : PlaylistInfoT<PlaylistInfo>
    {
        PlaylistInfo() = delete;

        PlaylistInfo(hstring const& playlistPath, hstring const& numberOfSongs);
        hstring PlaylistPath();
        void PlaylistPath(hstring const& value);
        hstring PlaylistTitle();
        void PlaylistTitle(hstring const& value);
        hstring NumberOfSongs();
        void NumberOfSongs(hstring const& value);
        winrt::event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
        void PropertyChanged(winrt::event_token const& token) noexcept;

    private:
		hstring m_playlistTitle;
        hstring m_playlistPath;
		hstring m_numberOfSongs;
		winrt::event<winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
namespace winrt::Folderify::factory_implementation
{
    struct PlaylistInfo : PlaylistInfoT<PlaylistInfo, implementation::PlaylistInfo>
    {
    };
}
