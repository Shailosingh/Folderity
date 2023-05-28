# Folderity

![sketch1674931853012 3423](https://user-images.githubusercontent.com/52577016/229378468-eb1e2443-2c84-4785-adb7-2ae3eb8e8c7c.png)

Folderity is a fast and lightweight music player for Windows that allows users to create custom playlists in folders on their computer that automatically update when a 
song is added or removed from the folder. This is very convenient for anyone who has a large collection of music files (MP3's, WAV's, etc.), since it allows the user to 
automatically organize their music into playlists without having to manually add songs to the playlist one at a time.

![PlaylistPage](https://github.com/Shailosingh/Folderity/assets/52577016/f4031401-df94-4c28-9dc4-79d5f6617be8)

![QueuePage](https://github.com/Shailosingh/Folderity/assets/52577016/9f8902f5-6598-4bad-bbcb-4f92f9ad519e)

## Technologies
Folderity is programmed in C++ and uses Microsoft Media Foundation (MMF) for the audio playing component. MMF is rather complex to use, so to simplify its use, I created
a helper library called [MMFSoundPlayer](https://github.com/Shailosingh/MMFSoundPlayer) to help programmers with quickly being able to play audio files in their C++ 
programs.
