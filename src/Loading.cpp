/*
 * Loading.cpp  -  see Loading.h
 */
#include "Loading.h"
#include <iostream>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN     // keeps <windows.h> small and avoids min/max macro clashes
#include <windows.h>            // provides Sleep()
#endif

void showLoadingAnimation() {
#ifdef _WIN32
    for (int i = 0; i < LOADING_FRAMES; i++) {
        // '\r' returns to the start of the line, so every frame overwrites the last one.
        // The spaces erase leftover dots when the dot count drops back to 0.
        std::cout << "\r" << std::string(i % 4, '.') << "    " << std::flush;
        Sleep(LOADING_FRAME_MS);                 // milliseconds
    }
    std::cout << "\r        \r" << std::flush;   // wipe the line so the real output starts clean
#endif
}

/*
 * Purpose : clear the console.
 * Windows : fills the whole console buffer with spaces and moves the cursor to
 *           the top-left (works in every Windows console, no setup needed).
 *           If the output is redirected to a file or pipe there is no console,
 *           so nothing is done.
 * Others  : prints the ANSI escape code "clear screen + cursor home".
 */
void clearScreen() {
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (console == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(console, &info)) {
        return;                                              // not a real console
    }
    DWORD cells = (DWORD)info.dwSize.X * (DWORD)info.dwSize.Y;
    DWORD written = 0;
    COORD topLeft = {0, 0};
    FillConsoleOutputCharacterA(console, ' ', cells, topLeft, &written);
    FillConsoleOutputAttribute(console, info.wAttributes, cells, topLeft, &written);
    SetConsoleCursorPosition(console, topLeft);
#else
    std::cout << "\033[2J\033[H" << std::flush;
#endif
}
