#include "pch.h"
#include "MusicController.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <format>
#include <thread>
#include <SharedWindowVariables.h>

using namespace MMFSoundPlayerLib;
using namespace winrt::Windows::Storage;
namespace fs = std::filesystem;

//Constructors and Destructors---------------------------------------------------------------------
MusicController::MusicController(winrt::Folderify::implementation::MainWindow* mainWindow)
{
	//Initialize variables
	CurrentSongIndex = -1;
	MainWindowPointer = mainWindow;
	IsLoopEnabled = false;
	ProgramRunning = true;
	SongPositionBarHeld = false;
	TrackbarRange = MainWindowPointer->TrackBar().Maximum() - MainWindowPointer->TrackBar().Minimum();

	//Initialize Queue Page Events
	QueueThreadRunning = false;
	for (int index = 0; index < static_cast<int>(QueuePageEventEnums::NumberOfEvents); index++)
	{
		QueuePageEvents[index] = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (QueuePageEvents[index] == nullptr)
		{
			throw std::exception("Failed to create QueuePageEvents\n");
		}
	}
	
	//Initialize History Page Events
	HistoryThreadRunning = false;
	for (int index = 0; index < static_cast<int>(HistoryPageEventEnums::NumberOfEvents); index++)
	{
		HistoryPageEvents[index] = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (HistoryPageEvents[index] == nullptr)
		{
			throw std::exception("Failed to create HistoryPageEvents\n");
		}
	}

	//Initialize mutex
	QueueMutex = CreateMutex(NULL, FALSE, NULL);
	if (QueueMutex == nullptr)
	{
		throw std::exception("Failed to create QueueMutex\n");
	}
	
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

	//Launch event thread that shall manage song changes and TrackBar updates
	EventThreadObject = std::thread{ &MusicController::EventThreadProc, this };
}

void MusicController::CloseController()
{
	//Signal to the dispatcher that the window is closing
	WindowClosing = true;
	
	//Stop any songs playing
	SoundPlayer->Stop();
	
	//Close event loop
	ProgramRunning = false;
	EventThreadObject.join();
	
	//Close the music player
	SoundPlayer->Shutdown();

	//Signal the Queue and History pages to close down and then, close up all handles controlled by the MusicController
	if (QueueThreadRunning)
	{
		SetEvent(QueuePageEvents[static_cast<int>(QueuePageEventEnums::PageClosing)]);
		WaitForSingleObject(QueuePageEvents[static_cast<int>(QueuePageEventEnums::PageClosed)], INFINITE);
	}
	for (int index = 0; index < static_cast<int>(QueuePageEventEnums::NumberOfEvents); index++)
	{
		CloseHandle(QueuePageEvents[index]);
	}
	
	if (HistoryThreadRunning)
	{
		SetEvent(HistoryPageEvents[static_cast<int>(HistoryPageEventEnums::PageClosing)]);
		WaitForSingleObject(HistoryPageEvents[static_cast<int>(HistoryPageEventEnums::PageClosed)], INFINITE);
	}
	for (int index = 0; index < static_cast<int>(HistoryPageEventEnums::NumberOfEvents); index++)
	{
		CloseHandle(HistoryPageEvents[index]);
	}
	
	CloseHandle(QueueMutex);
	
	//Save up the queue file
	UpdateQueueFile();
}

void MusicController::EventThreadProc()
{
	//Try to set the song
	LoadSongIntoPlayer(CurrentSongIndex);

	//Immediately pause the song
	Pause();

	//Main program loop
	while (ProgramRunning)
	{
		//Update current position of song until song is done
		while(GetPlayerState() == PlayerState::Playing || GetPlayerState() == PlayerState::Paused)
		{
			//Update current position of song as long as the bar isn't being held
			if (!SongPositionBarHeld)
			{
				DispatchCurrentSongPositionAndDuration();
			}
			
			//Sleep for 100 ms so CPU doesn't freak out
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		//Load next song in the queue if and only if the old song ended
		if (GetPlayerState() == PlayerState::PresentationEnd)
		{
			if (IsLoopEnabled)
			{
				LoadSongIntoPlayer(CurrentSongIndex);
			}
			else
			{
				LoadSongIntoPlayer(CurrentSongIndex + 1);
			}
		}

		//Sleep for 100 ms so CPU doesn't freak out
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
	uint32_t numPlaylists = 0;
	inputReader.read(reinterpret_cast<char*>(&numPlaylists), sizeof(uint32_t));

	//Reserve the number of playlists in the vector, plus 10 more for efficiency (almost impossible to have more than 2^64 -1 playlists)
	AllPlaylists.reserve(numPlaylists+10);

	//Read the playlist paths from the file. If the playlist no longer exists, skip it and signal master file for updating
	bool masterFileNeedsUpdating = false;
	for (uint32_t index = 0; index < numPlaylists; index++)
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
		if (!LoadSongsFromSinglePlaylist(playlist))
		{
			return false;
		}
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
	inputReader.read(reinterpret_cast<char*>(&CurrentSongIndex), sizeof(int32_t));

	//Read the number of songs from the file
	uint32_t numSongs = 0;
	inputReader.read(reinterpret_cast<char*>(&numSongs), sizeof(uint32_t));
	
	//Reserve the queue vector for the number of songs (plus 10 for efficiency)
	PlayerQueue.reserve(numSongs + 10);

	//Read the song paths from the file and add their names to PlayerQueue
	bool queueFileNeedsUpdating = false;
	for (uint32_t index = 0; index < numSongs; index++)
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
	uint32_t numSongs = 0;
	inputReader.read(reinterpret_cast<char*>(&numSongs), sizeof(uint32_t));

	//Reserve the history vector for the number of songs (plus 10 for efficiency)
	SongHistory.reserve(numSongs + 10);

	//Read the song paths from the file and add their names to History
	for (uint32_t index = 0; index < numSongs; index++)
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
/// Takes a new playlist object with only the path and no songs in it yet and fills it with the songs
/// at that path. NOTE: It must have a playlist file in its path already
/// </summary>
/// <param name="newPlaylist">New playlist object with NO SONGS but, already has a Playlist file</param>
/// <returns>Whether or not the playlist was populated</returns>
bool MusicController::LoadSongsFromSinglePlaylist(Playlist& newPlaylist)
{
	//Get the playlist file path and ensure it exists
	fs::path playlistFile = fs::path(newPlaylist.playlistPath) / PLAYLIST_SONG_ORDER_FILENAME;
	if (!fs::exists(playlistFile))
	{
		return false;
	}

	//Open the playlist file
	std::ifstream inputReader;
	inputReader.open(playlistFile, std::ios::binary);
	inputReader >> std::noskipws;
	if (!inputReader)
	{
		return false;
	}

	//Read the number of songs from the file
	uint32_t numSongs = 0;
	inputReader.read(reinterpret_cast<char*>(&numSongs), sizeof(uint32_t));

	//Reserve the number of songs (with 10 extra spaces for more songs)
	newPlaylist.songNamesWithExtension.reserve(numSongs + 10);

	//Read the song paths from the file and add their names to the song list of each playlist
	for (uint32_t index = 0; index < numSongs; index++)
	{
		//Read the length of the song path (including +1 for null) as a uint_32_t
		uint32_t songPathLength = 0;
		inputReader.read(reinterpret_cast<char*>(&songPathLength), sizeof(uint32_t));

		//Read the song name (with extension)
		std::wstring songName;
		songName.resize(songPathLength);
		inputReader.read(reinterpret_cast<char*>(songName.data()), songPathLength * sizeof(wchar_t));

		//Add the song name to the playlist's song list
		newPlaylist.songNamesWithExtension.push_back(songName);
	}

	//Close up the reader
	inputReader.close();
	return true;
}

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
	uint32_t numPlaylists = AllPlaylists.size();
	outputWriter.write(reinterpret_cast<char*>(&numPlaylists), sizeof(uint32_t));

	//Write the playlist paths to the file
	for (const Playlist& playlist : AllPlaylists)
	{
		//Write the length of the playlist path as a uint_32_t
		uint32_t playlistPathLength = playlist.playlistPath.length();
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
	outputWriter.write(reinterpret_cast<char*>(&CurrentSongIndex), sizeof(int32_t));

	//Write the number of songs to the file
	uint32_t numSongs = PlayerQueue.size();
	outputWriter.write(reinterpret_cast<char*>(&numSongs), sizeof(uint32_t));
	
	//Write the song paths to the file
	for (const Song& song : PlayerQueue)
	{
		//Get the song path
		fs::path songPath = fs::path(song.playlistPath) / song.songNameWithExtension;

		//Write the length of the song path (including +1 for null) as a uint_32_t
		uint32_t songPathLength = songPath.wstring().length();
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
	uint32_t numSongs = SongHistory.size();
	outputWriter.write(reinterpret_cast<char*>(&numSongs), sizeof(uint32_t));

	//Write the song paths to the file
	for (const Song& song : SongHistory)
	{
		//Get the song path
		fs::path songPath = fs::path(song.playlistPath) / song.songNameWithExtension;

		//Write the length of the song path (including +1 for null) as a uint_32_t
		uint32_t songPathLength = songPath.wstring().length();
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
	outputWriter.open(fs::path(playlistObject.playlistPath) / PLAYLIST_SONG_ORDER_FILENAME, std::ios::binary);
	if (!outputWriter)
	{
		return false;
	}

	//Write the number of songs to the file
	uint32_t numSongs = playlistObject.songNamesWithExtension.size();
	outputWriter.write(reinterpret_cast<char*>(&numSongs), sizeof(uint32_t));

	//Write the song paths to the file
	for (const std::wstring& songName : playlistObject.songNamesWithExtension)
	{
		//Write the length of the song path as a uint_32_t
		uint32_t songPathLength = songName.length();
		outputWriter.write(reinterpret_cast<char*>(&songPathLength), sizeof(uint32_t));

		//Write the song path
		outputWriter.write(reinterpret_cast<const char*>(songName.data()), songPathLength * sizeof(wchar_t));
	}

	//Close up the writer
	outputWriter.close();
	
	return true;
}

//Song Loaders-------------------------------------------------------------------------------------

/// <summary>
/// Takes the index and tries to play that song in the queue. If unable to play that song, delete it
/// and traverse queue until a playable song is found and played. If no playable song is found, the queue
/// will be empty. The CurrentIndex will be set to the index of the song currently playing or 0 if it is empty.
/// </summary>
/// <param name="index">The index of the song playing</param>
void MusicController::LoadSongIntoPlayer(uint32_t index)
{
	//Ensure only one thread is changing around the queue and starting songs at once
	WaitForSingleObject(QueueMutex, INFINITE);
	
	if (!PlayerQueue.empty())
	{
		uint32_t oldNumSongsInQueue = PlayerQueue.size();
		int32_t oldIndex = CurrentSongIndex;
		bool successfullySetSong;
		do
		{
			//Ensure the song index is within range
			if (!(index < PlayerQueue.size()))
			{
				index = 0;
			}

			//Try to load up the song into the player. If not possible, delete it and move on
			successfullySetSong = LoadSongIntoPlayer_Aux(index);
			if (!successfullySetSong)
			{
				PlayerQueue.erase(PlayerQueue.begin() + index);

				//If the queue is now empty, set the index back to default 0 and exit
				if (PlayerQueue.empty())
				{
					index = 0;
					break;
				}
			}
		} while (!successfullySetSong);
		
		//Set the current index as the index of the song that has started playing
		CurrentSongIndex = index;

		//If the number of songs in the queue has changed, signal the QueuePage that the SongList changed
		if (oldNumSongsInQueue != PlayerQueue.size())
		{
			SetEvent(QueuePageEvents[static_cast<int>(QueuePageEventEnums::SongListChanged)]);
		}
		
		else if (oldIndex != CurrentSongIndex)
		{
			SetEvent(QueuePageEvents[static_cast<int>(QueuePageEventEnums::IndexChanged)]);
		}
	}

	//If the queue is empty, set the current song index to default 0
	else
	{
		CurrentSongIndex = -1;
	}

	//Release the mutex
	ReleaseMutex(QueueMutex);
}

/// <summary>
/// Takes the index of a song in the queue, loads that song into the music player and plays it
/// </summary>
/// <param name="index">Index of song in queue</param>
/// <returns>Whether song is loaded or not</returns>
bool MusicController::LoadSongIntoPlayer_Aux(uint32_t index)
{
	//First ensure the index is a valid index
	if (!(index < PlayerQueue.size()))
	{
		return false;
	}

	//Stop the session just in case it is playing a song
	HRESULT hr = SoundPlayer->Stop();
	if (FAILED(hr))
	{
		return false;
	}

	//Disable all the buttons and set image to default while song is being found
	DispatchPreviousButtonToggle(false);
	DispatchPlayButtonToggle(false);
	DispatchNextButtonToggle(false);
	DispatchTrackBarToggle(false);
	DispatchSongTitle(L"Waiting For Song...");
	DispatchPlaylistTitle(L"Waiting For Playlist...");
	//TODO: Set image to default

	//Get the whole path of the song
	fs::path songPath = fs::path(PlayerQueue[index].playlistPath) / PlayerQueue[index].songNameWithExtension;

	//Load the song into the MMFSoundPlayer
	hr = SoundPlayer->SetFileIntoPlayer(songPath.wstring().c_str());
	if (FAILED(hr))
	{
		return false;
	}

	//Change the state of the buttons, song name, playlist name and image
	DispatchPreviousButtonToggle(index != 0);
	DispatchPlayButtonToggle(true);
	DispatchPlayButtonIcon(false);
	DispatchNextButtonToggle(index != (PlayerQueue.size() - 1));
	DispatchTrackBarToggle(true);
	DispatchSongTitle(fs::path(PlayerQueue[index].songNameWithExtension).stem());
	DispatchPlaylistTitle(fs::path(PlayerQueue[index].playlistPath).filename());
	//TODO: Set image to song's filename with either png, jpeg jpg extension

	return true;
}

//General Helpers----------------------------------------------------------------------------------
std::wstring MusicController::Convert100NanoSecondsToTimestamp(UINT64 input100NanoSeconds)
{
	UINT64 seconds = input100NanoSeconds / OneSecond_100NanoSecondUnits;
	UINT64 minutes = seconds / 60;
	seconds = seconds % 60;
	return std::format(L"{:02}:{:02}", minutes, seconds);
}

//Getters------------------------------------------------------------------------------------------
PlayerState MusicController::GetPlayerState()
{
	return SoundPlayer->GetPlayerState();
}

//NOTE: Only done during initialization of PlaylistSelectionPage
void MusicController::GetPlaylistNames(winrt::Folderify::PlaylistSelectionPageViewModel& playlistPageModel)
{
	//Clear out the input vector
	playlistPageModel.Playlists().Clear();

	//Fill in all playlists
	for (uint32_t index = 0; index < AllPlaylists.size(); index++)
	{
		playlistPageModel.Playlists().Append(winrt::Folderify::PlaylistInfo(fs::path(AllPlaylists[index].playlistPath).stem().c_str(), std::to_wstring(AllPlaylists[index].songNamesWithExtension.size())));
	}
}

void MusicController::GetPlaylistSongNames(int32_t playlistIndex, winrt::Folderify::PlaylistSelectionPageViewModel& playlistPageModel)
{
	//Ensure that the playlist index is valid
	bool isPlaylistIndexWithinBounds = (playlistIndex >= 0) && (playlistIndex < AllPlaylists.size());
	if (!isPlaylistIndexWithinBounds)
	{
		return;
	}

	//Check if the number of songs in input is too small or equal to the real amount
	if (playlistPageModel.Songs().Size() <= AllPlaylists[playlistIndex].songNamesWithExtension.size())
	{
		//If too small, update all songs that will fit
		for (uint32_t index = 0; index < playlistPageModel.Songs().Size(); index++)
		{
			playlistPageModel.Songs().SetAt(index, winrt::Folderify::SongInfo((fs::path(AllPlaylists[playlistIndex].playlistPath) / fs::path(AllPlaylists[playlistIndex].songNamesWithExtension[index])).c_str()));
		}
		
		//Append rest of songs if there are any left
		for (uint32_t index = playlistPageModel.Songs().Size(); index < AllPlaylists[playlistIndex].songNamesWithExtension.size(); index++)
		{
			playlistPageModel.Songs().Append(winrt::Folderify::SongInfo((fs::path(AllPlaylists[playlistIndex].playlistPath) / fs::path(AllPlaylists[playlistIndex].songNamesWithExtension[index])).c_str()));
		}
	}
	
	//If input list of songs is too big
	else
	{
		//Update input for all songs in this playlist
		for (uint32_t index = 0; index < AllPlaylists[playlistIndex].songNamesWithExtension.size(); index++)
		{
			playlistPageModel.Songs().SetAt(index, winrt::Folderify::SongInfo((fs::path(AllPlaylists[playlistIndex].playlistPath) / fs::path(AllPlaylists[playlistIndex].songNamesWithExtension[index])).c_str()));
		}

		//Purge the excess elements
		while (playlistPageModel.Songs().Size() > AllPlaylists[playlistIndex].songNamesWithExtension.size())
		{
			playlistPageModel.Songs().RemoveAtEnd();
		}
	}
}

int32_t MusicController::GetCurrentSongIndex()
{
	return CurrentSongIndex;
}

/// <summary>
/// Loads the song names into the queue page model and then returns the index of the current song
/// playing
/// </summary>
/// <param name="queuePageModel">The queue's page model</param>
/// <returns>The currently playing song index</returns>
int32_t MusicController::GetQueueSongNames(winrt::Folderify::QueuePageViewModel& queuePageModel)
{
	//Check if the number of songs in input is too small or equal to the real amount
	if (queuePageModel.Songs().Size() <= PlayerQueue.size())
	{
		//If too small, update all songs that will fit
		for (uint32_t index = 0; index < queuePageModel.Songs().Size(); index++)
		{
			queuePageModel.Songs().SetAt(index, winrt::Folderify::SongInfo((fs::path(PlayerQueue[index].playlistPath) / fs::path(PlayerQueue[index].songNameWithExtension)).c_str()));
		}

		//Append rest of songs if there are any left
		for (uint32_t index = queuePageModel.Songs().Size(); index < PlayerQueue.size(); index++)
		{
			queuePageModel.Songs().Append(winrt::Folderify::SongInfo((fs::path(PlayerQueue[index].playlistPath) / fs::path(PlayerQueue[index].songNameWithExtension)).c_str()));
		}
	}
	
	//If input list of songs is too big
	else
	{
		//Update input for all songs in this playlist
		for (uint32_t index = 0; index < PlayerQueue.size(); index++)
		{
			queuePageModel.Songs().SetAt(index, winrt::Folderify::SongInfo((fs::path(PlayerQueue[index].playlistPath) / fs::path(PlayerQueue[index].songNameWithExtension)).c_str()));
		}

		//Purge the excess elements
		while (queuePageModel.Songs().Size() > PlayerQueue.size())
		{
			queuePageModel.Songs().RemoveAtEnd();
		}
	}

	return CurrentSongIndex;
}

bool MusicController::RefreshPlaylistSongNames(int32_t playlistIndex, winrt::Folderify::PlaylistSelectionPageViewModel& playlistPageModel)
{
	//Ensure that the playlist index is valid
	bool isPlaylistIndexWithinBounds = (playlistIndex >= 0) && (playlistIndex < AllPlaylists.size());
	if (!isPlaylistIndexWithinBounds)
	{
		return false;
	}

	//Try to update playlist
	bool isHandled = CheckForAndHandleAddedOrRemovedSongs(AllPlaylists[playlistIndex]);
	if (!isHandled)
	{
		//If the playlist update wasn't handled properly, it means the playlist was deleted or something similar
		AllPlaylists.erase(AllPlaylists.begin() + playlistIndex);
		playlistPageModel.Playlists().RemoveAt(playlistIndex);
		return false;
	}

	//If the playlist is updated, update the model for the playlist
	GetPlaylistSongNames(playlistIndex, playlistPageModel);

	//Update the playlist's count in the model
	playlistPageModel.Playlists().SetAt(playlistIndex, winrt::Folderify::PlaylistInfo(fs::path(AllPlaylists[playlistIndex].playlistPath).stem().c_str(), std::to_wstring(AllPlaylists[playlistIndex].songNamesWithExtension.size())));
	
	return true;
}

HWND MusicController::GetWindowHandle()
{
	HWND hwnd;
	winrt::Microsoft::UI::Xaml::Window window = MainWindowPointer->try_as<winrt::Microsoft::UI::Xaml::Window>();
	window.as<IWindowNative>()->get_WindowHandle(&hwnd);
	return hwnd;
}

//Setters------------------------------------------------------------------------------------------
bool MusicController::CreateNewPlaylist(const std::wstring& newPlaylistFolderPath, winrt::Folderify::PlaylistSelectionPageViewModel& playlistPageModel)
{
	//Check if a playlist already exists in this folder by checking if a playlist order file is there
	fs::path playlistOrderFilePath = fs::path(newPlaylistFolderPath) / PLAYLIST_SONG_ORDER_FILENAME;
	if (fs::exists(playlistOrderFilePath))
	{
		//If the file exists, check if the playlist is already in the list. If it is, take no action
		for (uint32_t index = 0; index < AllPlaylists.size(); index++)
		{
			if (AllPlaylists[index].playlistPath == newPlaylistFolderPath)
			{
				return false;
			}
		}

		//If the playlist isn't in the list, it must be created 
		Playlist newPlaylist{ newPlaylistFolderPath, std::vector<std::wstring>() };
		
		//Load in the songs in the playlist
		if (LoadSongsFromSinglePlaylist(newPlaylist))
		{
			//Add the new playlist to the list of playlists
			AllPlaylists.push_back(newPlaylist);
			
			//Update the master playlist file
			UpdatePlaylistMasterFile();
			
			//If the songs were loaded, add the playlist to the list
			playlistPageModel.Playlists().Append(winrt::Folderify::PlaylistInfo(fs::path(newPlaylistFolderPath).stem().c_str(), std::to_wstring(AllPlaylists[AllPlaylists.size() - 1].songNamesWithExtension.size())));
			
			return true;
		}

		else
		{
			//If the playlist cannot be loaded, delete the existing playlist file and start from scratch
			fs::remove(playlistOrderFilePath);
		}
	}
	
	//Create the new playlist object
	Playlist newPlaylist;
	newPlaylist.playlistPath = newPlaylistFolderPath;

	//Get all MP3s and WAVs in the folder and place them in the playlist
	for (const fs::directory_entry& entry : fs::directory_iterator(newPlaylistFolderPath))
	{
		//Get the file extension
		std::wstring fileExtension = entry.path().extension().wstring();

		//Lowercase the file extension 
		std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
		
		//If the song is an mp3 or wav, add it to the playlist
		if (fileExtension == L".mp3" || fileExtension == L".wav")
		{
			newPlaylist.songNamesWithExtension.emplace_back(entry.path().filename());
		}
	}

	//Add the new playlist to the list of playlists
	AllPlaylists.push_back(newPlaylist);

	//Create the playlist order file for the playlist
	UpdatePlaylistOrderFile(newPlaylist);

	//Update the master playlist file
	UpdatePlaylistMasterFile();
	
	//Update UI
	playlistPageModel.Playlists().Append(winrt::Folderify::PlaylistInfo(fs::path(newPlaylistFolderPath).stem().c_str(), std::to_wstring(newPlaylist.songNamesWithExtension.size())));
	
	//Playlist successfully created
	return true;
}

void MusicController::UpdatePlaylist(int32_t playlistIndex, winrt::Folderify::PlaylistSelectionPageViewModel& playlistPageModel)
{
	//Ensure that the playlist index is valid
	bool isPlaylistIndexWithinBounds = (playlistIndex >= 0) && (playlistIndex < AllPlaylists.size());
	if (!isPlaylistIndexWithinBounds)
	{
		return;
	}
	
	//Iterate through every song in the playlist and if the song has changed, update the playlist
	bool playlistUpdated = false;
	for (uint32_t index = 0; index < AllPlaylists[playlistIndex].songNamesWithExtension.size(); index++)
	{
		//Get the current song
		std::wstring currentSong = AllPlaylists[playlistIndex].songNamesWithExtension[index];

		//Get the new song
		std::wstring newSong = fs::path(playlistPageModel.Songs().GetAt(index).SongPath().c_str()).filename();

		//If the song has changed, update the playlist
		if (currentSong != newSong)
		{
			//Update the playlist
			AllPlaylists[playlistIndex].songNamesWithExtension[index] = newSong;

			//Signal playlist to be updated
			playlistUpdated = true;
		}
	}

	//If the playlist was updated at all, update the playlist order file
	if (playlistUpdated)
	{
		UpdatePlaylistOrderFile(AllPlaylists[playlistIndex]);
	}
}

void MusicController::UpdateQueue(int32_t newQueueIndex, winrt::Folderify::QueuePageViewModel& queuePageModel)
{
	//Ensure the QueuePageModel is valid, by checking that the queue sizes are equal
	assert(queuePageModel.Songs().Size() == PlayerQueue.size());

	//Lock the queue, to ensure nothing else touches it while the PlayerQueue is being updated
	WaitForSingleObject(QueueMutex, INFINITE);

	//Iterate through every song in the queuePageModel and if the corresponding song in the PlayerQueue is different, update the PlayerQueue
	for (uint32_t index = 0; index < queuePageModel.Songs().Size(); index++)
	{
		//Get the current song
		std::wstring currentSong = fs::path(PlayerQueue[index].playlistPath) / fs::path(PlayerQueue[index].songNameWithExtension);

		//Get the new song
		std::wstring newSong = fs::path(queuePageModel.Songs().GetAt(index).SongPath().c_str()).filename();

		//If the song has changed, update the PlayerQueue
		if (currentSong != newSong)
		{
			//Update the PlayerQueue
			PlayerQueue[index].playlistPath = fs::path(queuePageModel.Songs().GetAt(index).SongPath().c_str()).parent_path().wstring();
			PlayerQueue[index].songNameWithExtension = fs::path(queuePageModel.Songs().GetAt(index).SongPath().c_str()).filename().wstring();
		}
	}

	//Ensure that the new queue index is valid, before setting it
	bool isQueueIndexWithinBounds = (newQueueIndex >= 0) && (newQueueIndex < PlayerQueue.size());
	if (!isQueueIndexWithinBounds)
	{
		return;
	}

	//Set the new queue index
	CurrentSongIndex = newQueueIndex;
	DispatchPreviousButtonToggle(CurrentSongIndex != 0);
	DispatchNextButtonToggle(CurrentSongIndex != PlayerQueue.size() - 1);

	//Release the queue
	ReleaseMutex(QueueMutex);
}

void MusicController::AddPlaylistToQueue(int32_t playlistIndex, int32_t startingSongIndex)
{
	//It should be impossible for playlistIndex and startingSongIndex to be out of bounds
	bool isPlaylistIndexWithinBounds = (playlistIndex >= 0) && (playlistIndex < AllPlaylists.size());
	bool isSongIndexWithinBounds = (startingSongIndex >= 0) && (startingSongIndex < AllPlaylists[playlistIndex].songNamesWithExtension.size());
	if (!(isPlaylistIndexWithinBounds && isSongIndexWithinBounds))
	{
		return;
	}
	
	//Ensure only one thread is changing around the queue and starting songs at once
	WaitForSingleObject(QueueMutex, INFINITE);

	//Clear the queue
	PlayerQueue.clear();
	
	//Add the songs to the queue
	for (uint32_t index = 0; index < AllPlaylists[playlistIndex].songNamesWithExtension.size(); index++)
	{
		PlayerQueue.push_back({ .songNameWithExtension = AllPlaylists[playlistIndex].songNamesWithExtension[index], .playlistPath = AllPlaylists[playlistIndex].playlistPath });
	}

	//Release the mutex
	ReleaseMutex(QueueMutex);

	//Set the starting song
	LoadSongIntoPlayer(startingSongIndex);
}

void MusicController::ChangeSongIndex(int32_t newIndex)
{
	//If the selection index of the view is the same as the current song index, do nothing, otherwise, change the song
	if (newIndex != CurrentSongIndex)
	{
		LoadSongIntoPlayer(newIndex);
	}
}
//Player Controls----------------------------------------------------------------------------------
void MusicController::Play()
{
	HRESULT hr = SoundPlayer->Play();
	if (SUCCEEDED(hr))
	{
		DispatchPlayButtonIcon(false);
	}
}

void MusicController::Pause()
{
	//Ensure it is only paused if it is playing
	HRESULT hr = SoundPlayer->Pause();
	if (SUCCEEDED(hr))
	{
		DispatchPlayButtonIcon(true);
	}
}

void MusicController::Previous()
{
	LoadSongIntoPlayer(CurrentSongIndex - 1);
}

void MusicController::Next()
{
	LoadSongIntoPlayer(CurrentSongIndex + 1);
}

void MusicController::Seek(double trackBarValue)
{
	//Check if song is paused paused
	bool isPaused = GetPlayerState() == Paused;

	//Compute the time to pass to the sound player's seek function
	UINT64 seekPosition_100NanoSeconds = (trackBarValue * SoundPlayer->GetAudioFileDuration_100NanoSecondUnits()) / TrackbarRange;
	UINT64 timeRemaining_100NanoSeconds = SoundPlayer->GetAudioFileDuration_100NanoSecondUnits() - seekPosition_100NanoSeconds;

	//Seek to the correct time
	SoundPlayer->Seek(seekPosition_100NanoSeconds);

	//If the song was paused and timestamp was not at the end of the song
	if (isPaused && timeRemaining_100NanoSeconds > OneSecond_100NanoSecondUnits)
	{
		Pause();
	}
	
	else if (isPaused)
	{
		DispatchPlayButtonIcon(false);
	}
}

void MusicController::LoopToggle()
{
	IsLoopEnabled = !IsLoopEnabled;
	DispatchLoopButtonIcon();
	
}

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
	
	//Get percent for song position
	double songPositionPercent = 0;
	if (SoundPlayer->GetAudioFileDuration_100NanoSecondUnits() != 0) //Avoid division by 0
	{
		songPositionPercent = (SoundPlayer->GetCurrentPresentationTime_100NanoSecondUnits() * TrackbarRange) / SoundPlayer->GetAudioFileDuration_100NanoSecondUnits();
	}
	
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
				//Check if window is still open
				if (WindowClosing)
				{
					return;
				}
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

void MusicController::DispatchPlayButtonIcon(bool isPlayIcon)
{
	if (MainWindowPointer->DispatcherQueue().HasThreadAccess())
	{
		if (isPlayIcon)
		{
			MainWindowPointer->PlayPauseButton().Icon(winrt::Microsoft::UI::Xaml::Controls::SymbolIcon(winrt::Microsoft::UI::Xaml::Controls::Symbol::Play));
			winrt::Microsoft::UI::Xaml::Controls::ToolTipService::SetToolTip(MainWindowPointer->PlayPauseButton(), winrt::box_value(L"Play"));
		}
		
		else
		{
			MainWindowPointer->PlayPauseButton().Icon(winrt::Microsoft::UI::Xaml::Controls::SymbolIcon(winrt::Microsoft::UI::Xaml::Controls::Symbol::Pause));
			winrt::Microsoft::UI::Xaml::Controls::ToolTipService::SetToolTip(MainWindowPointer->PlayPauseButton(), winrt::box_value(L"Pause"));

		}
	}
	else
	{
		bool isQueued = MainWindowPointer->DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, [isPlayIcon, this]()
			{
				if (isPlayIcon)
				{
					MainWindowPointer->PlayPauseButton().Icon(winrt::Microsoft::UI::Xaml::Controls::SymbolIcon(winrt::Microsoft::UI::Xaml::Controls::Symbol::Play));
					winrt::Microsoft::UI::Xaml::Controls::ToolTipService::SetToolTip(MainWindowPointer->PlayPauseButton(), winrt::box_value(L"Play"));
				}

				else
				{
					MainWindowPointer->PlayPauseButton().Icon(winrt::Microsoft::UI::Xaml::Controls::SymbolIcon(winrt::Microsoft::UI::Xaml::Controls::Symbol::Pause));
					winrt::Microsoft::UI::Xaml::Controls::ToolTipService::SetToolTip(MainWindowPointer->PlayPauseButton(), winrt::box_value(L"Pause"));
				}
			});
	}
}

void MusicController::DispatchLoopButtonIcon()
{
	if (MainWindowPointer->DispatcherQueue().HasThreadAccess())
	{
		if (IsLoopEnabled)
		{
			MainWindowPointer->LoopButton().Icon(winrt::Microsoft::UI::Xaml::Controls::SymbolIcon(winrt::Microsoft::UI::Xaml::Controls::Symbol::RepeatOne));
			winrt::Microsoft::UI::Xaml::Controls::ToolTipService::SetToolTip(MainWindowPointer->LoopButton(), winrt::box_value(L"Loop Enabled"));
		}

		else
		{
			MainWindowPointer->LoopButton().Icon(winrt::Microsoft::UI::Xaml::Controls::SymbolIcon(winrt::Microsoft::UI::Xaml::Controls::Symbol::RepeatAll));
			winrt::Microsoft::UI::Xaml::Controls::ToolTipService::SetToolTip(MainWindowPointer->LoopButton(), winrt::box_value(L"Loop Disabled"));
		}
	}
	else
	{
		bool isQueued = MainWindowPointer->DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, [this]()
			{
				if (IsLoopEnabled)
				{
					MainWindowPointer->LoopButton().Icon(winrt::Microsoft::UI::Xaml::Controls::SymbolIcon(winrt::Microsoft::UI::Xaml::Controls::Symbol::RepeatOne));
					winrt::Microsoft::UI::Xaml::Controls::ToolTipService::SetToolTip(MainWindowPointer->LoopButton(), winrt::box_value(L"Loop Enabled"));

				}

				else
				{
					MainWindowPointer->LoopButton().Icon(winrt::Microsoft::UI::Xaml::Controls::SymbolIcon(winrt::Microsoft::UI::Xaml::Controls::Symbol::RepeatAll));
					winrt::Microsoft::UI::Xaml::Controls::ToolTipService::SetToolTip(MainWindowPointer->LoopButton(), winrt::box_value(L"Loop Disabled"));

				}
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