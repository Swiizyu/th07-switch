#include "GameErrorContext.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

GameErrorContext g_GameErrorContext;

const char *GameErrorContext::Log(const char *fmt, ...)
{
    char tmp[8192];
    size_t tmpSize;
    va_list args;

    va_start(args, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, args);
    tmpSize = strlen(tmp);
    if (this->m_BufferEnd + tmpSize < this->m_Buffer + 0x1fff)
    {
        strcpy(this->m_BufferEnd, tmp);

        this->m_BufferEnd += tmpSize;
        *this->m_BufferEnd = '\0';
    }
    va_end(args);
    return fmt;
}

const char *GameErrorContext::Fatal(const char *fmt, ...)
{
    char tmp[512];
    size_t tmpSize;
    va_list args;

    va_start(args, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, args);
    tmpSize = strlen(tmp);
    if (this->m_BufferEnd + tmpSize < this->m_Buffer + 0x1fff)
    {
        strcpy(this->m_BufferEnd, tmp);
        this->m_BufferEnd += tmpSize;
        *this->m_BufferEnd = '\0';
    }
    va_end(args);
    this->m_ShowMessageBox = true;
    return fmt;
}
