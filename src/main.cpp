/*
 * main.cpp  -  Program entry point (Member 3: integration)
 * Flow: load data on startup -> show load report -> menu loop -> (menu offers to save on exit)
 */
#include <iostream>
#include "Graph.h"
#include "FileManager.h"
#include "Menu.h"
#include "Utils.h"

int main() {
    SocialNetwork network;
    SecurityConfig config;
    LoadReport report;

    std::cout << "=== Social Network Analyzer ===\n";
    Status status = FileManager::loadNetwork(network, DEFAULT_USERS_FILE, DEFAULT_FRIENDS_FILE, report);
    if (status != OK) {
        std::cout << "Warning: " << statusToString(status) << "\n";
    }
    FileManager::printLoadReport(report);
    waitForEnter("\nPress Enter to open the menu...");   // the menu clears the screen, so let the user read the report first

    runMenu(network, config);
    return 0;
}
