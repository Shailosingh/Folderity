# Folderity

![sketch1674931853012 3423](https://user-images.githubusercontent.com/52577016/229378468-eb1e2443-2c84-4785-adb7-2ae3eb8e8c7c.png)

Folderity is a fast and lightweight music player for Windows that allows users to create custom playlists in folders on their computer that automatically update when a 
song is added or removed from the folder. This is very convenient for anyone who has a large collection of music files (MP3's, WAV's, etc.), since it allows the user to 
automatically organize their music into playlists without having to manually add songs to the playlist one at a time.

![image](https://user-images.githubusercontent.com/52577016/229377824-e16905b2-5e4c-40e7-835d-a8106c0cf9cf.png)

![image](https://user-images.githubusercontent.com/52577016/229377874-6c815e0d-0f00-45a3-aa60-e6d53033ad1f.png)

## Technologies
Folderity is programmed in C++ and uses Microsoft Media Foundation (MMF) for the audio playing component. MMF is rather complex to use, so to simplify its use, I created
a helper library called [MMFSoundPlayer](https://github.com/Shailosingh/MMFSoundPlayer) to help programmers with quickly being able to play audio files in their C++ 
programs.
