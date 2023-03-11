#include "pch.h"
#include "SongInfo.h"
#include "SongInfo.g.cpp"

namespace winrt::Folderify::implementation
{
    SongInfo::SongInfo(hstring const& songTitle, hstring const& playlistTitle)
    {
        m_songTitle = songTitle;
        m_playlistTitle = playlistTitle;
    }
    
    hstring SongInfo::SongTitle()
    {
        return m_songTitle;
    }
    void SongInfo::SongTitle(hstring const& value)
    {
		if (m_songTitle != value)
		{
			m_songTitle = value;
			m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs(L"SongTitle"));
		}
    }
    
    hstring SongInfo::PlaylistTitle()
    {
		return m_playlistTitle;
    }
    void SongInfo::PlaylistTitle(hstring const& value)
    {
        if (m_playlistTitle != value)
        {
			m_playlistTitle = value;
			m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs(L"PlaylistTitle"));
        }
    }
    
    winrt::event_token SongInfo::PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
    {
		return m_propertyChanged.add(handler);
    }
    void SongInfo::PropertyChanged(winrt::event_token const& token) noexcept
    {
		m_propertyChanged.remove(token);
    }
}
