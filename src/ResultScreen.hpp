#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include "AnmVm.hpp"
#include "Chain.hpp"
#include "ReplayManager.hpp"
#include "ZunResult.hpp"
#include "inttypes.hpp"

#define TH7K_MAGIC 'K7HT'
#define CATK_MAGIC 'KTAC'
#define HSCR_MAGIC 'RCSH'
#define CLRD_MAGIC 'DRLC'
#define PSCR_MAGIC 'RCSP'
#define PLST_MAGIC 'TSLP'
#define LSNM_MAGIC 'MNSL'
#define VRSM_MAGIC 'MSRV'

struct Th7k
{
    u32 magic;
    u16 th7kLen;
    u16 th7kLen2;
    u8 version;
    u8 isPlayerScore;
    u8 pad[2];
};
static_assert(sizeof(Th7k) == 0xc);

struct Catk
{
    Th7k base;
    u32 highScorePerShot[7];
    u16 idx;
    u8 nameCsum;
    char name[49];
    u16 numAttemptsPerShot[7];
    u16 numSuccessesPerShot[7];
};
static_assert(sizeof(Catk) == 0x78);

struct Hscr
{
    Th7k base;
    u32 score;
    f32 slowRatePercent;
    u8 character;
    u8 difficulty;
    u8 stage;
    char name[9];
    char date[6];
    i8 numRetries;
    u8 pad;
};
static_assert(sizeof(Hscr) == 0x28);

struct Clrd
{
    Th7k base;
    u8 difficultyClearedWithRetries[6];
    u8 difficultyClearedWithoutRetries[6];
    u8 characterShotType;
    u8 pad[3];
};
static_assert(sizeof(Clrd) == 0x1c);

struct Pscr
{
    Th7k base;
    i32 playCount;
    i32 score;
    u8 character;
    u8 difficulty;
    u8 stage;
    u8 pad;
};
static_assert(sizeof(Pscr) == 0x18);

struct PlstPlayCounts
{
    u32 playCount;
    u32 playCountPerShotType[6];
    u32 clearCount;
    u32 noContinueClearCount;
    u32 retryCount;
    u32 extraClearCount;
};
static_assert(sizeof(PlstPlayCounts) == 0x2c);

struct Plst
{
    Th7k base;
    u32 totalHours;
    u32 totalMinutes;
    u32 totalSeconds;
    u32 totalMilliseconds;
    u32 gameHours;
    u32 gameMinutes;
    u32 gameSeconds;
    u32 gameMilliseconds;
    PlstPlayCounts playDataByDifficulty[7]; // 7 is Total
};
static_assert(sizeof(Plst) == 0x160);

struct Lsnm
{
    Th7k base;
    char name[12];
};
static_assert(sizeof(Lsnm) == 0x18);

struct Vrsm
{
    Th7k base;
    char versionStr[6];
    i32 exeSize;
    i32 exeChecksum;
};
static_assert(sizeof(Vrsm) == 0x1c);

struct ScoreDatRaw
{
    u8 xorseed[2];
    u16 csum;
    u16 magic;
    u8 unused_6;
    u8 pad1;
    i32 dataOffset;
    u32 reservedPtr;
    i32 fileLength;
    u32 dstLen;
    i32 srcLen;
};
static_assert(sizeof(ScoreDatRaw) == 0x1c);

struct ScoreListNode
{
    ScoreListNode()
    {
        prev = NULL;
        next = NULL;
        data = NULL;
    }

    ScoreListNode *prev;
    ScoreListNode *next;
    Hscr *data;
};

struct ScoreDat
{
    ScoreDatRaw raw;
    ScoreListNode *scores;
    u8 *decodedData;
};

struct ResultScreen
{
    ResultScreen()
    {
        memset((void *)this, 0, sizeof(ResultScreen));
        this->cursor = 1;
    }

    ~ResultScreen()
    {
        free(this->scoreDat);
    }

    static ZunResult RegisterChain(u32 type);

    static ZunResult AddedCallback(ResultScreen *arg);
    static ZunResult DeletedCallback(ResultScreen *arg);
    static u32 OnUpdate(ResultScreen *arg);
    static u32 OnDraw(ResultScreen *arg);

    ZunResult CheckConfirmButton();
    ZunResult DrawFinalStats();
    i32 DrawStats();
    static void GetDate(char *outDate);
    ZunResult HandleReplaySaveKeyboard();
    ZunResult HandleResultKeyboard();
    static i32 MoveCursor(ResultScreen *screen, i32 max);
    static i32 MoveCursor2(ResultScreen *screen, i32 max);
    static i32 MoveCursorHorizontally(ResultScreen *screen, i32 max);

    static ScoreDat *OpenScore(const char *path);
    static i32 LinkScore(ScoreListNode *prevNode, Hscr *hscr);
    i32 LinkScoreEx(Hscr *out, i32 difficulty, i32 character);
    static u32 GetHighScore(ScoreDat *scoreDat, ScoreListNode *node, u32 character, u32 difficulty,
                            u8 *numRetries);
    static ZunResult ParseCatk(ScoreDat *scoreDat, Catk *outCatk);
    static ZunResult ParseClrd(ScoreDat *scoreDat, Clrd *outClrd);
    static ZunResult ParsePlst(ScoreDat *scoreDat, Plst *outPlst);
    static ZunResult ParsePscr(ScoreDat *scoreDat, Pscr *outPscr);
    static ZunResult ParseScores();
    static void ReleaseScoreDat(ScoreDat *scoreDat);
    void FreeScore(i32 difficulty, i32 character);
    static void FreeAllScores(ScoreListNode *scores);
    static i32 ParseLsnm(ScoreDat *scoreDat, Lsnm *outLsnm);
    void WriteScore();

    void UpdatePrev()
    {
        for (i32 i = 0; i < 41; i++)
        {
            this->vms[i].UpdatePrev();
        }
        for (i32 i = 0; i < 15; i++)
        {
            this->spellcardListVms[i].UpdatePrev();
        }
        this->leftArrowVm.UpdatePrev();
        this->rightArrowVm.UpdatePrev();
    }

    ScoreDat *scoreDat;
    i32 frameTimer;
    i32 resultScreenState;
    i32 stateStep;
    i32 cursor;
    i32 prevCursor;
    i32 savedCursor;
    i32 chosenReplayIdx;
    i32 selectedChar;
    i32 spellcardListPage;
    i32 prevSpellcardListPage;
    i32 listScrollAnimState;
    i32 charUsed;
    i32 lastSpellcardSelected;
    i32 diffPlayed;
    i32 cheatCodeStep;
    i32 isClearingReplayName;
    char replayName[8];
    i32 unused_4c;
    i32 totalPlayCountPerCharacter[7];
    u8 lastTotalSeconds;
    u8 pad[3];
    AnmVm vms[41];
    AnmVm spellcardListVms[15];
    AnmVm leftArrowVm;
    AnmVm rightArrowVm;
    ScoreListNode scoreLists[6][6];
    Hscr defaultScores[6][6][10];
    Hscr curScore;
    Th7k th7kHeader;
    Lsnm lsnmHeader;
    ChainElem *calcChain;
    ChainElem *drawChain;
    ReplayFile replays[15];
    ReplayFile defaultReplay;
};
