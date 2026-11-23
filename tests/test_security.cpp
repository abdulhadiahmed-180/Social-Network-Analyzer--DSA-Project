/* test_security.cpp - SecurityAnalyzer rules R1..R4, settings, edge cases */
#include "TestHelper.h"
#include "Security.h"

static int countRule(const Flag flags[], int count, int ruleId) {
    int n = 0;
    for (int i = 0; i < count; i++) if (flags[i].ruleId == ruleId) n++;
    return n;
}

static bool hasFlag(const Flag flags[], int count, int userId, int ruleId) {
    for (int i = 0; i < count; i++) if (flags[i].userId == userId && flags[i].ruleId == ruleId) return true;
    return false;
}

/*
 * Network used by most tests:
 *   user 1      : hub with friends 2..7 (6 friends who do not know each other)  -> R1 (if limit < 6), R3
 *   user 8      : no friends                                                    -> R2
 *   users 9,10  : friends with each other, SAME contact number                  -> R4 (both)
 *   users 11-14 : a clique of 4 (every friend shares a mutual friend)           -> clean
 */
static void buildNetwork(SocialNetwork& net) {
    addTestUsers(net, 8);
    for (int i = 2; i <= 7; i++) net.addFriendship(1, i);
    net.addUser(9,  "Twin A", 25, "Karachi", "Clerk", "03001234567");
    net.addUser(10, "Twin B", 26, "Karachi", "Clerk", "03001234567");
    net.addFriendship(9, 10);
    for (int i = 11; i <= 14; i++) addTestUser(net, i);
    for (int i = 11; i <= 14; i++) for (int j = i + 1; j <= 14; j++) net.addFriendship(i, j);
}

static void testAllRules() {
    SocialNetwork net;
    buildNetwork(net);
    SecurityConfig cfg;
    cfg.maxFriendThreshold = 5;
    Flag* flags = new Flag[MAX_FLAGS];
    int count = 0;

    CHECK(SecurityAnalyzer::analyze(net, cfg, flags, count) == OK, "analyze succeeds");
    CHECK(countRule(flags, count, RULE_MANY_FRIENDS) == 1 && hasFlag(flags, count, 1, RULE_MANY_FRIENDS), "R1 flags only user 1");
    CHECK(countRule(flags, count, RULE_ISOLATED) == 1 && hasFlag(flags, count, 8, RULE_ISOLATED), "R2 flags only user 8");
    CHECK(countRule(flags, count, RULE_LOW_CLUSTERING) == 1 && hasFlag(flags, count, 1, RULE_LOW_CLUSTERING),
          "R3 flags the star-like hub only");
    CHECK(countRule(flags, count, RULE_SHARED_CONTACT) == 2 && hasFlag(flags, count, 9, RULE_SHARED_CONTACT) &&
          hasFlag(flags, count, 10, RULE_SHARED_CONTACT), "R4 flags both accounts sharing a contact");
    CHECK(count == 5, "5 flags in total");
    CHECK(!hasFlag(flags, count, 11, RULE_LOW_CLUSTERING) && !hasFlag(flags, count, 12, RULE_MANY_FRIENDS),
          "clique members are not flagged");
    bool leavesClean = true;
    for (int i = 2; i <= 7; i++) for (int r = 1; r <= 4; r++) if (hasFlag(flags, count, i, r)) leavesClean = false;
    CHECK(leavesClean, "users with a single friend are not flagged (R3 needs >= 3 friends)");

    for (int i = 0; i < count; i++) {
        if (flags[i].ruleId == RULE_MANY_FRIENDS) {
            CHECK(flags[i].observedValue == 6 && flags[i].thresholdValue == 5, "R1 stores observed 6 and threshold 5");
        }
        if (flags[i].ruleId == RULE_LOW_CLUSTERING) {
            CHECK(flags[i].observedValue == 0 && flags[i].thresholdValue == 20, "R3 stores observed 0% and threshold 20%");
        }
        if (flags[i].ruleId == RULE_SHARED_CONTACT) {
            CHECK(flags[i].observedValue == 2, "R4 stores the number of accounts sharing the number");
        }
        CHECK(containsIgnoreCase(flags[i].reason, "potentially suspicious"), "wording is 'potentially suspicious'");
        CHECK(!containsIgnoreCase(flags[i].reason, "malicious"), "never says 'malicious'");
    }
    SecurityAnalyzer::printReport(flags, count);
    delete[] flags;
}

static void testThresholdBoundary() {
    SocialNetwork net;
    buildNetwork(net);
    SecurityConfig cfg;
    Flag flags[MAX_FLAGS_PER_USER];
    int count = 0;
    cfg.maxFriendThreshold = 6;                      // user 1 has exactly 6: NOT above the limit
    CHECK(SecurityAnalyzer::analyzeUser(net, 1, cfg, flags, count) == OK, "analyzeUser ok");
    CHECK(countRule(flags, count, RULE_MANY_FRIENDS) == 0, "friend count equal to the threshold is not flagged (rule is >)");
    cfg.maxFriendThreshold = 5;
    SecurityAnalyzer::analyzeUser(net, 1, cfg, flags, count);
    CHECK(countRule(flags, count, RULE_MANY_FRIENDS) == 1 && count == 2, "6 > 5: R1 (plus R3) flagged");
}

static void testSwitches() {
    SocialNetwork net;
    buildNetwork(net);
    SecurityConfig cfg;
    cfg.maxFriendThreshold = 5;
    cfg.isolationEnabled = false;
    cfg.sharedContactEnabled = false;
    cfg.lowClusteringPercent = 0;
    Flag* flags = new Flag[MAX_FLAGS];
    int count = 0;
    SecurityAnalyzer::analyze(net, cfg, flags, count);
    CHECK(count == 1 && flags[0].ruleId == RULE_MANY_FRIENDS, "only R1 remains when R2, R3, R4 are switched off");
    cfg.maxFriendThreshold = MAX_USERS;
    SecurityAnalyzer::analyze(net, cfg, flags, count);
    CHECK(count == 0, "everything off: no flags");
    CHECK(SecurityAnalyzer::analyze(net, cfg, flags, count) == OK, "no flags is still OK");
    SecurityAnalyzer::printReport(flags, 0);
    delete[] flags;
}

static void testClusteringBoundary() {
    // user 20 has 4 friends; 21-22 know each other -> 2 of 4 friends (50%) share a mutual friend
    SocialNetwork net;
    for (int id = 20; id <= 24; id++) addTestUser(net, id);
    for (int id = 21; id <= 24; id++) net.addFriendship(20, id);
    net.addFriendship(21, 22);
    SecurityConfig cfg;
    cfg.maxFriendThreshold = 100;
    Flag flags[MAX_FLAGS_PER_USER];
    int count = 0;
    cfg.lowClusteringPercent = 50;
    SecurityAnalyzer::analyzeUser(net, 20, cfg, flags, count);
    CHECK(countRule(flags, count, RULE_LOW_CLUSTERING) == 0, "exactly 50% with a 50% limit: not flagged (rule is <)");
    cfg.lowClusteringPercent = 60;
    SecurityAnalyzer::analyzeUser(net, 20, cfg, flags, count);
    CHECK(countRule(flags, count, RULE_LOW_CLUSTERING) == 1 && flags[0].observedValue == 50 &&
          flags[0].thresholdValue == 60, "50% < 60%: flagged with observed 50, threshold 60");
    // two friends only: R3 not evaluated
    net.removeFriendship(20, 23);
    net.removeFriendship(20, 24);
    SecurityAnalyzer::analyzeUser(net, 20, cfg, flags, count);
    CHECK(count == 0, "fewer than R3_MIN_FRIENDS friends: R3 skipped");
}

static void testEdgeCases() {
    SecurityConfig cfg;
    Flag* flags = new Flag[MAX_FLAGS];
    int count = 99;
    SocialNetwork empty;
    CHECK(SecurityAnalyzer::analyze(empty, cfg, flags, count) == ERR_NETWORK_EMPTY && count == 0, "empty network");
    CHECK(SecurityAnalyzer::analyzeUser(empty, 1, cfg, flags, count) == ERR_NOT_FOUND, "analyzeUser on empty network");

    SocialNetwork single;
    addTestUser(single, 1);
    CHECK(SecurityAnalyzer::analyze(single, cfg, flags, count) == OK && count == 1 &&
          flags[0].ruleId == RULE_ISOLATED, "single user: only the isolated flag");
    CHECK(SecurityAnalyzer::analyzeUser(single, 42, cfg, flags, count) == ERR_NOT_FOUND, "missing user");

    // MAX_USERS isolated users: flag array of MAX_FLAGS never overflows
    SocialNetwork big;
    addTestUsers(big, MAX_USERS);
    CHECK(SecurityAnalyzer::analyze(big, cfg, flags, count) == OK && count == MAX_USERS, "1000 isolated users -> 1000 flags");
    delete[] flags;
}

static void testSettings() {
    SecurityConfig cfg;
    CHECK(SecurityAnalyzer::setThreshold(cfg, 10) == OK && cfg.maxFriendThreshold == 10, "valid threshold");
    CHECK(SecurityAnalyzer::setThreshold(cfg, 0) == ERR_INVALID_INPUT && cfg.maxFriendThreshold == 10, "0 rejected, value kept");
    CHECK(SecurityAnalyzer::setThreshold(cfg, -4) == ERR_INVALID_INPUT, "negative rejected");
    CHECK(SecurityAnalyzer::setThreshold(cfg, MAX_USERS + 1) == ERR_INVALID_INPUT, "too large rejected");
    CHECK(SecurityAnalyzer::setThreshold(cfg, MAX_USERS) == OK, "upper bound accepted");
    CHECK(SecurityAnalyzer::setThreshold(cfg, 1) == OK, "lower bound accepted");
    CHECK(SecurityAnalyzer::setClusteringPercent(cfg, 0) == OK && cfg.lowClusteringPercent == 0, "0% accepted (R3 off)");
    CHECK(SecurityAnalyzer::setClusteringPercent(cfg, 100) == OK, "100% accepted");
    CHECK(SecurityAnalyzer::setClusteringPercent(cfg, 101) == ERR_INVALID_INPUT && cfg.lowClusteringPercent == 100, "101% rejected");
    CHECK(SecurityAnalyzer::setClusteringPercent(cfg, -1) == ERR_INVALID_INPUT, "-1% rejected");
}

int main() {
    testAllRules();
    testThresholdBoundary();
    testSwitches();
    testClusteringBoundary();
    testEdgeCases();
    testSettings();
    return TEST_SUMMARY("test_security");
}
