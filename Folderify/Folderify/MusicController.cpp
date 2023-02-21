#include "pch.h"
#include "MusicController.h"
#include <winrt/Windows.Storage.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <format>
#include <thread>

using namespace MMFSoundPlayerLib;
using namespace winrt::Windows::Storage;
namespace fs = std::filesystem;

//Constructors and Destructors---------------------------------------------------------------------
MusicController::MusicController(winrt::Folderify::implementation::MainWindow* mainWindow)
{
	//Initialize variables
	CurrentSongIndex = 0;
	MainWindowPointer = mainWindow;
	NewSongPosition = 0;
	ProgramRunning = true;
	SongPositionBarHeld = false;

	//Initialize events
	SongChangedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	HistoryUpdatedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	//Test this out. DELETE .vs folder,CLEAN and REBUILD solution first
	MainWindowPointer->Title(L"Folderity");
	
	//Initialize the SoundPlayer
	HRESULT hr = MMFSoundPlayer::CreateInstance(&SoundPlayer);
	if (FAILED(hr))
	{
		throw std::exception("Failed to create sound player\n");
	}
	
	//Get LocalApplicationData folder
	std::wstring localFolder = ApplicationData::Current().LocalFolder().Path().c_str();
	
	//If the folderity folder doesn't exist, create it
	fs::path folderityFolder = fs::path(localFolder) / LOCAL_FOLDER_NAME;
	if (!fs::exists(folderityFolder))
	{
		if (!fs::create_directory(folderityFolder))
		{
			throw std::exception("Failed to create folderity application data folder\n");
		}
	}
	
	//Check if the playlist master file exists. If it doesn't create it
	fs::path playlistMasterFile = folderityFolder / PLAYLIST_MASTER_FILENAME;
	if (!fs::exists(playlistMasterFile))
	{
		//Create the playlist master file
		bool fileCreated = UpdatePlaylistMasterFile();
		if (!fileCreated)
		{
			throw std::exception("Failed to create playlist master file\n");
		}

		//Make Queue file
		fileCreated = UpdateQueueFile();
		if (!fileCreated)
		{
			throw std::exception("Failed to create queue file\n");
		}

		//Make History file
		fileCreated = UpdateHistoryFile();
		if (!fileCreated)
		{
			throw std::exception("Failed to create history file\n");
		}

		//Initialize the queue to hold 100 songs and the list of playlists to have 10 playlists by default
		PlayerQueue.reserve(100);
		AllPlaylists.reserve(10);
		SongHistory.reserve(100);

		return;
	}

	//If it does exist, load all the playlists from the master file
	bool playlistsLoaded = LoadAllPlaylistsFromMasterFile();
	if (!playlistsLoaded)
	{
		throw std::exception("Failed to load playlists from master file\n");
	}

	//Load all the songs from the playlist files
	bool songsLoaded = LoadAllSongsFromPlaylistFiles();
	if (!songsLoaded)
	{
		throw std::exception("Failed to load songs from playlist files\n");
	}

	//Ensure all playlists are up to date
	for (Playlist& playlist : AllPlaylists)
	{
		bool checkSuccessful = CheckForAndHandleAddedOrRemovedSongs(playlist);
		if (!checkSuccessful)
		{
			throw std::exception("Failed to update playlist order file\n");
		}
	}

	//Load in the queue from the queue file
	bool queueLoaded = LoadQueueSongsFromQueueFile();
	if (!queueLoaded)
	{
		throw std::exception("Failed to load queue from queue file\n");
	}

	//Load in the history from the history file
	bool historyLoaded = LoadHistoryFromHistoryFile();
	if (!historyLoaded)
	{
		throw std::exception("Failed to load history from history file\n");
	}
}

MusicController::~MusicController()
{
	//TODO
}

void MusicController::EventThread()
{
	while (ProgramRunning)
	{
		while(GetPlayerState() != PlayerState::Ready)
		{
			//TODO: Update current position of song as long as the bar isn't being held
			if (!SongPositionBarHeld)
			{
				
			}
			
			//Sleep for 100 ms so CPU doesn't freak out
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		//When it is ready, go to the next song
	}
}

//File IO Initialization helpers-------------------------------------------------------------------
/// <summary>
/// Loads every single playlist from the master file into the AllPlaylists vector. If a playlist in 
/// the master file no longer exists, it will be removed from the master file. This function is only
/// called during initialization.
/// </summary>
/// <returns>If the master file was able to be opened and if needbe, updated</returns>
bool MusicController::LoadAllPlaylistsFromMasterFile()
{
	//Get LocalApplicationData folder
	std::wstring localFolder = ApplicationData::Current().LocalFolder().Path().c_str();

	//Get the playlist master file path
	fs::path playlistMasterFile = fs::path(localFolder) / LOCAL_FOLDER_NAME / PLAYLIST_MASTER_FILENAME;

	//Open the playlist master file
	std::ifstream inputReader;
	inputReader.open(playlistMasterFile, std::ios::binary);
	inputReader >> std::noskipws;
	if (!inputReader)
	{
		return false;
	}

	//Read the number of playlists from the file
	uint64_t numPlaylists = 0;
	inputReader.read(reinterpret_cast<char*>(&numPlaylists), sizeof(uint64_t));

	//Reserve the number of playlists in the vector, plus 10 more for efficiency (almost impossible to have more than 2^64 -1 playlists)
	AllPlaylists.reserve(numPlaylists+10);

	//Read the playlist paths from the file. If the playlist no longer exists, skip it and signal master file for updating
	bool masterFileNeedsUpdating = false;
	for (uint64_t index = 0; index < numPlaylists; index++)
	{
		//Read the length of the playlist path (including +1 for null) as a uint_32_t
		uint32_t playlistPathLength = 0;
		inputReader.read(reinterpret_cast<char*>(&playlistPathLength), sizeof(uint32_t));

		//Read the playlist path
		std::wstring playlistPath;
		playlistPath.resize(playlistPathLength);
		inputReader.read(reinterpret_cast<char*>(playlistPath.data()), playlistPathLength * sizeof(wchar_t));
		
		//Check if playlist (file) exists and if the master file needs updating
		bool playlistExists = fs::exists(fs::path(playlistPath)/PLAYLIST_SONG_ORDER_FILENAME);
		masterFileNeedsUpdating = masterFileNeedsUpdating || !playlistExists;

		//If playlist doesn't exist, add the playlist to the list of playlists
		if (playlistExists)
		{
			AllPlaylists.push_back(Playlist{ playlistPath, std::vector<std::wstring>() });
		}
	}

	//Close up the reader
	inputReader.close();

	//If the master file needs updating, update it
	if (masterFileNeedsUpdating)
	{
		bool masterFileUpdated = UpdatePlaylistMasterFile();
		if (!masterFileUpdated)
		{
			return false;
		}
	}

	return true;
}

/// <summary>
/// Loads every single song from every single playlist order file into the playlist object's song vector.
/// This is called only during initialization.
/// </summary>
/// <returns>If the playlist order file was successfully read</returns>
bool MusicController::LoadAllSongsFromPlaylistFiles()
{
	//Iterate through every playlist and load the songs from the playlist files
	for (Playlist& playlist : AllPlaylists)
	{
		//Get the playlist file path
		fs::path playlistFile = fs::path(playlist.playlistPath) / PLAYLIST_SONG_ORDER_FILENAME;

		//Open the playlist file
		std::ifstream inputReader;
		inputReader.open(playlistFile, std::ios::binary);
		inputReader >> std::noskipws;
		if (!inputReader)
		{
			return false;
		}

		//Read the number of songs from the file
		uint64_t numSongs = 0;
		inputReader.read(reinterpret_cast<char*>(&numSongs), sizeof(uint64_t));

		//Reserve the number of songs (with 10 extra spaces for more songs)
		playlist.songNamesWithExtension.reserve(numSongs+10);

		//Read the song paths from the file and add their names to the song list of each playlist
		for (uint64_t index = 0; index < numSongs; index++)
		{
			//Read the length of the song path (including +1 for null) as a uint_32_t
			uint32_t songPathLength = 0;
			inputReader.read(reinterpret_cast<char*>(&songPathLength), sizeof(uint32_t));

			//Read the song name (with extension)
			std::wstring songName;
			songName.resize(songPathLength);
			inputReader.read(reinterpret_cast<char*>(songName.data()), songPathLength * sizeof(wchar_t));

			//Add the song name to the playlist's song list
			playlist.songNamesWithExtension.push_back(songName);
		}

		//Close up the reader
		inputReader.close();
	}

	return true;
}

/// <summary>
/// Takes every song from the queue file and places them into the queue vector. If the song no longer exists, 
/// it will be removed from the queue file.
/// </summary>
/// <returns>Whether the queue file could successfully be loaded or not and updated if needbe</returns>
bool MusicController::LoadQueueSongsFromQueueFile()
{
	//Get LocalApplicationData folder
	std::wstring localFolder = ApplicationData::Current().LocalFolder().Path().c_str();

	//Get the queue file path
	fs::path queueFile = fs::path(localFolder) / LOCAL_FOLDER_NAME / QUEUE_FILENAME;
	
	//Open the queue file
	std::ifstream inputReader;
	inputReader.open(queueFile, std::ios::binary);
	inputReader >> std::noskipws;
	if (!inputReader)
	{
		return false;
	}

	//Read the index of the current song in the queue
	inputReader.read(reinterpret_cast<char*>(&CurrentSongIndex), sizeof(uint64_t));

	//Read the number of songs from the file
	uint64_t numSongs = 0;
	inputReader.read(reinterpret_cast<char*>(&numSongs), sizeof(uint64_t));
	
	//Reserve the queue vector for the number of songs (plus 10 for efficiency)
	PlayerQueue.reserve(numSongs + 10);

	//Read the song paths from the file and add their names to PlayerQueue
	bool queueFileNeedsUpdating = false;
	for (uint64_t index = 0; index < numSongs; index++)
	{
		//Read the length of the song path (including +1 for null) as a uint_32_t
		uint32_t songPathLength = 0;
		inputReader.read(reinterpret_cast<char*>(&songPathLength), sizeof(uint32_t));

		//Read the song path
		std::wstring songPath;
		songPath.resize(songPathLength);
		inputReader.read(reinterpret_cast<char*>(songPath.data()), songPathLength * sizeof(wchar_t));

		//Check if song exists and if the queue file needs updating
		bool songExists = fs::exists(songPath);
		queueFileNeedsUpdating = queueFileNeedsUpdating || !songExists;

		//Get the playlist path from the song path
		fs::path playlistFileOrderPath = fs::path(songPath).parent_path() / PLAYLIST_SONG_ORDER_FILENAME;

		//Ensure playlist still actually exists, if not, signal queue to be updated and song not added to queue
		bool playlistExists = fs::exists(playlistFileOrderPath);
		queueFileNeedsUpdating = queueFileNeedsUpdating || !playlistExists;

		//If song exists, add it to the queue
		if (songExists && playlistExists)
		{
			Song song;
			song.songNameWithExtension = fs::path(songPath).filename().wstring();
			song.playlistPath = fs::path(songPath).parent_path().wstring();
			PlayerQueue.push_back(song);
		}
	}

	//Close up the reader
	inputReader.close();

	//If the queue file needs updating, update it
	if (queueFileNeedsUpdating)
	{
		bool queueFileUpdated = UpdateQueueFile();
		if (!queueFileUpdated)
		{
			return false;
		}
	}

	return true;
}

bool MusicController::LoadHistoryFromHistoryFile()
{
	//Get LocalApplicationData folder
	std::wstring localFolder = ApplicationData::Current().LocalFolder().Path().c_str();

	//Get the history file path
	fs::path historyFile = fs::path(localFolder) / LOCAL_FOLDER_NAME / HISTORY_FILENAME;

	//Open the history file
	std::ifstream inputReader;
	inputReader.open(historyFile, std::ios::binary);
	inputReader >> std::noskipws;
	if (!inputReader)
	{
		return false;
	}

	//Read the number of songs from the file
	uint64_t numSongs = 0;
	inputReader.read(reinterpret_cast<char*>(&numSongs), sizeof(uint64_t));

	//Reserve the history vector for the number of songs (plus 10 for efficiency)
	SongHistory.reserve(numSongs + 10);

	//Read the song paths from the file and add their names to History
	for (uint64_t index = 0; index < numSongs; index++)
	{
		//Read the length of the song path (including +1 for null) as a uint_32_t
		uint32_t songPathLength = 0;
		inputReader.read(reinterpret_cast<char*>(&songPathLength), sizeof(uint32_t));

		//Read the song path
		std::wstring songPath;
		songPath.resize(songPathLength);
		inputReader.read(reinterpret_cast<char*>(songPath.data()), songPathLength * sizeof(wchar_t));

		//Add song to History
		Song song;
		song.songNameWithExtension = fs::path(songPath).filename().wstring();
		song.playlistPath = fs::path(songPath).parent_path().wstring();
		SongHistory.push_back(song);
	}

	//Close up the reader
	inputReader.close();

	return true;
}

//File IO General helpers--------------------------------------------------------------------------
/// <summary>
/// Takes the current list of playlists and writes them to the master file. Does not check whether
/// the current list of playlists is still up to date. That must be done elsewhere.
/// </summary>
/// <returns>If the master file was successfully able to be opened for writing</returns>
bool MusicController::UpdatePlaylistMasterFile()
{
	//Get LocalApplicationData folder
	std::wstring localFolder = ApplicationData::Current().LocalFolder().Path().c_str();

	//Get the playlist master file path
	fs::path playlistMasterFile = fs::path(localFolder) / LOCAL_FOLDER_NAME / PLAYLIST_MASTER_FILENAME;

	//Open the playlist master file
	std::ofstream outputWriter;
	outputWriter.open(playlistMasterFile, std::ios::binary);
	if (!outputWriter)
	{
		return false;
	}

	//Write the number of playlists to the file
	uint64_t numPlaylists = AllPlaylists.size();
	outputWriter.write(reinterpret_cast<char*>(&numPlaylists), sizeof(uint64_t));

	//Write the playlist paths to the file
	for (const Playlist& playlist : AllPlaylists)
	{
		//Write the length of the playlist path (including +1 for null) as a uint_32_t
		uint32_t playlistPathLength = playlist.playlistPath.length() + 1;
		outputWriter.write(reinterpret_cast<char*>(&playlistPathLength), sizeof(uint32_t));

		//Write the playlist path
		outputWriter.write(reinterpret_cast<const char*>(playlist.playlistPath.c_str()), playlistPathLength * sizeof(wchar_t));
	}

	//Close up the writer
	outputWriter.close();

	return true;
}

/// <summary>
/// Takes the current list of songs in the queue and writes them to the queue file. Does not check whether
/// the current list of songs in the queue is still up to date. That must be done elsewhere.
/// </summary>
/// <returns>If the queue file was successfully opened up</returns>
bool MusicController::UpdateQueueFile()
{
	//Get LocalApplicationData folder
	std::wstring localFolder = ApplicationData::Current().LocalFolder().Path().c_str();

	//Get the queue file path
	fs::path queueFile = fs::path(localFolder) / LOCAL_FOLDER_NAME / QUEUE_FILENAME;

	//Open the queue file
	std::ofstream outputWriter;
	outputWriter.open(queueFile, std::ios::binary);
	if (!outputWriter)
	{
		return false;
	}
	
	//Write the index of the current song in the queue
	outputWriter.write(reinterpret_cast<char*>(&CurrentSongIndex), sizeof(uint64_t));

	//Write the number of songs to the file
	uint64_t numSongs = PlayerQueue.size();
	outputWriter.write(reinterpret_cast<char*>(&numSongs), sizeof(uint64_t));
	
	//Write the song paths to the file
	for (const Song& song : PlayerQueue)
	{
		//Get the song path
		fs::path songPath = fs::path(song.playlistPath) / song.songNameWithExtension;

		//Write the length of the song path (including +1 for null) as a uint_32_t
		uint32_t songPathLength = songPath.wstring().length() + 1;
		outputWriter.write(reinterpret_cast<char*>(&songPathLength), sizeof(uint32_t));

		//Write the song path
		outputWriter.write(reinterpret_cast<const char*>(songPath.wstring().c_str()), songPathLength * sizeof(wchar_t));
	}
	
	//Close up the writer
	outputWriter.close();
	
	return true;
}

bool MusicController::UpdateHistoryFile()
{
	//Get LocalApplicationData folder
	std::wstring localFolder = ApplicationData::Current().LocalFolder().Path().c_str();

	//Get the history file path
	fs::path historyFile = fs::path(localFolder) / LOCAL_FOLDER_NAME / HISTORY_FILENAME;

	//Open the history file
	std::ofstream outputWriter;
	outputWriter.open(historyFile, std::ios::binary);
	if (!outputWriter)
	{
		return false;
	}

	//Write the number of songs to the file
	uint64_t numSongs = SongHistory.size();
	outputWriter.write(reinterpret_cast<char*>(&numSongs), sizeof(uint64_t));

	//Write the song paths to the file
	for (const Song& song : SongHistory)
	{
		//Get the song path
		fs::path songPath = fs::path(song.playlistPath) / song.songNameWithExtension;

		//Write the length of the song path (including +1 for null) as a uint_32_t
		uint32_t songPathLength = songPath.wstring().length() + 1;
		outputWriter.write(reinterpret_cast<char*>(&songPathLength), sizeof(uint32_t));

		//Write the song path
		outputWriter.write(reinterpret_cast<const char*>(songPath.wstring().c_str()), songPathLength * sizeof(wchar_t));
	}

	//Close up the writer
	outputWriter.close();

	return true;
}

/// <summary>
/// Scans the playlist object for songs that have been added or removed from the playlist and
/// updates the playlist object and order file accordingly.
/// </summary>
/// <param name="playlistObject">Reference to object that shall be updated, should there be added or removed songs</param>
/// <returns>Signals to the caller that everything was handled successfully</returns>
bool MusicController::CheckForAndHandleAddedOrRemovedSongs(Playlist& playlistObject)
{
	//Initialize return variable
	bool playlistAltered = false;
	
	//Cycle through every song in the playlist and check if it exists. If it does not, remove it from object
	for (auto songIterator = playlistObject.songNamesWithExtension.begin(); songIterator != playlistObject.songNamesWithExtension.end();)
	{
		//Get the song path
		fs::path songPath = fs::path(playlistObject.playlistPath) / *songIterator;
		
		//Check if the song exists
		if (!fs::exists(songPath))
		{
			//If the song does not exist, remove it from the playlist object
			songIterator = playlistObject.songNamesWithExtension.erase(songIterator);
			playlistAltered = true;
		}
		else
		{
			//If the song does exist, move on to the next song
			songIterator++;
		}
	}	
	
	//Check every mp3 and wav file in the playlist folder and add it to the playlist object if it is not already there
	//FIX LATER THIS IS INEFFICIENT!!!!!!!!!!!!!!!!!!!
	for (const fs::directory_entry& entry : fs::directory_iterator(playlistObject.playlistPath))
	{
		//Get the file extension
		std::wstring fileExtension = entry.path().extension().wstring();

		//Lowercase the file extension 
		std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

		//Check if the file is an mp3 or wav file 
		if (fileExtension == L".mp3" || fileExtension == L".wav")
		{
			//Get the file name with extension
			std::wstring fileNameWithExtension = entry.path().filename().wstring();

			//Check if the file name is already in the playlist object
			bool fileAlreadyInPlaylist = false;
			for (const std::wstring& songName : playlistObject.songNamesWithExtension)
			{
				if (songName == fileNameWithExtension)
				{
					fileAlreadyInPlaylist = true;
					break;
				}
			}

			//If the file is not already in the playlist object, add it
			if (!fileAlreadyInPlaylist)
			{
				playlistObject.songNamesWithExtension.push_back(fileNameWithExtension);
				playlistAltered = true;
			}
		}
	}

	//If playlist was altered, update the order file
	if (playlistAltered)
	{
		bool sucessfullyUpdated = UpdatePlaylistOrderFile(playlistObject);
		return sucessfullyUpdated;
	}
	
	return true;
}

/// <summary>
/// Takes the song vector from the given playlist object and writes them to the playlist file. Does not check whether
/// the current list of songs is still up to date. That must be done elsewhere.
/// </summary>
/// <param name="playlistObject">Playlist object from AllPlaylists vector</param>
/// <returns>Whether or not the playlist order file was able to be opneed for writing.</returns>
bool MusicController::UpdatePlaylistOrderFile(const Playlist& playlistObject)
{
	//Open the playlist file
	std::ofstream outputWriter;
	outputWriter.open(playlistObject.playlistPath, std::ios::binary);
	if (!outputWriter)
	{
		return false;
	}

	//Write the number of songs to the file
	uint64_t numSongs = playlistObject.songNamesWithExtension.size();
	outputWriter.write(reinterpret_cast<char*>(&numSongs), sizeof(uint64_t));

	//Write the song paths to the file
	for (const std::wstring& songName : playlistObject.songNamesWithExtension)
	{
		//Write the length of the song path (including +1 for null) as a uint_32_t
		uint32_t songPathLength = songName.length() + 1;
		outputWriter.write(reinterpret_cast<char*>(&songPathLength), sizeof(uint32_t));

		//Write the song path
		outputWriter.write(reinterpret_cast<const char*>(songName.data()), songPathLength * sizeof(wchar_t));
	}

	//Close up the writer
	outputWriter.close();
	
	return true;
}

/// <summary>
/// Clears out the current queue and plops the songs from the given playlist into the queue.
/// </summary>
/// <param name="playlistObject">Playlist to be plopped</param>
void MusicController::LoadPlaylistIntoQueue(const Playlist& playlistObject)
{
	//Clear the queue
	PlayerQueue.clear();

	//Add all songs from the playlist to the queue
	for (const std::wstring& songName : playlistObject.songNamesWithExtension)
	{
		//Create a new song object
		Song newSong;
		newSong.playlistPath = playlistObject.playlistPath;
		newSong.songNameWithExtension = songName;
		
		//Add the song to the queue
		PlayerQueue.push_back(newSong);
	}

	//Update the queue file
	UpdateQueueFile();

	//Update the current song index
	CurrentSongIndex = 0;
}

//General Helpers----------------------------------------------------------------------------------
std::wstring MusicController::Convert100NanoSecondsToTimestamp(UINT64 input100NanoSeconds)
{
	UINT64 seconds = SoundPlayer->GetCurrentPresentationTime_100NanoSecondUnits() / OneSecond_100NanoSecondUnits;
	UINT64 minutes = seconds / 60;
	seconds = seconds % 60;
	return std::format(L"{:02}:{:02}", minutes, seconds);
}

//Getters------------------------------------------------------------------------------------------
PlayerState MusicController::GetPlayerState()
{
	return SoundPlayer->GetPlayerState();
}

//Setters------------------------------------------------------------------------------------------

//Player Controls----------------------------------------------------------------------------------

//Dispatcher fuctions------------------------------------------------------------------------------
void MusicController::DispatchSongTitle(std::wstring title)
{
	if (MainWindowPointer->DispatcherQueue().HasThreadAccess())
	{
		MainWindowPointer->SongNameBlock().Text(title);
	}
	else
	{
		bool isQueued = MainWindowPointer->DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, [title, this]()
			{
				MainWindowPointer->SongNameBlock().Text(title);
			});
	}
}

void MusicController::DispatchPlaylistTitle(std::wstring title)
{
	if (MainWindowPointer->DispatcherQueue().HasThreadAccess())
	{
		MainWindowPointer->PlaylistNameBlock().Text(title);
	}
	else
	{
		bool isQueued = MainWindowPointer->DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, [title, this]()
			{
				MainWindowPointer->PlaylistNameBlock().Text(title);
			});
	}
}

void MusicController::DispatchCurrentSongPositionAndDuration()
{
	//Convert positions to timestamps
	std::wstring currentTimestamp = Convert100NanoSecondsToTimestamp(SoundPlayer->GetCurrentPresentationTime_100NanoSecondUnits());
	std::wstring totalTimestamp = Convert100NanoSecondsToTimestamp(SoundPlayer->GetAudioFileDuration_100NanoSecondUnits());
	
	//Get percent (0.0 - 1.0) for song position
	double songPositionPercent = (SoundPlayer->GetCurrentPresentationTime_100NanoSecondUnits() * 100) / SoundPlayer->GetAudioFileDuration_100NanoSecondUnits();
	
	if (MainWindowPointer->DispatcherQueue().HasThreadAccess())
	{
		MainWindowPointer->CurrentTimestampBlock().Text(currentTimestamp);
		MainWindowPointer->TrackBar().Value(songPositionPercent);
		MainWindowPointer->TotalSongDurationBlock().Text(totalTimestamp);
	}
	else
	{
		bool isQueued = MainWindowPointer->DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, [this, currentTimestamp, songPositionPercent, totalTimestamp]()
			{
				MainWindowPointer->CurrentTimestampBlock().Text(currentTimestamp);
				MainWindowPointer->TrackBar().Value(songPositionPercent);
				MainWindowPointer->TotalSongDurationBlock().Text(totalTimestamp);
			});
	}
}

void MusicController::DispatchPreviousButtonToggle(bool isEnabled)
{
	if (MainWindowPointer->DispatcherQueue().HasThreadAccess())
	{ 
		MainWindowPointer->PreviousButton().IsEnabled(isEnabled);
	}
	else
	{
		bool isQueued = MainWindowPointer->DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, [isEnabled, this]()
			{
				MainWindowPointer->PreviousButton().IsEnabled(isEnabled);
			});
	}
}

void MusicController::DispatchNextButtonToggle(bool isEnabled)
{
	if (MainWindowPointer->DispatcherQueue().HasThreadAccess())
	{
		MainWindowPointer->NextButton().IsEnabled(isEnabled);
	}
	else
	{
		bool isQueued = MainWindowPointer->DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, [isEnabled, this]()
			{
				MainWindowPointer->NextButton().IsEnabled(isEnabled);
			});
	}
}

void MusicController::DispatchPlayButtonToggle(bool isEnabled)
{
	if (MainWindowPointer->DispatcherQueue().HasThreadAccess())
	{
		MainWindowPointer->PlayPauseButton().IsEnabled(isEnabled);
	}
	else
	{
		bool isQueued = MainWindowPointer->DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, [isEnabled, this]()
			{
				MainWindowPointer->PlayPauseButton().IsEnabled(isEnabled);
			});
	}
}

void MusicController::DispatchTrackBarToggle(bool isEnabled)
{
	if (MainWindowPointer->DispatcherQueue().HasThreadAccess())
	{
		MainWindowPointer->TrackBar().IsEnabled(isEnabled);
	}
	else
	{
		bool isQueued = MainWindowPointer->DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, [isEnabled, this]()
			{
				MainWindowPointer->TrackBar().IsEnabled(isEnabled);
			});
	}
}