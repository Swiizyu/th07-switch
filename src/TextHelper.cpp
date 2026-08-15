#include "TextHelper.hpp"

#include <SDL3/SDL_surface.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "GameErrorContext.hpp"
#include "Supervisor.hpp"
#include "graphics/ZunGraphics.hpp"
#include "inttypes.hpp"
#include "sj2utf8/sj2utf8.h"

static TTF_Font *g_Font = nullptr;

// stolen from
// https://stackoverflow.com/questions/3404199/how-to-find-out-the-encoding-of-a-file-c-sharp/3404317#3404317
bool IsUtf8(const char *string)
{
    i32 charByteCounter = 1;
    unsigned char curByte;

    size_t len = strlen(string);
    for (size_t i = 0; i < len; i++)
    {
        curByte = string[i];
        if (charByteCounter == 1)
        {
            if (curByte >= 0x80)
            {
                while (((curByte <<= 1) & 0x80) != 0)
                {
                    charByteCounter++;
                }
                if (charByteCounter == 1 || charByteCounter > 6)
                {
                    return false;
                }
            }
        }
        else
        {
            if ((curByte & 0xC0) != 0x80)
            {
                return false;
            }
            charByteCounter--;
        }
    }
    if (charByteCounter > 1)
    {
        return false;
    }

    return true;
}

TextHelper::TextHelper()
{
    this->buffer = NULL;
    this->width = 0;
    this->height = 0;
}

TextHelper::~TextHelper()
{
    ReleaseBuffer();
}

bool TextHelper::ReleaseBuffer()
{
    if (this->buffer)
    {
        SDL_DestroySurface(this->buffer);
        this->buffer = NULL;
        this->width = 0;
        this->height = 0;
        return true;
    }
    return false;
}

bool TextHelper::AllocateBuffer(i32 width, i32 height)
{
    ReleaseBuffer();
    this->buffer = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
    if (!this->buffer)
    {
        return false;
    }
    SDL_FillSurfaceRect(this->buffer, NULL, 0);
    this->width = width;
    this->height = height;
    return true;
}

bool TextHelper::InvertAlpha(i32 x, i32 y, i32 spriteWidth, i32 fontHeight, i32 param5)
{
    i32 doubleArea = spriteWidth * fontHeight * 2;
    if (doubleArea == 0 || !this->buffer)
    {
        return false;
    }

    SDL_LockSurface(this->buffer);
    u8 *pixels = (u8 *)this->buffer->pixels;
    i32 pitch = this->buffer->pitch;

    for (i32 py = 0; py < fontHeight; py++)
    {
        for (i32 px = 0; px < spriteWidth; px++)
        {
            u8 *p = &pixels[(py + y) * pitch + (px + x) * 4];
            u8 r = p[0];
            u8 g = p[1];
            u8 b = p[2];
            u8 a = p[3];

            if (a > 0)
            {
                i32 i = (py * spriteWidth + px) * 2;

                if (!param5)
                {
                    if (r >= b)
                    {
                        r = r - (r * i * 2) / doubleArea / 3;
                        g = g - (g * i * 2) / doubleArea / 3;
                    }
                    else
                    {
                        b = b - (b * i) / doubleArea / 2;
                        g = g - (g * i) / doubleArea / 2;
                    }
                }
                else
                {
                    if (r >= b)
                    {
                        r = r - (r * i) / doubleArea / 4;
                        g = g - (g * i) / doubleArea / 4;
                    }
                    else
                    {
                        b = b - (b * i) / doubleArea / 4;
                        g = g - (g * i) / doubleArea / 4;
                    }
                }

                p[0] = r;
                p[1] = g;
                p[2] = b;
                p[3] = a;
            }
            else
            {
                p[0] = 0;
                p[1] = 0;
                p[2] = 0;
                p[3] = 0;
            }
        }
    }

    SDL_UnlockSurface(this->buffer);
    return true;
}

bool TextHelper::CopyTextToTexture(i32 yPos, i32 spriteWidth, i32 spriteHeight, i32 fontHeight,
                                   i32 fontWidth, GfxTextureHandle outTexture)
{
    SDL_Surface *outSurface = SDL_CreateSurface(spriteWidth, spriteHeight, SDL_PIXELFORMAT_RGBA32);
    if (!outSurface)
    {
        return false;
    }
    SDL_FillSurfaceRect(outSurface, NULL, 0);

    SDL_Rect srcRect;
    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.w = spriteWidth * 2;
    srcRect.h = fontHeight * 2;
    if (srcRect.w > this->width)
    {
        srcRect.w = this->width;
    }
    if (srcRect.h > this->height)
    {
        srcRect.h = this->height;
    }

    SDL_Rect dstRect;
    dstRect.x = 0;
    dstRect.y = 0;
    dstRect.w = spriteWidth;
    dstRect.h = fontWidth;

    SDL_StretchSurface(this->buffer, &srcRect, outSurface, &dstRect, SDL_SCALEMODE_LINEAR);

    g_Supervisor.gfxDevice->BindTexture(outTexture);
    g_Supervisor.gfxDevice->SetTextureSubImage(0, yPos, outSurface->w, fontWidth,
                                               outSurface->pixels);
    SDL_DestroySurface(outSurface);
    return true;
}

ZunResult TextHelper::CreateTextBuffer()
{
    if (g_Font)
    {
        return ZUN_SUCCESS;
    }

    if (!TTF_Init())
    {
        g_GameErrorContext.Log("TTF_Init fail : %s\n", SDL_GetError());
        return ZUN_ERROR;
    }

    g_Font = TTF_OpenFont(FileSystem::GetBasePath("msgothic.ttc").c_str(), 10);
    if (!g_Font)
    {
        g_GameErrorContext.Log("TTF_OpenFont fail : %s\n", SDL_GetError());
        return ZUN_ERROR;
    }
    TTF_SetFontStyle(g_Font, TTF_STYLE_BOLD);
    return ZUN_SUCCESS;
}

void TextHelper::ReleaseTextBuffer()
{
    TTF_CloseFont(g_Font);
    g_Font = nullptr;
    TTF_Quit();
}

void TextHelper::RenderTextToTextureBold(i32 xPos, i32 yPos, i32 spriteWidth, i32 spriteHeight,
                                         i32 fontHeight, i32 fontWidth, u32 textColor,
                                         u32 outlineType, char *string, GfxTextureHandle outTexture)
{
    i32 fontSize = fontHeight * 2 - 2;
    if (fontSize <= 0)
    {
        return;
    }

    if (CreateTextBuffer() != ZUN_SUCCESS)
    {
        return;
    }

    TTF_SetFontSize(g_Font, fontSize);

    char *convStr = string;
    bool needsFree = false;
    if (!IsUtf8(string))
    {
        std::string tmp = sj2utf8(string);
        if (!tmp.empty())
        {
            convStr = SDL_strdup(tmp.c_str());
            needsFree = true;
        }
    }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *textSurf = TTF_RenderText_Blended(g_Font, convStr, 0, white);
    if (needsFree)
    {
        SDL_free(convStr);
    }
    if (!textSurf)
    {
        return;
    }

    i32 dWidth = spriteWidth * 2;
    i32 dHeight = fontHeight * 2 + 6;
    if (dWidth > 1024)
    {
        dWidth = 1024;
    }
    if (dHeight > 64)
    {
        dHeight = 64;
    }
    if (dWidth <= 0 || dHeight <= 0)
    {
        SDL_DestroySurface(textSurf);
        return;
    }

    TextHelper textHelper;
    textHelper.AllocateBuffer(dWidth, dHeight);

    SDL_SetSurfaceBlendMode(textSurf, SDL_BLENDMODE_BLEND);

    SDL_SetSurfaceColorMod(textSurf, 0, 0, 0);
    SDL_Rect dstRect;
    if (outlineType != 0xffffffff)
    {
        i32 dx[4] = {4, 0, 2, 2};
        i32 dy[4] = {2, 2, 0, 4};
        for (i32 i = 0; i < 4; i++)
        {
            dstRect = {xPos * 2 + dx[i], dy[i], textSurf->w, textSurf->h};
            SDL_BlitSurface(textSurf, NULL, textHelper.buffer, &dstRect);
        }
    }
    else
    {
        i32 dx[4] = {3, 1, 2, 2};
        i32 dy[4] = {2, 2, 1, 3};
        for (i32 i = 0; i < 4; i++)
        {
            dstRect = {xPos * 2 + dx[i], dy[i], textSurf->w, textSurf->h};
            SDL_BlitSurface(textSurf, NULL, textHelper.buffer, &dstRect);
        }
    }

    u8 r = (textColor >> 16) & 0xFF;
    u8 g = (textColor >> 8) & 0xFF;
    u8 b_col = textColor & 0xFF;
    SDL_SetSurfaceColorMod(textSurf, r, g, b_col);
    dstRect = {xPos * 2 + 2, 2, textSurf->w, textSurf->h};
    SDL_BlitSurface(textSurf, NULL, textHelper.buffer, &dstRect);

    SDL_DestroySurface(textSurf);

    textHelper.InvertAlpha(0, 0, spriteWidth << 1, fontHeight * 2 + 6,
                           (u32)(outlineType == 0xffffffff));
    textHelper.CopyTextToTexture(yPos, spriteWidth, spriteHeight, fontHeight, fontWidth,
                                 outTexture);
}

i32 TextHelper::GetLogicalStringWidth(const char *str)
{
    if (!IsUtf8(str))
    {
        return strlen(str);
    }

    i32 width = 0;
    while (*str)
    {
        if ((*str & 0x80) == 0)
        {
            width += 1;
            str += 1;
        }
        else if ((*str & 0xE0) == 0xC0)
        {
            width += 2;
            str += 2;
        }
        else if ((*str & 0xF0) == 0xE0)
        {
            width += 2;
            str += 3;
        }
        else if ((*str & 0xF8) == 0xF0)
        {
            width += 2;
            str += 4;
        }
        else
        {
            str++;
        }
    }
    return width;
}
