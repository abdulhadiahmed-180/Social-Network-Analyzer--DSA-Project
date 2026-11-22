/*
 * Menu.cpp  -  MODULE C (Member 3): console menu
 *
 * The menu never changes the graph directly: it only calls the public
 * SocialNetwork methods, the Module B algorithms and the Module C classes,
 * then prints the returned Status as a message.
 */
#include "Menu.h"
#include "Algorithms.h"
#include "FileManager.h"
#include "Security.h"
#include "Loading.h"
#include <iostream>

/* ============================================================
 *                      Input helpers
 * ============================================================ */

namespace {

/* Prints the prompt and reads one line.  Returns false if the input stream is closed. */
bool readRaw(const std::string& prompt, std::string& line) {
    std::cout << prompt;
    return !std::getline(std::cin, line).fail();
}

} // namespace

std::string readLine(const std::string& prompt) {
    std::string line;
    if (!readRaw(prompt, line)) return "";
    return line;
}

int readInt(const std::string& prompt, int minValue, int maxValue) {
    while (true) {
        std::string line;
        if (!readRaw(prompt, line)) return minValue;          // input closed
        int value = 0;
        if (parseInteger(line, value) && value >= minValue && value <= maxValue) return value;
        std::cout << "  Invalid input. Please enter a whole number from "
                  << minValue << " to " << maxValue << ".\n";
    }
}

std::string readField(const std::string& prompt) {
    while (true) {
        std::string line;
        if (!readRaw(prompt, line)) return "";
        if (isValidTextField(line)) return trim(line);
        std::cout << "  Invalid text. Use 1-" << MAX_FIELD_LEN
                  << " characters and do not use the '|' character.\n";
    }
}

std::string readContact(const std::string& prompt) {
    while (true) {
        std::string line;
        if (!readRaw(prompt, line)) return "";
        if (isValidContact(line)) return trim(line);
        std::cout << "  Invalid contact. Use an optional '+' followed by "
                  << MIN_CONTACT_DIGITS << "-" << MAX_CONTACT_DIGITS << " digits.\n";
    }
}

void waitForEnter(const std::string& prompt) {
    readLine(prompt);        // the text typed (if any) is ignored
}

bool confirm(const std::string& prompt) {
    while (true) {
        std::string line;
        if (!readRaw(prompt + " (y/n): ", line)) return false;
        std::string answer = trim(line);
        if (equalsIgnoreCase(answer, "y") || equalsIgnoreCase(answer, "yes")) return true;
        if (equalsIgnoreCase(answer, "n") || equalsIgnoreCase(answer, "no")) return false;
        std::cout << "  Please answer y or n.\n";
    }
}

/* ============================================================
 *                    Printing helpers
 * ============================================================ */

namespace {

/* Prints "Done" style message on success, or the error text on failure. */
void reportStatus(Status status, const std::string& successMessage) {
    if (status == OK) std::cout << successMessage << "\n";
    else std::cout << "Error: " << statusToString(status) << "\n";
}

/* Prints "Name [ID]" for each ID in the list, separated by 'separator'. */
void printUserList(const SocialNetwork& net, const SafeIntArray& ids, const char* separator) {
    for (int i = 0; i < ids.getSize(); i++) {
        int id = ids.getOr(i, 0);
        const UserNode* user = net.findUserById(id);
        if (i > 0) std::cout << separator;
        if (user != nullptr) std::cout << user->name << " [" << id << "]";
        else std::cout << "[" << id << "]";
    }
    std::cout << "\n";
}

void printMenu() {
    std::cout << "======================== MENU ========================\n"
              << " 1. Add user              10. DFS traversal\n"
              << " 2. Search users          11. Shortest path\n"
              << " 3. Update user           12. Connected components\n"
              << " 4. Remove user           13. Recommend friends\n"
              << " 5. Add friendship        14. Rank users by friends\n"
              << " 6. Remove friendship     15. Security scan\n"
              << " 7. Show friends          16. Change security settings\n"
              << " 8. Mutual friends        17. Save\n"
              << " 9. BFS traversal         18. Load\n"
              << " 0. Exit\n"
              << "======================================================\n";
}

} // namespace

/* ============================================================
 *                      Menu actions
 * ============================================================ */

namespace {

/* 1. Add user */
void actionAddUser(SocialNetwork& net) {
    int id = readInt("User ID (positive number): ", 1, MAX_ID);
    if (net.findUserById(id) != nullptr) {       // fail early, before asking for everything else
        reportStatus(ERR_DUPLICATE_ID, "");
        return;
    }
    std::string name = readField("Name: ");
    int age = readInt("Age (" + std::to_string(MIN_AGE) + "-" + std::to_string(MAX_AGE) + "): ", MIN_AGE, MAX_AGE);
    std::string city = readField("City: ");
    std::string profession = readField("Profession: ");
    std::string contact = readContact("Contact number: ");
    reportStatus(net.addUser(id, name, age, city, profession, contact), "User added.");
}

/* 2. Search users */
void actionSearchUsers(SocialNetwork& net) {
    std::cout << "Search by: 1) ID  2) Name  3) City  4) Profession\n";
    int mode = readInt("Choice: ", 1, 4);
    if (mode == 1) {
        int id = readInt("User ID: ", 1, MAX_ID);
        net.displayUser(id);
        return;
    }
    SearchField field = (mode == 2) ? BY_NAME : (mode == 3) ? BY_CITY : BY_PROFESSION;
    std::string keyword = trim(readLine("Keyword (partial text, any case): "));
    if (keyword.empty()) {
        std::cout << "Keyword cannot be empty.\n";
        return;
    }
    SafeIntArray found;
    int count = net.searchUsers(field, keyword, found);
    if (count == 0) {
        std::cout << "No matching users.\n";
        return;
    }
    for (int i = 0; i < count; i++) net.displayUser(found.getOr(i, 0));
    std::cout << count << " match(es).\n";
}

/* 3. Update user */
void actionUpdateUser(SocialNetwork& net) {
    int id = readInt("ID of the user to update: ", 1, MAX_ID);
    if (net.findUserById(id) == nullptr) {
        reportStatus(ERR_NOT_FOUND, "");
        return;
    }
    std::cout << "Current details: ";
    net.displayUser(id);
    std::cout << "Enter the new details (the ID cannot change).\n";
    std::string name = readField("Name: ");
    int age = readInt("Age (" + std::to_string(MIN_AGE) + "-" + std::to_string(MAX_AGE) + "): ", MIN_AGE, MAX_AGE);
    std::string city = readField("City: ");
    std::string profession = readField("Profession: ");
    std::string contact = readContact("Contact number: ");
    reportStatus(net.updateUser(id, name, age, city, profession, contact), "User updated.");
}

/* 4. Remove user */
void actionRemoveUser(SocialNetwork& net) {
    int id = readInt("ID of the user to remove: ", 1, MAX_ID);
    if (net.findUserById(id) == nullptr) {
        reportStatus(ERR_NOT_FOUND, "");
        return;
    }
    int friends = net.getFriendCount(id);
    if (!confirm("Remove this user and their " + std::to_string(friends) + " friendship(s)?")) {
        std::cout << "Cancelled.\n";
        return;
    }
    reportStatus(net.removeUser(id), "User removed.");
}

/* 5. Add friendship */
void actionAddFriendship(SocialNetwork& net) {
    int a = readInt("First user ID: ", 1, MAX_ID);
    int b = readInt("Second user ID: ", 1, MAX_ID);
    reportStatus(net.addFriendship(a, b), "Friendship added.");
}

/* 6. Remove friendship */
void actionRemoveFriendship(SocialNetwork& net) {
    int a = readInt("First user ID: ", 1, MAX_ID);
    int b = readInt("Second user ID: ", 1, MAX_ID);
    reportStatus(net.removeFriendship(a, b), "Friendship removed.");
}

/* 7. Show friends */
void actionShowFriends(SocialNetwork& net) {
    int id = readInt("User ID: ", 1, MAX_ID);
    Status s = net.displayFriends(id);
    if (s != OK) reportStatus(s, "");
}

/* 8. Mutual friends */
void actionMutualFriends(SocialNetwork& net) {
    int a = readInt("First user ID: ", 1, MAX_ID);
    int b = readInt("Second user ID: ", 1, MAX_ID);
    SafeIntArray result;
    Status s = mutualFriends(net, a, b, result);
    if (s != OK) { reportStatus(s, ""); return; }
    if (result.getSize() == 0) {
        std::cout << "These users have no mutual friends.\n";
        return;
    }
    std::cout << result.getSize() << " mutual friend(s): ";
    printUserList(net, result, ", ");
}

/* 9 and 10. BFS / DFS */
void actionTraversal(SocialNetwork& net, bool useBfs) {
    int start = readInt("Start user ID: ", 1, MAX_ID);
    SafeIntArray order;
    Status s = useBfs ? bfsTraversal(net, start, order) : dfsTraversal(net, start, order);
    if (s != OK) { reportStatus(s, ""); return; }
    std::cout << (useBfs ? "BFS" : "DFS") << " order (" << order.getSize() << " user(s) reachable):\n  ";
    printUserList(net, order, " -> ");
}

/* 11. Shortest path */
void actionShortestPath(SocialNetwork& net) {
    int src = readInt("From user ID: ", 1, MAX_ID);
    int dst = readInt("To user ID: ", 1, MAX_ID);
    SafeIntArray path;
    int distance = 0;
    Status s = shortestPath(net, src, dst, path, distance);
    if (s != OK) { reportStatus(s, ""); return; }
    std::cout << "Degrees of separation: " << distance << "\n  ";
    printUserList(net, path, " -> ");
}

/* 12. Connected components */
void actionComponents(SocialNetwork& net) {
    SafeIntArray labels;
    int count = connectedComponents(net, labels);
    if (count == 0) { reportStatus(ERR_NETWORK_EMPTY, ""); return; }
    std::cout << count << " connected component(s):\n";
    for (int c = 0; c < count; c++) {
        std::cout << "  Component " << (c + 1) << ": ";
        bool first = true;
        for (const UserNode* u = net.getHead(); u != nullptr; u = u->next) {
            if (labels.getOr(u->tempIndex, -1) == c) {
                if (!first) std::cout << ", ";
                std::cout << u->name << " [" << u->id << "]";
                first = false;
            }
        }
        std::cout << "\n";
    }
}

/* 13. Recommend friends */
void actionRecommend(SocialNetwork& net) {
    int id = readInt("User ID: ", 1, MAX_ID);
    int topK = readInt("How many suggestions (1-" + std::to_string(MAX_USERS) + ")? ", 1, MAX_USERS);
    RecommendEntry* results = new RecommendEntry[topK];
    int count = 0;
    Status s = recommendFriends(net, id, topK, results, count);
    if (s != OK) {
        reportStatus(s, "");
    } else if (count == 0) {
        std::cout << "No suggestions (no friends-of-friends who are not already friends).\n";
    } else {
        std::cout << "Suggested friends:\n";
        for (int i = 0; i < count; i++) {
            std::cout << "  " << (i + 1) << ". " << results[i].name << " [" << results[i].id << "] - "
                      << results[i].mutualCount << " mutual friend(s)\n";
        }
    }
    delete[] results;
}

/* 14. Rank users by friends */
void actionRank(SocialNetwork& net) {
    int n = net.getUserCount();
    if (n == 0) { reportStatus(ERR_NETWORK_EMPTY, ""); return; }
    RankEntry* ranking = new RankEntry[n];
    int count = 0;
    Status s = rankUsersByFriends(net, ranking, count);
    if (s != OK) {
        reportStatus(s, "");
    } else {
        std::cout << padRight("Rank", 6) << padRight("ID", 11) << padRight("Name", 24) << "Friends\n";
        for (int i = 0; i < count; i++) {
            std::cout << padRight(std::to_string(i + 1), 6) << padRight(std::to_string(ranking[i].id), 11)
                      << padRight(ranking[i].name, 24) << ranking[i].friendCount << "\n";
        }
    }
    delete[] ranking;
}

/* 15. Security scan */
void actionSecurityScan(SocialNetwork& net, const SecurityConfig& config) {
    std::cout << "Scan: 1) whole network  2) one user\n";
    int mode = readInt("Choice: ", 1, 2);
    Flag* flags = new Flag[MAX_FLAGS];
    int count = 0;
    Status s;
    if (mode == 1) {
        s = SecurityAnalyzer::analyze(net, config, flags, count);
    } else {
        int id = readInt("User ID: ", 1, MAX_ID);
        s = SecurityAnalyzer::analyzeUser(net, id, config, flags, count);
    }
    if (s != OK) reportStatus(s, "");
    else SecurityAnalyzer::printReport(flags, count);
    delete[] flags;
}

/* 16. Change security settings */
void actionSecuritySettings(SecurityConfig& config) {
    while (true) {
        std::cout << "\n--- Security settings ---\n"
                  << " 1. R1 max friends allowed      : " << config.maxFriendThreshold << "\n"
                  << " 2. R2 isolated-account check   : " << (config.isolationEnabled ? "ON" : "OFF") << "\n"
                  << " 3. R3 low-clustering percent   : " << config.lowClusteringPercent
                  << (config.lowClusteringPercent == 0 ? " (OFF)" : "") << "\n"
                  << " 4. R4 shared-contact check     : " << (config.sharedContactEnabled ? "ON" : "OFF") << "\n"
                  << " 0. Back\n";
        int choice = readInt("Choice: ", 0, 4);
        if (choice == 0) return;
        if (choice == 1) {
            int value = readInt("New max friends (1-" + std::to_string(MAX_USERS) + "): ", 1, MAX_USERS);
            reportStatus(SecurityAnalyzer::setThreshold(config, value), "Threshold updated.");
        } else if (choice == 2) {
            config.isolationEnabled = !config.isolationEnabled;
            std::cout << "Isolated-account check is now " << (config.isolationEnabled ? "ON" : "OFF") << ".\n";
        } else if (choice == 3) {
            int value = readInt("New percentage (0-100, 0 turns R3 off): ", 0, 100);
            reportStatus(SecurityAnalyzer::setClusteringPercent(config, value), "Percentage updated.");
        } else {
            config.sharedContactEnabled = !config.sharedContactEnabled;
            std::cout << "Shared-contact check is now " << (config.sharedContactEnabled ? "ON" : "OFF") << ".\n";
        }
    }
}

/* 17. Save (also used by the exit prompt) */
void actionSave(const SocialNetwork& net) {
    Status s = FileManager::saveNetwork(net, DEFAULT_USERS_FILE, DEFAULT_FRIENDS_FILE);
    if (s == OK) {
        std::cout << "Saved " << net.getUserCount() << " user(s) and " << net.getEdgeCount()
                  << " friendship(s) to " << DEFAULT_USERS_FILE << " and " << DEFAULT_FRIENDS_FILE << ".\n";
    } else {
        std::cout << "Error: " << statusToString(s) << " (does the 'data' folder exist?)\n";
    }
}

/* 18. Load */
void actionLoad(SocialNetwork& net) {
    if (!confirm("Loading replaces the current network. Continue?")) {
        std::cout << "Cancelled.\n";
        return;
    }
    LoadReport report;
    Status s = FileManager::loadNetwork(net, DEFAULT_USERS_FILE, DEFAULT_FRIENDS_FILE, report);
    if (s != OK) std::cout << "Error: " << statusToString(s) << "\n";
    FileManager::printLoadReport(report);
}

} // namespace

/* ============================================================
 *                        Main loop
 * ============================================================ */

/*
 * Which menu options clear the screen right after they are chosen?
 * Index = menu number (0..18).  true = clear, false = keep the menu on screen.
 * To change it, just edit a true / false below.
 */
const bool CLEAR_ON_CHOICE[19] = {
    false,                        //  0 Exit
    true,  true,  true,  true,    //  1-4   add / search / update / remove user
    true,  true,  true,  true,    //  5-8   friendships, show friends, mutual friends
    true,  true,  true,  true,    //  9-12  BFS, DFS, shortest path, components
    true,  true,  true,  true,    // 13-16  recommend, rank, security scan, settings
    true,  true                   // 17-18  save, load
};

void runMenu(SocialNetwork& net, SecurityConfig& config) {
    while (true) {
        clearScreen();                               // fresh screen every time the menu is shown
        printMenu();
        int choice = readInt("Enter choice: ", 0, 18);
        if (CLEAR_ON_CHOICE[choice]) clearScreen();  // clean screen for the chosen action's output
        if (choice != 0) showLoadingAnimation();     // dots while the choice is "processed" (Windows only)
        //std::cout << "\n";
        switch (choice) {
            case 1:  actionAddUser(net); break;
            case 2:  actionSearchUsers(net); break;
            case 3:  actionUpdateUser(net); break;
            case 4:  actionRemoveUser(net); break;
            case 5:  actionAddFriendship(net); break;
            case 6:  actionRemoveFriendship(net); break;
            case 7:  actionShowFriends(net); break;
            case 8:  actionMutualFriends(net); break;
            case 9:  actionTraversal(net, true); break;
            case 10: actionTraversal(net, false); break;
            case 11: actionShortestPath(net); break;
            case 12: actionComponents(net); break;
            case 13: actionRecommend(net); break;
            case 14: actionRank(net); break;
            case 15: actionSecurityScan(net, config); break;
            case 16: actionSecuritySettings(config); break;
            case 17: actionSave(net); break;
            case 18: actionLoad(net); break;
            case 0:
                if (confirm("Save the network before exiting?")) actionSave(net);
                std::cout << "Goodbye!\n";
                return;
        }
        // Without this pause the screen would be cleared before the result could be read.
        if (choice != 16) waitForEnter("\nPress Enter to return to the menu...");
    }
}
