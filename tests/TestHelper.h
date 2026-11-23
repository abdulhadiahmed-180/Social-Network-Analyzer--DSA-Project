/*
 * TestHelper.h  -  tiny test framework (no external libraries)
 * ---------------------------------------------------------------
 * CHECK(condition, message)  counts a pass or prints a FAIL line.
 * TEST_SUMMARY(name)         prints totals; evaluates to the process exit code.
 * Each test file is its own program and needs no keyboard input.
 */
#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <iostream>
#include <string>
#include "Graph.h"

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(cond, msg)                                                           \
    do {                                                                           \
        if (cond) { ++g_passed; }                                                  \
        else { ++g_failed; std::cout << "  FAIL: " << msg                          \
                                     << "  [" << __FILE__ << ":" << __LINE__ << "]\n"; } \
    } while (0)

/* Prints the totals and returns the exit code (0 = all passed). */
inline int testSummary(const char* name) {
    std::cout << name << ": " << g_passed << " passed, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
#define TEST_SUMMARY(name) testSummary(name)

/* ---- builders used by several test files ---- */

/* Adds a valid user with a UNIQUE contact number derived from the ID. */
inline Status addTestUser(SocialNetwork& net, int id) {
    return net.addUser(id, "User" + std::to_string(id), 20 + (id % 30), "Karachi", "Student",
                       "92300" + std::to_string(100000 + id));
}

/* Adds users 1..n. */
inline void addTestUsers(SocialNetwork& net, int n) {
    for (int i = 1; i <= n; i++) addTestUser(net, i);
}

/* true if the array contains exactly the given IDs in the given order. */
inline bool arrayEquals(const SafeIntArray& a, const int expected[], int count) {
    if (a.getSize() != count) return false;
    for (int i = 0; i < count; i++) {
        if (a.getOr(i, -999) != expected[i]) return false;
    }
    return true;
}

#endif
