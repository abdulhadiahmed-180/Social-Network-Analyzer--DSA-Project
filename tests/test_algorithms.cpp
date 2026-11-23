/* test_algorithms.cpp - BFS, DFS, shortest path, components, mutual, recommend, rank */
#include "TestHelper.h"
#include "Algorithms.h"

/*
 * Test graph (users 1..8):
 *      1 --- 2          6 --- 7        8 (isolated)
 *      |     |
 *      3 --- 4 --- 5
 * Edges are added in a fixed order so visiting order is predictable.
 */
static void buildGraph(SocialNetwork& net) {
    addTestUsers(net, 8);
    net.addFriendship(1, 2);
    net.addFriendship(1, 3);
    net.addFriendship(2, 4);
    net.addFriendship(3, 4);
    net.addFriendship(4, 5);
    net.addFriendship(6, 7);
}

static void testBfs() {
    SocialNetwork net;
    buildGraph(net);
    SafeIntArray order;
    CHECK(bfsTraversal(net, 1, order) == OK, "bfs from 1");
    const int expected[] = {1, 2, 3, 4, 5};
    CHECK(arrayEquals(order, expected, 5), "bfs order is level by level, only the component");
    CHECK(bfsTraversal(net, 8, order) == OK && order.getSize() == 1 && order.getOr(0, 0) == 8, "bfs from isolated user");
    CHECK(bfsTraversal(net, 99, order) == ERR_NOT_FOUND && order.getSize() == 0, "bfs from missing user");
    SocialNetwork empty;
    CHECK(bfsTraversal(empty, 1, order) == ERR_NETWORK_EMPTY, "bfs on empty network");

    SocialNetwork single;
    addTestUser(single, 1);
    CHECK(bfsTraversal(single, 1, order) == OK && order.getSize() == 1, "bfs on single user");
}

static void testDfs() {
    SocialNetwork net;
    buildGraph(net);
    SafeIntArray order;
    CHECK(dfsTraversal(net, 1, order) == OK, "dfs from 1");
    const int expected[] = {1, 2, 4, 3, 5};
    CHECK(arrayEquals(order, expected, 5), "dfs goes deep first (1,2,4,3,5)");
    CHECK(dfsTraversal(net, 6, order) == OK && order.getSize() == 2, "dfs in the small component");
    CHECK(dfsTraversal(net, 99, order) == ERR_NOT_FOUND, "dfs from missing user");
    SocialNetwork empty;
    CHECK(dfsTraversal(empty, 1, order) == ERR_NETWORK_EMPTY, "dfs on empty network");
}

static void testDeepChain() {
    // MAX_USERS users in one line: recursion depth 1000 must work
    SocialNetwork net;
    addTestUsers(net, MAX_USERS);
    for (int i = 1; i < MAX_USERS; i++) net.addFriendship(i, i + 1);
    SafeIntArray order, path;
    int distance = 0;
    CHECK(dfsTraversal(net, 1, order) == OK && order.getSize() == MAX_USERS, "dfs on a 1000-user chain");
    CHECK(bfsTraversal(net, 1, order) == OK && order.getSize() == MAX_USERS, "bfs on a 1000-user chain");
    CHECK(shortestPath(net, 1, MAX_USERS, path, distance) == OK && distance == MAX_USERS - 1 &&
          path.getSize() == MAX_USERS, "shortest path along the chain has 999 hops");
    SafeIntArray labels;
    CHECK(connectedComponents(net, labels) == 1, "chain is one component");
}

static void testShortestPath() {
    SocialNetwork net;
    buildGraph(net);
    SafeIntArray path;
    int distance = -5;
    CHECK(shortestPath(net, 1, 5, path, distance) == OK, "path 1 -> 5 exists");
    const int expected[] = {1, 2, 4, 5};
    CHECK(arrayEquals(path, expected, 4) && distance == 3, "path 1,2,4,5 with distance 3");
    CHECK(shortestPath(net, 5, 1, path, distance) == OK && distance == 3 && path.getOr(0, 0) == 5 &&
          path.getOr(3, 0) == 1, "reverse path starts at 5 and ends at 1");
    CHECK(shortestPath(net, 1, 2, path, distance) == OK && distance == 1 && path.getSize() == 2, "direct friends: distance 1");
    CHECK(shortestPath(net, 4, 4, path, distance) == OK && distance == 0 && path.getSize() == 1 &&
          path.getOr(0, 0) == 4, "same source and destination: distance 0");
    CHECK(shortestPath(net, 1, 6, path, distance) == ERR_NO_PATH && path.getSize() == 0, "different components: ERR_NO_PATH");
    CHECK(shortestPath(net, 1, 8, path, distance) == ERR_NO_PATH, "path to isolated user: ERR_NO_PATH");
    CHECK(shortestPath(net, 1, 99, path, distance) == ERR_NOT_FOUND, "missing destination");
    CHECK(shortestPath(net, 99, 1, path, distance) == ERR_NOT_FOUND, "missing source");
    SocialNetwork empty;
    CHECK(shortestPath(empty, 1, 2, path, distance) == ERR_NETWORK_EMPTY, "shortest path on empty network");
}

static void testComponents() {
    SocialNetwork net;
    buildGraph(net);
    SafeIntArray labels;
    CHECK(connectedComponents(net, labels) == 3, "three components");
    CHECK(labels.getSize() == 8, "one label per user");
    bool firstGroup = true;
    for (int i = 0; i < 5; i++) if (labels.getOr(i, -1) != 0) firstGroup = false;
    CHECK(firstGroup, "users 1-5 share label 0");
    CHECK(labels.getOr(5, -1) == 1 && labels.getOr(6, -1) == 1, "users 6,7 share label 1");
    CHECK(labels.getOr(7, -1) == 2, "user 8 alone has label 2");

    SocialNetwork empty;
    CHECK(connectedComponents(empty, labels) == 0 && labels.getSize() == 0, "empty network: 0 components");
    SocialNetwork single;
    addTestUser(single, 1);
    CHECK(connectedComponents(single, labels) == 1, "single user: 1 component");

    net.addFriendship(5, 6);                         // merge two groups
    CHECK(connectedComponents(net, labels) == 2, "adding a bridge merges components");
    net.removeUser(4);                               // splits 1,2,3 from 5
    CHECK(connectedComponents(net, labels) == 3, "removing user 4 splits {1,2,3} from {5,6,7}, plus isolated 8");
}

static void testMutualFriends() {
    SocialNetwork net;
    buildGraph(net);
    SafeIntArray result;
    CHECK(mutualFriends(net, 2, 3, result) == OK, "mutual(2,3)");
    const int expected[] = {1, 4};
    CHECK(arrayEquals(result, expected, 2), "2 and 3 share friends 1 and 4");
    CHECK(mutualFriends(net, 1, 5, result) == OK && result.getSize() == 0, "no mutual friends gives OK and empty");
    CHECK(mutualFriends(net, 1, 4, result) == OK && result.getSize() == 2, "1 and 4 share 2 and 3 even though not friends");
    CHECK(mutualFriends(net, 8, 1, result) == OK && result.getSize() == 0, "user with 0 friends");
    CHECK(mutualFriends(net, 1, 1, result) == ERR_INVALID_INPUT, "same user twice");
    CHECK(mutualFriends(net, 1, 99, result) == ERR_NOT_FOUND, "missing user");
    SocialNetwork empty;
    CHECK(mutualFriends(empty, 1, 2, result) == ERR_NETWORK_EMPTY, "empty network");
}

static void testRecommend() {
    SocialNetwork net;
    buildGraph(net);
    RecommendEntry out[10];
    int count = 0;
    CHECK(recommendFriends(net, 1, 5, out, count) == OK, "recommend for 1");
    CHECK(count == 1 && out[0].id == 4 && out[0].mutualCount == 2, "only candidate is 4 with 2 mutual friends");
    CHECK(out[0].name == "User4", "name copied into the entry");
    CHECK(recommendFriends(net, 8, 5, out, count) == OK && count == 0, "user with 0 friends: no suggestions");
    CHECK(recommendFriends(net, 5, 5, out, count) == OK && count == 2, "user 5: suggests 2 and 3 (friends of 4)");
    CHECK(out[0].id == 2 && out[1].id == 3, "equal mutual counts are ordered by ID");

    // second graph to test ranking and top-K cut
    SocialNetwork g;
    const int ids[] = {10, 11, 12, 13, 20, 21, 22};
    for (int i = 0; i < 7; i++) addTestUser(g, ids[i]);
    g.addFriendship(10, 11); g.addFriendship(10, 12); g.addFriendship(10, 13);
    g.addFriendship(11, 20); g.addFriendship(12, 20);        // 20: 2 mutual
    g.addFriendship(12, 21); g.addFriendship(13, 21);        // 21: 2 mutual
    g.addFriendship(13, 22);                                 // 22: 1 mutual
    RecommendEntry big[10];
    CHECK(recommendFriends(g, 10, 10, big, count) == OK && count == 3, "three candidates");
    CHECK(big[0].id == 20 && big[1].id == 21 && big[2].id == 22, "sorted by mutual desc, ties by ID asc");
    CHECK(big[0].mutualCount == 2 && big[2].mutualCount == 1, "mutual counts correct");
    CHECK(recommendFriends(g, 10, 2, big, count) == OK && count == 2 && big[0].id == 20 && big[1].id == 21,
          "topK cuts the list");
    CHECK(recommendFriends(g, 10, 1, big, count) == OK && count == 1 && big[0].id == 20, "topK = 1");
    CHECK(recommendFriends(g, 10, 500, big, count) == OK && count == 3, "topK larger than candidates");
    CHECK(recommendFriends(g, 10, 0, big, count) == ERR_INVALID_INPUT, "topK 0 invalid");
    CHECK(recommendFriends(g, 10, -3, big, count) == ERR_INVALID_INPUT, "negative topK invalid");
    CHECK(recommendFriends(g, 99, 3, big, count) == ERR_NOT_FOUND, "missing user");
    // a later candidate with a smaller ID must jump ahead of an equal-count earlier one
    g.addUser(5, "Early", 20, "K", "S", "9230099999");
    g.addFriendship(5, 11); g.addFriendship(5, 12);         // 5: 2 mutual (via 11,12), added AFTER 20/21 in list order
    CHECK(recommendFriends(g, 10, 10, big, count) == OK && big[0].id == 5 && big[1].id == 20 && big[2].id == 21,
          "tie-break by ID even when list order differs");
    SocialNetwork empty;
    CHECK(recommendFriends(empty, 1, 3, big, count) == ERR_NETWORK_EMPTY, "empty network");
}

static void testRank() {
    SocialNetwork net;
    buildGraph(net);
    RankEntry out[MAX_USERS];
    int count = 0;
    CHECK(rankUsersByFriends(net, out, count) == OK && count == 8, "rank returns every user");
    const int expectedOrder[] = {4, 1, 2, 3, 5, 6, 7, 8};
    bool orderOk = true;
    for (int i = 0; i < 8; i++) if (out[i].id != expectedOrder[i]) orderOk = false;
    CHECK(orderOk, "order: 4(3) then 1,2,3(2) then 5,6,7(1) then 8(0), ties by ID");
    CHECK(out[0].friendCount == 3 && out[7].friendCount == 0, "friend counts stored");

    // users added in descending ID order, no friendships: ties must be sorted by ID ascending
    SocialNetwork reversed;
    for (int i = 6; i >= 1; i--) addTestUser(reversed, i);
    CHECK(rankUsersByFriends(reversed, out, count) == OK && count == 6, "rank on reversed input");
    bool ascending = true;
    for (int i = 0; i < 6; i++) if (out[i].id != i + 1) ascending = false;
    CHECK(ascending, "ties sorted by ID ascending");

    // already sorted input and single user
    SocialNetwork single;
    addTestUser(single, 1);
    CHECK(rankUsersByFriends(single, out, count) == OK && count == 1, "rank single user");
    SocialNetwork empty;
    CHECK(rankUsersByFriends(empty, out, count) == ERR_NETWORK_EMPTY && count == 0, "rank empty network");
}

static void testDfsVisitDirect() {
    SocialNetwork net;
    buildGraph(net);
    net.assignIndices();
    SafeIntArray visited(net.getUserCount());
    SafeIntArray order;
    dfsVisit(net.findUserById(6), visited, order);
    CHECK(order.getSize() == 2 && order.getOr(0, 0) == 6 && order.getOr(1, 0) == 7, "dfsVisit helper directly");
    dfsVisit(nullptr, visited, order);
    CHECK(order.getSize() == 2, "dfsVisit(nullptr) does nothing");
}

int main() {
    testBfs();
    testDfs();
    testDeepChain();
    testShortestPath();
    testComponents();
    testMutualFriends();
    testRecommend();
    testRank();
    testDfsVisitDirect();
    return TEST_SUMMARY("test_algorithms");
}
