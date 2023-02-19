#pragma once
#include "pch.h"
#include <vector>
#include <filesystem>
#include <queue>
#include "MainWindow.xaml.h"

//This header path will be diferent depending on where you store your MMFSoundPlayer. Mine is here. Important thing is that you import the "MMFSoundPlayer.h" file in the MMFSoundPlayer project
#include "/C++ Projects/MMFSoundPlayer/MMFSoundPlayer/MMFSoundPlayer.h"

typedef struct Song
{
	std::wstring songNameWithExtension;
	std::wstring playlistPath;
} Song;

typedef struct Playlist
{
	std::wstring playlistPath;
	std::vector<std::wstring> songNamesWithExtension;
} Playlist;

class MusicController
{
private:
	//Constants
	const std::filesystem::path LOCAL_FOLDER_NAME = L"Folderity";
	const std::filesystem::path PLAYLIST_MASTER_FILENAME = L"PlaylistMaster.bin";
	const std::filesystem::path HISTORY_FILENAME = L"History.bin";
	const std::filesystem::path QUEUE_FILENAME = L"Queue.bin";
	const std::filesystem::path PLAYLIST_SONG_ORDER_FILENAME = L"PlaylistOrder.FolderityPlaylist";

	//Unit Constants
	UINT64 const OneSecond_100NanoSecondUnits = 10000000;
	
	//General Datafields
	CComPtr<MMFSoundPlayerLib::MMFSoundPlayer> SoundPlayer;
	winrt::Folderify::implementation::MainWindow* MainWindowPointer;
	std::vector<Song> PlayerQueue;
	uint64_t CurrentSongIndex;
	std::vector<Playlist> AllPlaylists;
	std::vector<Song> SongHistory;

	//String helpers
	std::wstring Convert100NanoSecondsToTimestamp(UINT64 input100NanoSeconds);

	//Event fields and functions
	void EventThread();
	double NewSongPosition;
	bool ProgramRunning;
	bool SongPositionBarHeld;
	
	//File IO Initialization helpers
	bool LoadAllPlaylistsFromMasterFile();
	bool LoadAllSongsFromPlaylistFiles();
	bool LoadQueueSongsFromQueueFile();
	bool LoadHistoryFromHistoryFile();
	
	//File IO General helpers
	bool UpdatePlaylistMasterFile();
	bool UpdateQueueFile();
	bool UpdateHistoryFile();
	bool CheckForAndHandleAddedOrRemovedSongs(Playlist& playlistObject);
	bool UpdatePlaylistOrderFile(const Playlist& playlistObject);
	void LoadPlaylistIntoQueue(const Playlist& playlistObject);

	//Dispatcher functions
	void DispatchSongTitle(std::wstring title);
	void DispatchPlaylistTitle(std::wstring title);
	void DispatchCurrentSongPositionAndDuration();
	void DispatchPreviousButtonToggle(bool isEnabled);
	void DispatchNextButtonToggle(bool isEnabled);
	void DispatchPlayButtonToggle(bool isEnabled);
	void DispatchTrackBarToggle(bool isEnabled);
	
public:
	//Public events
	HANDLE SongChangedEvent;
	HANDLE HistoryUpdatedEvent;

	//Public lists (made public to be data sources for ListViews)
	
	//Getters
	MMFSoundPlayerLib::PlayerState GetPlayerState();
	double GetVolumeLevel();
	
	//Setters
	void SetVolumeLevel(double volumeLevel);

	//Player Controls
	void Play();
	void Pause();
	void Previous();
	void Next();
	void Shuffle();
	void RepeatToggle();
	
	//Destructors and Constructors
	MusicController(winrt::Folderify::implementation::MainWindow* mainWindow);
	~MusicController();
};