#include "pch.h"
#include "SharedWindowVariables.h"

//Declarations
MusicController* ControllerObject;

//Variables to ensure dispatcher does not keep running after window is closed
bool WindowClosing = false;