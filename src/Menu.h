/*
 * Menu.h  -  MODULE C (Member 3): console menu and input helpers
 * ---------------------------------------------------------------
 * Input helpers read WHOLE LINES with getline, so no stray characters are
 * ever left in the input buffer, and they re-prompt until the input is valid.
 * If the input stream is closed (Ctrl+D / end of a piped file) they return
 * a harmless value (min value / empty string / false) instead of looping forever.
 */
#ifndef MENU_H
#define MENU_H

#include <string>
#include "Graph.h"
#include "Utils.h"

/* Main loop: shows the menu, runs the chosen action, repeats until 0 (Exit). */
void runMenu(SocialNetwork& net, SecurityConfig& config);

/* Re-prompts until the user types an integer in [minValue, maxValue]. */
int readInt(const std::string& prompt, int minValue, int maxValue);

/* Reads one line (any text). */
std::string readLine(const std::string& prompt);

/* Re-prompts until the text is a valid name/city/profession field (Section 7a). */
std::string readField(const std::string& prompt);

/* Re-prompts until the text is a valid phone number (optional '+', 7-15 digits). */
std::string readContact(const std::string& prompt);

/* Shows a message and waits for the user to press Enter (so output can be read
 * before the screen is cleared).  Returns immediately if the input is closed. */
void waitForEnter(const std::string& prompt = "\nPress Enter to continue...");

/* Asks a yes/no question.  Returns true for y/yes (any case). */
bool confirm(const std::string& prompt);

#endif
