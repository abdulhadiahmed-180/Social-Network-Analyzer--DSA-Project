/*
 * Security.cpp  -  MODULE C (Member 3): rule engine implementation
 */
#include "Security.h"
#include "Algorithms.h"
#include <iostream>

namespace {

/* Appends one flag if there is room.  Returns false if MAX_FLAGS was reached. */
bool addFlag(Flag out[], int& flagCount, int userId, int ruleId,
             const std::string& reason, int observed, int threshold) {
    if (flagCount >= MAX_FLAGS) return false;
    Flag& f = out[flagCount];
    f.userId = userId;
    f.ruleId = ruleId;
    f.reason = reason;
    f.observedValue = observed;
    f.thresholdValue = threshold;
    flagCount++;
    return true;
}

/* How many accounts (including 'user') have exactly this contact number.  O(V). */
int countSharedContact(const SocialNetwork& net, const UserNode* user) {
    int count = 0;
    for (const UserNode* u = net.getHead(); u != nullptr; u = u->next) {
        if (u->contact == user->contact) count++;
    }
    return count;
}

/*
 * R3 helper: how many of this user's friends share at least one mutual
 * friend with the user (i.e. the user and that friend sit in a triangle).
 * Uses mutualFriends() as agreed in the interface contract.
 */
int countClusteredFriends(const SocialNetwork& net, const UserNode* user) {
    int clustered = 0;
    for (const FriendNode* f = user->friendHead; f != nullptr; f = f->next) {
        SafeIntArray mutual;
        if (mutualFriends(net, user->id, f->friendId, mutual) == OK && mutual.getSize() > 0) {
            clustered++;
        }
    }
    return clustered;
}

/* Runs R1..R4 for one user and appends any flags. */
void checkUser(const SocialNetwork& net, const UserNode* user, const SecurityConfig& config,
               Flag out[], int& flagCount) {
    // R1: too many friends
    if (user->friendCount > config.maxFriendThreshold) {
        addFlag(out, flagCount, user->id, RULE_MANY_FRIENDS,
                "potentially suspicious: unusually high number of friends",
                user->friendCount, config.maxFriendThreshold);
    }
    // R2: isolated account
    if (config.isolationEnabled && user->friendCount == 0) {
        addFlag(out, flagCount, user->id, RULE_ISOLATED,
                "potentially suspicious: isolated account (no friends)",
                user->friendCount, 0);
    }
    // R3: star-like pattern (friends do not know each other)
    if (config.lowClusteringPercent > 0 && user->friendCount >= R3_MIN_FRIENDS) {
        int clustered = countClusteredFriends(net, user);
        // Integer comparison, no rounding:  clustered/friends < X/100
        if (clustered * 100 < config.lowClusteringPercent * user->friendCount) {
            addFlag(out, flagCount, user->id, RULE_LOW_CLUSTERING,
                    "potentially suspicious: few friends know each other "
                    "(observed = % of friends sharing a mutual friend with this user)",
                    clustered * 100 / user->friendCount, config.lowClusteringPercent);
        }
    }
    // R4: shared contact number
    if (config.sharedContactEnabled) {
        int sharing = countSharedContact(net, user);
        if (sharing >= 2) {
            addFlag(out, flagCount, user->id, RULE_SHARED_CONTACT,
                    "potentially suspicious: contact number shared by several accounts "
                    "(observed = number of accounts with this number)",
                    sharing, 2);
        }
    }
}

} // namespace

/*
 * Purpose : run all enabled rules on every user.
 * Complexity: R1/R2 O(V); R3 O(sum over users of d * (V + d)) because each
 *             mutualFriends call re-indexes the graph; R4 O(V^2).
 */
Status SecurityAnalyzer::analyze(const SocialNetwork& net, const SecurityConfig& config,
                                 Flag out[], int& flagCount) {
    flagCount = 0;
    if (net.getUserCount() == 0) return ERR_NETWORK_EMPTY;
    for (const UserNode* u = net.getHead(); u != nullptr; u = u->next) {
        checkUser(net, u, config, out, flagCount);
    }
    return OK;
}

/* Purpose: run all enabled rules on a single user.  Complexity: O(V + d*(V+d)). */
Status SecurityAnalyzer::analyzeUser(const SocialNetwork& net, int id, const SecurityConfig& config,
                                     Flag out[], int& flagCount) {
    flagCount = 0;
    const UserNode* user = net.findUserById(id);
    if (user == nullptr) return ERR_NOT_FOUND;
    checkUser(net, user, config, out, flagCount);
    return OK;
}

Status SecurityAnalyzer::setThreshold(SecurityConfig& config, int newMax) {
    if (newMax < 1 || newMax > MAX_USERS) return ERR_INVALID_INPUT;
    config.maxFriendThreshold = newMax;
    return OK;
}

Status SecurityAnalyzer::setClusteringPercent(SecurityConfig& config, int percent) {
    if (percent < 0 || percent > 100) return ERR_INVALID_INPUT;
    config.lowClusteringPercent = percent;
    return OK;
}

void SecurityAnalyzer::printReport(const Flag flags[], int count) {
    if (count == 0) {
        std::cout << "No potentially suspicious accounts found.\n";
        return;
    }
    std::cout << "---- Security report: " << count << " flag(s) ----\n";
    for (int i = 0; i < count; i++) {
        std::cout << "User " << flags[i].userId << " [R" << flags[i].ruleId << "] "
                  << flags[i].reason << "\n"
                  << "    observed = " << flags[i].observedValue
                  << ", threshold = " << flags[i].thresholdValue << "\n";
    }
    std::cout << "These are only indicators: review each account manually.\n";
}
