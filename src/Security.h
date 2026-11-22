/*
 * Security.h  -  MODULE C (Member 3): suspicious-account detection
 * ---------------------------------------------------------------
 * Rule-based and purely structural (looks only at the friend graph and
 * contact numbers).  Results are only "potentially suspicious" - a human
 * must decide; the tool never claims an account is malicious.
 *
 *   R1  friend count > maxFriendThreshold
 *   R2  user has 0 friends (isolated account)
 *   R3  low clustering: fewer than X% of the user's friends share a mutual
 *       friend with the user (star-like pattern).  Only checked for users with
 *       at least R3_MIN_FRIENDS friends.  X = 0 switches R3 off.
 *   R4  two or more accounts share the same contact number
 *
 * Every flag records the rule, the observed value and the threshold.
 */
#ifndef SECURITY_H
#define SECURITY_H

#include "Graph.h"
#include "Utils.h"

class SecurityAnalyzer {
public:
    /* Checks every user.  'out' must hold at least MAX_FLAGS entries.
     * Returns OK | ERR_NETWORK_EMPTY.  Flags are ordered by user (master-list order). */
    static Status analyze(const SocialNetwork& net, const SecurityConfig& config,
                          Flag out[], int& flagCount);

    /* Checks one user.  'out' must hold at least MAX_FLAGS_PER_USER entries.
     * Returns OK | ERR_NOT_FOUND. */
    static Status analyzeUser(const SocialNetwork& net, int id, const SecurityConfig& config,
                              Flag out[], int& flagCount);

    /* Sets R1's threshold.  Valid range: 1..MAX_USERS.  Returns OK | ERR_INVALID_INPUT. */
    static Status setThreshold(SecurityConfig& config, int newMax);

    /* Sets R3's percentage.  Valid range: 0..100 (0 = off).  Returns OK | ERR_INVALID_INPUT. */
    static Status setClusteringPercent(SecurityConfig& config, int percent);

    /* Prints the flags in a readable list. */
    static void printReport(const Flag flags[], int count);
};

#endif
