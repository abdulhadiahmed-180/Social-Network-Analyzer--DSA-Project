/*
 * Loading.h  -  Cosmetic console effects: loading dots + clear screen
 * ---------------------------------------------------------------
 * showLoadingAnimation(): prints  .  ..  ...  on one line (using '\r' to
 *                         overwrite it), then erases it.
 * clearScreen()         : wipes the console window.
 *
 * IMPORTANT: Sleep() and the console-clearing calls come from <windows.h>,
 * which is NOT one of the allowed project headers and only exists on Windows.
 * That is why they live in this one small file and nowhere else:
 *   - on Windows both effects work;
 *   - on Linux / macOS the animation does nothing and clearScreen() uses
 *     the standard ANSI escape code, so the program still builds and runs.
 * To remove the effects completely, delete the calls in Menu.cpp and this file.
 */
#ifndef LOADING_H
#define LOADING_H

const int LOADING_FRAMES   = 8;     // number of animation frames
const int LOADING_FRAME_MS = 100;   // milliseconds per frame  (8 x 100 = 0.8 seconds in total)

/* Shows the loading animation (Windows only, otherwise does nothing). */
void showLoadingAnimation();

/* Clears the console window and puts the cursor at the top-left. */
void clearScreen();

#endif
