/*
 * FileManager.cpp  -  MODULE C (Member 3): save / load implementation
 */
#include "FileManager.h"
#include <fstream>
#include <iostream>

namespace {

const int USER_FIELD_COUNT = 6;   // id|name|age|city|profession|contact
const int EDGE_FIELD_COUNT = 2;   // idA|idB

/*
 * Reads the next RECORD line: skips blank lines and '#' comments, trims the
 * line (this also removes a Windows '\r') and strips a UTF-8 BOM on line 1.
 * lineNo always holds the physical line number of the record returned.
 */
bool nextRecord(std::ifstream& in, std::string& record, int& lineNo) {
    std::string raw;
    while (std::getline(in, raw)) {
        lineNo++;
        if (lineNo == 1 && (int)raw.length() >= 3 &&
            raw[0] == '\xEF' && raw[1] == '\xBB' && raw[2] == '\xBF') {
            raw.erase(0, 3);
        }
        std::string text = trim(raw);
        if (text.empty() || text[0] == '#') continue;
        record = text;
        return true;
    }
    return false;
}

/* Counts a rejected record and stores "file line N: reason" (up to MAX_ERRORS). */
void rejectRecord(LoadReport& report, const std::string& path, int lineNo, const std::string& reason) {
    if (report.rejectedCount < MAX_ERRORS) {
        report.errors[report.rejectedCount] = path + " line " + std::to_string(lineNo) + ": " + reason;
    }
    report.rejectedCount++;
}

/*
 * Parses one users.txt record and adds the user.
 * Returns OK, ERR_MALFORMED_RECORD (wrong structure / not numbers),
 * ERR_INVALID_INPUT (a field value breaks Section 7a), ERR_DUPLICATE_ID or ERR_CAPACITY.
 * 'reason' explains any failure.
 */
Status applyUserRecord(SocialNetwork& net, const std::string& line, std::string& reason) {
    std::string f[USER_FIELD_COUNT];
    int count = FileManager::splitFields(line, '|', f, USER_FIELD_COUNT);
    if (count != USER_FIELD_COUNT) {
        reason = "wrong field count (expected 6, got " + std::to_string(count) + ")";
        return ERR_MALFORMED_RECORD;
    }
    int id = 0, age = 0;
    if (!FileManager::parsePositiveInt(f[0], id)) {
        reason = "ID is not a positive integer";
        return ERR_MALFORMED_RECORD;
    }
    if (!parseInteger(f[2], age)) {
        reason = "age is not a number";
        return ERR_MALFORMED_RECORD;
    }
    Status check = validateUserFields(id, f[1], age, f[3], f[4], f[5], &reason);
    if (check != OK) return check;

    Status added = net.addUser(id, f[1], age, f[3], f[4], f[5]);
    if (added == ERR_DUPLICATE_ID) reason = "duplicate user ID " + std::to_string(id);
    else if (added == ERR_CAPACITY) reason = "network is full (" + std::to_string(MAX_USERS) + " users)";
    else if (added != OK) reason = statusToString(added);
    return added;
}

/* Parses one friendships.txt record and adds the edge.  Same return style as applyUserRecord. */
Status applyEdgeRecord(SocialNetwork& net, const std::string& line, std::string& reason) {
    std::string f[EDGE_FIELD_COUNT];
    int count = FileManager::splitFields(line, '|', f, EDGE_FIELD_COUNT);
    if (count != EDGE_FIELD_COUNT) {
        reason = "wrong field count (expected 2, got " + std::to_string(count) + ")";
        return ERR_MALFORMED_RECORD;
    }
    int idA = 0, idB = 0;
    if (!FileManager::parsePositiveInt(f[0], idA) || !FileManager::parsePositiveInt(f[1], idB)) {
        reason = "friend IDs must be positive integers";
        return ERR_MALFORMED_RECORD;
    }
    Status added = net.addFriendship(idA, idB);
    if (added == ERR_SELF_FRIEND) reason = "user cannot be friends with themselves";
    else if (added == ERR_NOT_FOUND) reason = "friendship refers to a missing (or rejected) user";
    else if (added == ERR_DUPLICATE_EDGE) reason = "duplicate or reversed friendship";
    else if (added != OK) reason = statusToString(added);
    return added;
}

/* Reads one whole file.  isUserFile selects which record parser is used. */
Status loadOneFile(SocialNetwork& net, const std::string& path, bool isUserFile, LoadReport& report) {
    std::ifstream in(path.c_str());
    if (!in.is_open()) {                       // missing file = empty data (first run)
        if (isUserFile) report.usersFileMissing = true;
        else report.friendsFileMissing = true;
        return OK;
    }
    std::string line;
    int lineNo = 0;
    while (nextRecord(in, line, lineNo)) {
        report.linesRead++;
        std::string reason;
        Status result = isUserFile ? applyUserRecord(net, line, reason)
                                   : applyEdgeRecord(net, line, reason);
        if (result == OK) {
            if (isUserFile) report.usersLoaded++;
            else report.edgesLoaded++;
        } else {
            rejectRecord(report, path, lineNo, reason);
        }
    }
    if (in.bad()) return ERR_FILE_IO;
    return OK;
}

} // namespace

/* ============================================================
 *                         Helpers
 * ============================================================ */

/* Purpose: split on a delimiter.  Complexity: O(length of line). */
int FileManager::splitFields(const std::string& line, char delimiter,
                             std::string fields[], int maxFields) {
    int count = 0;
    std::string current;
    for (int i = 0; i <= (int)line.length(); i++) {
        if (i == (int)line.length() || line[i] == delimiter) {
            if (count < maxFields) fields[count] = current;
            count++;
            current.clear();
        } else {
            current += line[i];
        }
    }
    return count;
}

bool FileManager::parsePositiveInt(const std::string& text, int& out) {
    int value = 0;
    if (!parseInteger(text, value) || value < 1) return false;
    out = value;
    return true;
}

/* ============================================================
 *                        Save / Load
 * ============================================================ */

/*
 * Purpose : write the network to two text files.
 * Edges   : each friendship is written ONCE (only when ownerId < friendId).
 * Outputs : OK | ERR_FILE_IO
 * Complexity: O(V + E)
 */
Status FileManager::saveNetwork(const SocialNetwork& net,
                                const std::string& usersPath,
                                const std::string& friendsPath) {
    std::ofstream usersOut(usersPath.c_str());
    if (!usersOut.is_open()) return ERR_FILE_IO;
    usersOut << "# id|name|age|city|profession|contact\n";
    for (const UserNode* u = net.getHead(); u != nullptr; u = u->next) {
        usersOut << u->id << '|' << u->name << '|' << u->age << '|'
                 << u->city << '|' << u->profession << '|' << u->contact << '\n';
    }
    usersOut.flush();
    if (!usersOut) return ERR_FILE_IO;
    usersOut.close();

    std::ofstream friendsOut(friendsPath.c_str());
    if (!friendsOut.is_open()) return ERR_FILE_IO;
    friendsOut << "# idA|idB  (each friendship once, smaller ID first)\n";
    for (const UserNode* u = net.getHead(); u != nullptr; u = u->next) {
        for (const FriendNode* f = u->friendHead; f != nullptr; f = f->next) {
            if (u->id < f->friendId) friendsOut << u->id << '|' << f->friendId << '\n';
        }
    }
    friendsOut.flush();
    if (!friendsOut) return ERR_FILE_IO;
    friendsOut.close();
    return OK;
}

/*
 * Purpose : rebuild the network from two files using "skip and report".
 * Order   : users file first (so friendships can find their endpoints), then friendships.
 * Complexity: O(V^2 + E*V) because addUser / addFriendship do O(V) lookups.
 */
Status FileManager::loadNetwork(SocialNetwork& net,
                                const std::string& usersPath,
                                const std::string& friendsPath,
                                LoadReport& report) {
    report.reset();
    net.clear();

    Status result = loadOneFile(net, usersPath, true, report);
    if (result != OK) return result;
    return loadOneFile(net, friendsPath, false, report);
}

/* Prints "Load summary" plus every stored rejection. */
void FileManager::printLoadReport(const LoadReport& report) {
    std::cout << "---- Load report ----\n";
    if (report.usersFileMissing)   std::cout << "Users file not found: starting with an empty network.\n";
    if (report.friendsFileMissing) std::cout << "Friendships file not found: no friendships loaded.\n";
    std::cout << "Records read   : " << report.linesRead << "\n"
              << "Users loaded   : " << report.usersLoaded << "\n"
              << "Edges loaded   : " << report.edgesLoaded << "\n"
              << "Rejected       : " << report.rejectedCount << "\n";
    for (int i = 0; i < report.storedErrorCount(); i++) {
        std::cout << "  - " << report.errors[i] << "\n";
    }
    if (report.rejectedCount > MAX_ERRORS) {
        std::cout << "  ... and " << (report.rejectedCount - MAX_ERRORS) << " more rejected record(s).\n";
    }
    std::cout << "---------------------\n";
}
