// th07-switch: Horizon OS (Nintendo Switch) platform glue.
//
// Responsibilities:
//   * bring up libnx subsystems the SDL3 switch backend does not own
//     (romfs, socket/nxlink stdio for debugging)
//   * locate the folder holding the user-supplied game data (th07.dat,
//     thbgm.dat, msgothic.ttc) and chdir into it, so every relative path used
//     by the game resolves correctly
//   * expose that folder to FileSystem::GetBasePath / GetPrefPath
//
// No game assets are shipped with this code. The user must supply their own
// copy of Perfect Cherry Blossom 1.00b.

#ifdef __SWITCH__

#include "PlatformSwitch.hpp"

#include <switch.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace
{

std::string g_DataPath = "";
bool g_RomfsMounted = false;
bool g_SocketUp = false;

bool FileExistsAt(const std::string &dir, const char *name)
{
    struct stat st;
    std::string full = dir + name;
    return stat(full.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool DirExists(const std::string &dir)
{
    struct stat st;
    std::string trimmed = dir;
    if (trimmed.size() > 1 && trimmed.back() == '/')
    {
        trimmed.pop_back();
    }
    return stat(trimmed.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

// Folder names we accept for the game data, compared case-insensitively.
// Both the SD root and sdmc:/switch/ are searched, so all of these work:
//   sd:/th07/  sd:/TH07/  sd:/touhou7/  sd:/Touhou7/  sd:/switch/th07/
//   sd:/switch/touhou7/  sd:/switch/Touhou7/ ...
const char *const kFolderNames[] = {
    "th07", "touhou7", "touhou 7", "th07-switch", "touhou7-switch",
    "pcb",  "perfect cherry blossom",
};

// Roots that get scanned, in priority order.
const char *const kRoots[] = {
    "sdmc:/switch/",
    "sdmc:/",
    "sdmc:/games/",
    "sdmc:/roms/",
};

std::string ToLower(const char *s)
{
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return out;
}

bool IsKnownFolderName(const std::string &lowered)
{
    for (const char *name : kFolderNames)
    {
        if (lowered == name)
        {
            return true;
        }
    }
    return false;
}

// Walk a root directory and return every subfolder whose (lowercased) name is
// one we recognise. Real directory entries are used instead of a hardcoded
// path list so that any capitalisation on the user's FAT32 card is matched.
std::vector<std::string> MatchingFoldersIn(const char *root)
{
    std::vector<std::string> matches;
    DIR *dir = opendir(root);
    if (!dir)
    {
        return matches;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != nullptr)
    {
        if (ent->d_name[0] == '.')
        {
            continue;
        }
        std::string full = std::string(root) + ent->d_name + "/";
        if (!DirExists(full))
        {
            continue;
        }
        if (IsKnownFolderName(ToLower(ent->d_name)))
        {
            matches.push_back(full);
        }
    }
    closedir(dir);
    return matches;
}

} // namespace

namespace PlatformSwitch
{

void Init()
{
    // romfs is optional: we only bundle a fallback font/README inside the NRO.
    if (R_SUCCEEDED(romfsInit()))
    {
        g_RomfsMounted = true;
    }

#ifdef TH_SWITCH_NXLINK
    if (R_SUCCEEDED(socketInitializeDefault()))
    {
        g_SocketUp = true;
        nxlinkStdio();
        std::printf("[th07-switch] nxlink stdio attached\n");
    }
#endif

    // 1. a recognised folder that actually contains the game data
    std::vector<std::string> candidates;
    for (const char *root : kRoots)
    {
        for (const std::string &folder : MatchingFoldersIn(root))
        {
            candidates.push_back(folder);
        }
    }

    for (const std::string &cand : candidates)
    {
        if (FileExistsAt(cand, "th07.dat"))
        {
            g_DataPath = cand;
            break;
        }
    }

    // 2. otherwise the directory the NRO was launched from (hbmenu sets cwd)
    if (g_DataPath.empty())
    {
        char cwd[FS_MAX_PATH];
        if (getcwd(cwd, sizeof(cwd)) && FileExistsAt(std::string(cwd) + "/", "th07.dat"))
        {
            g_DataPath = std::string(cwd) + "/";
        }
    }

    // 3. last resort: first recognised folder that exists at all, so the game
    //    can still write th07.cfg and report a useful error
    if (g_DataPath.empty() && !candidates.empty())
    {
        g_DataPath = candidates.front();
    }

    // 4. nothing at all: create sdmc:/switch/th07
    if (g_DataPath.empty())
    {
        mkdir("sdmc:/switch", 0777);
        mkdir("sdmc:/switch/th07", 0777);
        g_DataPath = "sdmc:/switch/th07/";
    }

    chdir(g_DataPath.c_str());
    std::printf("[th07-switch] data path: %s\n", g_DataPath.c_str());
}

void Exit()
{
    if (g_SocketUp)
    {
        socketExit();
        g_SocketUp = false;
    }
    if (g_RomfsMounted)
    {
        romfsExit();
        g_RomfsMounted = false;
    }
}

const std::string &DataPath()
{
    if (g_DataPath.empty())
    {
        g_DataPath = "sdmc:/switch/th07/";
    }
    return g_DataPath;
}

bool HasGameData()
{
    return FileExistsAt(DataPath(), "th07.dat");
}

} // namespace PlatformSwitch

#endif // __SWITCH__
