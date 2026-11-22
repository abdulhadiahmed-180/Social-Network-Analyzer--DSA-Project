/* test_files.cpp - FileManager: save/load round trip, malformed records, missing files */
#include <fstream>
#include "TestHelper.h"
#include "FileManager.h"

/* Writes 'content' to 'path' (overwrites). */
static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path.c_str());
    out << content;
}

/* Counts non-blank, non-comment lines of a file (-1 if it cannot be opened). */
static int countRecordLines(const std::string& path) {
    std::ifstream in(path.c_str());
    if (!in.is_open()) return -1;
    std::string line;
    int count = 0;
    while (std::getline(in, line)) {
        std::string t = trim(line);
        if (!t.empty() && t[0] != '#') count++;
    }
    return count;
}

static const char* U = "tmp_users.txt";
static const char* F = "tmp_friends.txt";

static void testRoundTrip() {
    SocialNetwork a;
    a.addUser(1, "Ali Khan", 21, "Karachi", "Student", "+923001234567");
    a.addUser(5, "Sara", 30, "Lahore", "Engineer", "03001234567");
    a.addUser(3, "Omar", 44, "Multan", "Doctor", "03111234567");
    a.addUser(9, "Lonely", 50, "Quetta", "Farmer", "03211234567");
    a.addFriendship(5, 1); a.addFriendship(3, 5); a.addFriendship(1, 3);

    CHECK(FileManager::saveNetwork(a, U, F) == OK, "save succeeds");
    CHECK(countRecordLines(U) == 4, "users file has 4 records");
    CHECK(countRecordLines(F) == 3, "each edge saved exactly once (3 edges, not 6)");

    SocialNetwork b;
    addTestUser(b, 777);                              // must disappear: load replaces the content
    LoadReport report;
    CHECK(FileManager::loadNetwork(b, U, F, report) == OK, "load succeeds");
    CHECK(report.usersLoaded == 4 && report.edgesLoaded == 3 && report.rejectedCount == 0, "report counts");
    CHECK(report.linesRead == 7, "7 record lines read");
    CHECK(!report.usersFileMissing && !report.friendsFileMissing, "files were found");
    CHECK(b.findUserById(777) == nullptr, "old content cleared by load");
    CHECK(b.getUserCount() == 4 && b.getEdgeCount() == 3, "loaded counts match");
    const UserNode* u = b.findUserById(1);
    CHECK(u != nullptr && u->name == "Ali Khan" && u->age == 21 && u->city == "Karachi" &&
          u->profession == "Student" && u->contact == "+923001234567", "all fields survive the round trip");
    CHECK(b.areFriends(1, 5) && b.areFriends(3, 5) && b.areFriends(1, 3) && !b.areFriends(9, 1), "edges survive");
    CHECK(b.getHead()->id == 1 && b.getHead()->next->id == 5, "user order preserved");
}

static void testMissingFiles() {
    SocialNetwork net;
    addTestUser(net, 1);
    LoadReport report;
    CHECK(FileManager::loadNetwork(net, "tmp_does_not_exist_u.txt", "tmp_does_not_exist_f.txt", report) == OK,
          "missing files are not an error");
    CHECK(net.getUserCount() == 0, "missing files give an empty network");
    CHECK(report.usersFileMissing && report.friendsFileMissing, "report says both files are missing");
    CHECK(report.rejectedCount == 0 && report.linesRead == 0, "no records, no errors");

    writeFile(U, "1|Ali|20|Karachi|Student|03001234567\n");
    CHECK(FileManager::loadNetwork(net, U, "tmp_does_not_exist_f.txt", report) == OK && net.getUserCount() == 1 &&
          !report.usersFileMissing && report.friendsFileMissing, "users present, friendships file missing");
}

static void testMixedValidAndInvalid() {
    writeFile(U,
        "# users file\n"                                        // line 1  comment
        "1|Ali Khan|21|Karachi|Student|+923001234567\n"         // line 2  valid
        "2|Sara|25|Lahore|Engineer|03001234567\n"               // line 3  valid
        "this is not a record\n"                                // line 4  wrong field count
        "3|Omar|12|Karachi|Student|03001234567\n"               // line 5  age too low
        "1|Duplicate|30|X|Y|03001234567\n"                      // line 6  duplicate ID
        "abc|Name|20|City|Prof|03001234567\n"                   // line 7  non-numeric ID
        "-5|Name|20|City|Prof|03001234567\n"                    // line 8  negative ID
        "4|Zed|20|City|Prof|12ab567\n"                          // line 9  bad contact
        "\n"                                                    // line 10 blank
        "6|Pipe|20|City|Prof|03001234567|extra\n"               // line 11 too many fields
        "8|   |20|City|Prof|03001234567\n"                      // line 12 blank name
        "9|Old|twenty|City|Prof|03001234567\n"                  // line 13 age not a number
        "7|Hina|30|Karachi|Doctor|03001234567\n");              // line 14 valid
    writeFile(F,
        "1|2\n"        // valid
        "2|1\n"        // reversed duplicate
        "1|1\n"        // self loop
        "1|99\n"       // missing endpoint
        "7|2\n"        // valid
        "x|y\n"        // non numeric
        "3|1\n"        // endpoint 3 was rejected
        "1\n"          // wrong field count
        "1|2\n");      // exact duplicate

    SocialNetwork net;
    LoadReport r;
    CHECK(FileManager::loadNetwork(net, U, F, r) == OK, "load returns OK even with bad records");
    CHECK(r.usersLoaded == 3, "3 valid users loaded (1, 2, 7)");
    CHECK(r.edgesLoaded == 2, "2 valid edges loaded");
    CHECK(r.rejectedCount == 9 + 7, "rejected: 9 bad user records + 7 bad edges");
    CHECK(r.linesRead == r.usersLoaded + r.edgesLoaded + r.rejectedCount, "invariant: every record is loaded or rejected");
    CHECK(net.getUserCount() == 3 && net.getEdgeCount() == 2, "network holds only the valid data");
    CHECK(net.findUserById(1)->name == "Ali Khan", "first record not overwritten by the duplicate ID");
    CHECK(net.areFriends(1, 2) && net.areFriends(7, 2), "valid edges present");

    CHECK(r.storedErrorCount() == r.rejectedCount, "all errors stored (fewer than MAX_ERRORS)");
    CHECK(containsIgnoreCase(r.errors[0], U), "error message names the file");
    CHECK(containsIgnoreCase(r.errors[0], "line 4"), "error message gives the physical line number (comment counted)");
    CHECK(containsIgnoreCase(r.errors[0], "field count"), "error message gives the reason");
    CHECK(containsIgnoreCase(r.errors[1], "line 5") && containsIgnoreCase(r.errors[1], "age"), "age error on line 5");
    CHECK(containsIgnoreCase(r.errors[2], "duplicate"), "duplicate ID reported");
    CHECK(containsIgnoreCase(r.errors[3], "ID"), "non-numeric ID reported");
    CHECK(containsIgnoreCase(r.errors[9], F) && containsIgnoreCase(r.errors[9], "duplicate"),
          "first edge error names the friendships file (reversed duplicate)");
    CHECK(containsIgnoreCase(r.errors[10], "themselves"), "self loop reported");
    CHECK(containsIgnoreCase(r.errors[11], "missing"), "missing endpoint reported");
}

static void testEdgeToRejectedUser() {
    writeFile(U, "1|Ali|20|Karachi|Student|03001234567\n2|Bad|5|Karachi|Student|03001234567\n");
    writeFile(F, "1|2\n");
    SocialNetwork net;
    LoadReport r;
    FileManager::loadNetwork(net, U, F, r);
    CHECK(r.usersLoaded == 1 && r.edgesLoaded == 0 && r.rejectedCount == 2, "edge to a rejected user is rejected too");
    CHECK(net.getEdgeCount() == 0, "no edge created");
}

static void testEmptyAndCorruptFiles() {
    SocialNetwork net;
    LoadReport r;
    writeFile(U, "");
    writeFile(F, "");
    CHECK(FileManager::loadNetwork(net, U, F, r) == OK && net.getUserCount() == 0 && r.rejectedCount == 0,
          "empty files = empty network, no errors");

    writeFile(U, "# only comments\n\n   \n# more\n");
    CHECK(FileManager::loadNetwork(net, U, F, r) == OK && r.linesRead == 0, "comments and blank lines ignored");

    writeFile(U, "@@@@\n|||||\n12345\n\x01\x02\x03\n||||||||\n");
    writeFile(F, "garbage\n|\n||\n");
    CHECK(FileManager::loadNetwork(net, U, F, r) == OK, "corrupt files do not crash the loader");
    CHECK(net.getUserCount() == 0 && r.rejectedCount == r.linesRead && r.linesRead == 8, "every corrupt record rejected (5 + 3)");

    // Windows line endings
    writeFile(U, "1|Ali|20|Karachi|Student|03001234567\r\n2|Sara|25|Lahore|Doctor|03007654321\r\n");
    writeFile(F, "1|2\r\n");
    CHECK(FileManager::loadNetwork(net, U, F, r) == OK && r.usersLoaded == 2 && r.edgesLoaded == 1 &&
          r.rejectedCount == 0, "CRLF line endings accepted");
    CHECK(net.findUserById(1)->contact == "03001234567", "no stray \\r in loaded fields");

    // UTF-8 byte order mark on the first line
    writeFile(U, "\xEF\xBB\xBF" "1|Ali|20|Karachi|Student|03001234567\n");
    writeFile(F, "");
    CHECK(FileManager::loadNetwork(net, U, F, r) == OK && r.usersLoaded == 1, "UTF-8 BOM on line 1 is skipped");

    // whitespace around fields
    writeFile(U, "  1 | Ali | 20 | Karachi | Student | 03001234567  \n");
    CHECK(FileManager::loadNetwork(net, U, F, r) == OK && r.usersLoaded == 1 && net.findUserById(1)->name == "Ali",
          "spaces around fields are trimmed");
}

static void testManyErrors() {
    std::string bad;
    for (int i = 0; i < 150; i++) bad += "bad line number " + std::to_string(i) + "\n";
    writeFile(U, bad);
    writeFile(F, "");
    SocialNetwork net;
    LoadReport r;
    CHECK(FileManager::loadNetwork(net, U, F, r) == OK, "load survives 150 bad records");
    CHECK(r.rejectedCount == 150, "all 150 rejections are counted");
    CHECK(r.storedErrorCount() == MAX_ERRORS, "only MAX_ERRORS messages are stored");
    CHECK(containsIgnoreCase(r.errors[MAX_ERRORS - 1], "line 100"), "last stored message is for line 100");
    FileManager::printLoadReport(r);                  // exercises the "... and 50 more" branch
}

static void testSaveFailures() {
    SocialNetwork net;
    addTestUser(net, 1);
    CHECK(FileManager::saveNetwork(net, "no_such_folder_xyz/u.txt", F) == ERR_FILE_IO, "unwritable users path");
    CHECK(FileManager::saveNetwork(net, U, "no_such_folder_xyz/f.txt") == ERR_FILE_IO, "unwritable friends path");
    SocialNetwork empty;
    CHECK(FileManager::saveNetwork(empty, U, F) == OK, "saving an empty network works");
    CHECK(countRecordLines(U) == 0 && countRecordLines(F) == 0, "empty network writes no records");
    LoadReport r;
    CHECK(FileManager::loadNetwork(net, U, F, r) == OK && net.getUserCount() == 0, "reloading the empty save");
}

static void testBigRoundTrip() {
    SocialNetwork a;
    addTestUsers(a, 200);
    for (int i = 1; i < 200; i++) a.addFriendship(i, i + 1);
    for (int i = 1; i <= 190; i += 10) a.addFriendship(i, i + 5);   // extra shortcuts
    CHECK(FileManager::saveNetwork(a, U, F) == OK, "save 200 users");
    SocialNetwork b;
    LoadReport r;
    CHECK(FileManager::loadNetwork(b, U, F, r) == OK && r.rejectedCount == 0, "load 200 users");
    CHECK(b.getUserCount() == 200 && b.getEdgeCount() == a.getEdgeCount(), "same size after round trip");
    bool same = true;
    for (int i = 1; i <= 200 && same; i++) {
        if (a.getFriendCount(i) != b.getFriendCount(i)) same = false;
    }
    CHECK(same, "every user keeps its friend count");
}

int main() {
    testRoundTrip();
    testMissingFiles();
    testMixedValidAndInvalid();
    testEdgeToRejectedUser();
    testEmptyAndCorruptFiles();
    testManyErrors();
    testSaveFailures();
    testBigRoundTrip();
    return TEST_SUMMARY("test_files");
}
