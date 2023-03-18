#include "pch.h"
#include "SongInfo.h"
#include "SongInfo.g.cpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace winrt::Folderify::implementation
{
    SongInfo::SongInfo(hstring const& songPath)
    {
		m_songPath = songPath;
        m_songTitle = fs::path(songPath.c_str()).filename().stem().c_str();
        m_playlistTitle = fs::path(songPath.c_str()).parent_path().filename().c_str();
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

    void SongInfo::SongPath(hstring const& value)
    {
        if (m_songPath != value)
        {
			m_songPath = value;
			m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs(L"SongPath"));
        }
    }

	hstring SongInfo::SongPath()
	{
		return m_songPath;
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
