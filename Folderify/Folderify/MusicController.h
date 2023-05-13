#pragma once
#include "pch.h"
#include <vector>
#include <filesystem>
#include <queue>
#include <thread>
#include "MainWindow.xaml.h"
#include "PlaylistSelectionPageViewModel.h"
#include "QueuePageViewModel.h"

//This header path will be diferent depending on where you store your MMFSoundPlayer. Mine is here. Important thing is that you import the "MMFSoundPlayer.h" file in the MMFSoundPlayer project
#include "/Users/compu/Desktop/Code Projects/C++ Projects/MMFSoundPlayer/MMFSoundPlayer/MMFSoundPlayer.h" //Desktop 
//#include "/C++ Projects/MMFSoundPlayer/MMFSoundPlayer/MMFSoundPlayer.h" //Laptop

enum class QueuePageEventEnums
{
	IndexChanged,
	SongListChanged,
	PageClosing,
	PageClosed,

	NumberOfEvents
};

enum class HistoryPageEventEnums
{
	SongListChanged,
	PageClosing,
	PageClosed,

	NumberOfEvents
};

enum class PlaylistPageEventEnums
{
	PlaylistSwitching,
	PlaylistSwitched,
	PageClosing,
	PageClosed,
	
	NumberOfOrdinaryEvents,

	//Not ordinary event. Must be closed using FindCloseChangeNotification 
	RefreshRequired = NumberOfOrdinaryEvents,
	
	NumberOfEvents
};

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
	int32_t CurrentSongIndex;
	std::vector<Playlist> AllPlaylists;
	std::vector<Song> SongHistory;
	bool IsLoopEnabled;
	bool IsHistoryEnabled;
	double TrackbarRange;
	double VolumeBarRange;

	//String helpers
	std::wstring Convert100NanoSecondsToTimestamp(UINT64 input100NanoSeconds);

	//Event fields and functions
	void EventThreadProc();
	std::thread EventThreadObject;
	bool ProgramRunning;
	
	//File IO Initialization helpers
	bool LoadAllPlaylistsFromMasterFile();
	bool LoadAllSongsFromPlaylistFiles();
	bool LoadQueueSongsFromQueueFile();
	bool LoadHistoryFromHistoryFile();
	
	//File IO General helpers
	bool LoadSongsFromSinglePlaylist(Playlist& newPlaylist);
	bool UpdatePlaylistMasterFile();
	bool UpdateQueueFile();
	bool UpdateHistoryFile();
	bool CheckForAndHandleAddedOrRemovedSongs(Playlist& playlistObject);
	bool UpdatePlaylistOrderFile(const Playlist& playlistObject);

	//General helpers
	void AddCurrentSongToHistory();

	//Song loaders
	void LoadSongIntoPlayer(uint32_t index);
	bool LoadSongIntoPlayer_Aux(uint32_t index);

	//Dispatcher functions
	void DispatchSongTitle(std::wstring title);
	void DispatchPlaylistTitle(std::wstring title);
	void DispatchCurrentSongPositionAndDuration();
	void DispatchPreviousButtonToggle(bool isEnabled);
	void DispatchNextButtonToggle(bool isEnabled);
	void DispatchPlayButtonToggle(bool isEnabled);
	void DispatchPlayButtonIcon(bool isPlayIcon);
	void DispatchLoopButtonIcon();
	void DispatchTrackBarToggle(bool isEnabled);
	void DispatchVolumeBarValue();
	
public:
	//Public events (TODO: Use these for something. Especially if you don't make the lists public)
	HANDLE QueuePageEvents[static_cast<int>(QueuePageEventEnums::NumberOfEvents)];
	bool QueueThreadRunning;
	HANDLE HistoryPageEvents[static_cast<int>(HistoryPageEventEnums::NumberOfEvents)];
	bool HistoryThreadRunning;
	HANDLE VolumeExternallyChanged;
	HANDLE PlaylistPageEvents[static_cast<int>(PlaylistPageEventEnums::NumberOfEvents)];
	bool PlaylistThreadRunning;

	//Datafields
	bool SongPositionBarHeld;
	
	//Public mutexes
	HANDLE QueueMutex;
	HANDLE HistoryMutex;
	
	//Getters
	MMFSoundPlayerLib::PlayerState GetPlayerState();
	double GetNewVolumeBarValue();
	void GetPlaylistNames(winrt::Folderify::PlaylistSelectionPageViewModel& playlistPageModel);
	void GetPlaylistSongNames(int32_t playlistIndex, winrt::Folderify::PlaylistSelectionPageViewModel& playlistPageModel);
	int32_t GetCurrentSongIndex();
	int32_t GetQueueSongNames(winrt::Folderify::QueuePageViewModel& queuePageModel);
	void GetHistory(winrt::Folderify::HistoryPageViewModel& historyPageModel);
	bool RefreshPlaylistSongNames(int32_t playlistIndex, winrt::Folderify::PlaylistSelectionPageViewModel& playlistPageModel);
	bool HistoryEnabled();
	HWND GetWindowHandle();
	
	//Setters
	bool SetVolumeLevel(double volumeBarValue);
	void SetHistoryTrackingState(bool historyIsEnabled);
	void ClearHistory();
	bool CreateNewPlaylist(const std::wstring& newPlaylistFolderPath, winrt::Folderify::PlaylistSelectionPageViewModel& playlistPageModel);
	void UpdatePlaylist(int32_t playlistIndex, winrt::Folderify::PlaylistSelectionPageViewModel& playlistPageModel);
	void UpdateQueue(int32_t newQueueIndex, winrt::Folderify::QueuePageViewModel& queuePageModel);
	void AddPlaylistToQueue(int32_t playlistIndex, int32_t startingSongIndex);
	void ChangeSongIndex(int32_t newIndex);
	
	//Player Controls
	void Play();
	void Pause();
	void Previous();
	void Next();
	void Seek(double trackBarValue);
	void LoopToggle();
	void Shuffle();
	
	//Destructors and Constructors
	MusicController(winrt::Folderify::implementation::MainWindow* mainWindow);
	void CloseController();
};