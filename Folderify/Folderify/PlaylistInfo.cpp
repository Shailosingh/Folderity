#include "pch.h"
#include "PlaylistInfo.h"
#include "PlaylistInfo.g.cpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace winrt::Folderify::implementation
{
    PlaylistInfo::PlaylistInfo(hstring const& playlistPath, hstring const& numberOfSongs)
    {
		m_playlistPath = playlistPath;
		m_playlistTitle = fs::path(playlistPath.c_str()).filename().c_str();
		m_numberOfSongs = numberOfSongs;
    }

	hstring PlaylistInfo::PlaylistPath()
	{
		return m_playlistPath;
	}

    void PlaylistInfo::PlaylistPath(hstring const& value)
    {
		if (m_playlistPath != value)
		{
			m_playlistPath = value;
			m_propertyChanged(*this, winrt::Windows::UI::Xaml::Data::PropertyChangedEventArgs{ L"PlaylistPath" });
		}
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
