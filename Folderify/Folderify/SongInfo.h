#pragma once
#include "SongInfo.g.h"

namespace winrt::Folderify::implementation
{
    struct SongInfo : SongInfoT<SongInfo>
    {
        SongInfo() = delete;

        SongInfo(hstring const& songTitle, hstring const& playlistTitle);
        hstring SongTitle();
        void SongTitle(hstring const& value);
        hstring PlaylistTitle();
        void PlaylistTitle(hstring const& value);
        winrt::event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
        void PropertyChanged(winrt::event_token const& token) noexcept;

    private:
		hstring m_songTitle;
		hstring m_playlistTitle;
		winrt::event<winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
namespace winrt::Folderify::factory_implementation
{
    struct SongInfo : SongInfoT<SongInfo, implementation::SongInfo>
    {
    };
}
