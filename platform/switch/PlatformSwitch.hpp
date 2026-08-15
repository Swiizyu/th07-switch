// th07-switch: Horizon OS platform glue (see PlatformSwitch.cpp).
#pragma once

#ifdef __SWITCH__

#include <string>

namespace PlatformSwitch
{
// Mount romfs, optionally attach nxlink stdio, locate + chdir into the folder
// that holds the user's th07.dat / thbgm.dat / msgothic.ttc.
void Init();

// Tear down libnx subsystems brought up by Init().
void Exit();

// Absolute path (with trailing slash) of the game data folder.
const std::string &DataPath();

// True when th07.dat was found in DataPath().
bool HasGameData();
} // namespace PlatformSwitch

#endif
