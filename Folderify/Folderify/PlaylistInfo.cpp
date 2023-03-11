#include "pch.h"
#include "PlaylistInfo.h"
#include "PlaylistInfo.g.cpp"

namespace winrt::Folderify::implementation
{
    PlaylistInfo::PlaylistInfo(hstring const& playlistTitle, hstring const& numberOfSongs)
    {
		m_playlistTitle = playlistTitle;
		m_numberOfSongs = numberOfSongs;
    }
    
    hstring PlaylistInfo::PlaylistTitle()
    {
        return m_playlistTitle;
    }
    void PlaylistInfo::PlaylistTitle(hstring const& value)
    {
        if (m_playlistTitle != value)
        {
			m_playlistTitle = value;
			m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs(L"PlaylistTitle"));
        }
    }
    
    hstring PlaylistInfo::NumberOfSongs()
    {
		return m_numberOfSongs;
    }
    void PlaylistInfo::NumberOfSongs(hstring const& value)
    {
        if (m_numberOfSongs != value)
        {
			m_numberOfSongs = value;
			m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs(L"NumberOfSongs"));
        }
    }
    
    winrt::event_token PlaylistInfo::PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
    {
		return m_propertyChanged.add(handler);
    }
    void PlaylistInfo::PropertyChanged(winrt::event_token const& token) noexcept
    {
		m_propertyChanged.remove(token);
    }
}
