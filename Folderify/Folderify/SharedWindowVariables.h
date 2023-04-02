#pragma once
#include "pch.h"
#include "MusicController.h"

//Variables that are belong to the whole window. Every page should have access to these
extern MusicController* ControllerObject;

//Used to ensure dispatcher does not keep running after window is closed
extern bool WindowClosing;