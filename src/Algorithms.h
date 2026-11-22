/*
 * Algorithms.h  -  MODULE B (Member 2): graph algorithms
 * ---------------------------------------------------------------
 * All functions take the network by const reference and only READ it
 * (through getHead() / findUserById() / friendHead lists).
 * They call net.assignIndices() first, so every node has a tempIndex
 * 0..V-1 that is used to index the visited / parent / counts arrays.
 *
 * Output arrays (SafeIntArray) are cleared at the start of each call.
 * Neighbours are always visited in friend-list (insertion) order, so
 * the results are deterministic and testable.
 *
 * V = users, E = friendships.
 */
#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "Graph.h"
#include "Queue.h"
#include "Utils.h"

/* BFS from startId.  visitOrder receives user IDs in visiting order.
 * Returns OK | ERR_NETWORK_EMPTY | ERR_NOT_FOUND.            O(V + E) */
Status bfsTraversal(const SocialNetwork& net, int startId, SafeIntArray& visitOrder);

/* Fewest-hops path between two users (BFS + parent array).
 * pathIds = IDs from src to dst inclusive; distance = degrees of separation
 * (0 when src == dst).  Returns OK | ERR_NETWORK_EMPTY | ERR_NOT_FOUND | ERR_NO_PATH.   O(V + E) */
Status shortestPath(const SocialNetwork& net, int srcId, int dstId,
                    SafeIntArray& pathIds, int& distance);

/* Recursive DFS from startId.  Returns OK | ERR_NETWORK_EMPTY | ERR_NOT_FOUND.   O(V + E)
 * Recursion depth is at most V <= MAX_USERS (1000). */
Status dfsTraversal(const SocialNetwork& net, int startId, SafeIntArray& visitOrder);

/* Recursive helper used by dfsTraversal (public so it can be unit tested).
 * REQUIRES net.assignIndices() to have been called, and 'visited' sized V. */
void dfsVisit(const UserNode* node, SafeIntArray& visited, SafeIntArray& order);

/* Labels every user with a component number 0..k-1 and returns k.
 * componentLabel[i] is the label of the user whose tempIndex is i
 * (= position i in master-list order).  Empty network -> returns 0.   O(V + E) */
int connectedComponents(const SocialNetwork& net, SafeIntArray& componentLabel);

/* Users who are friends with BOTH idA and idB (in idB's friend-list order).
 * Returns OK | ERR_NETWORK_EMPTY | ERR_NOT_FOUND | ERR_INVALID_INPUT (idA == idB).   O(V + dA + dB) */
Status mutualFriends(const SocialNetwork& net, int idA, int idB, SafeIntArray& resultIds);

/* Top-K friend suggestions: friends-of-friends who are not the user and not
 * already friends, ranked by number of mutual friends (desc), ties by ID (asc).
 * 'out' must have room for at least topK entries.  outCount may be < topK.
 * Returns OK | ERR_INVALID_INPUT (topK < 1) | ERR_NETWORK_EMPTY | ERR_NOT_FOUND.   */
Status recommendFriends(const SocialNetwork& net, int userId, int topK,
                        RecommendEntry out[], int& outCount);

/* Ranks ALL users by friend count (desc), ties by ID (asc) using insertion sort.
 * 'out' must have room for getUserCount() entries.
 * Returns OK | ERR_NETWORK_EMPTY.   O(V^2) worst case, O(V) if already sorted. */
Status rankUsersByFriends(const SocialNetwork& net, RankEntry out[], int& outCount);

#endif
