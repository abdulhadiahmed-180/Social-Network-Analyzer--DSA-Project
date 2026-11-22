/*
 * Algorithms.cpp  -  MODULE B (Member 2): BFS, DFS, shortest path,
 *                    components, mutual friends, recommendations, ranking.
 */
#include "Algorithms.h"

namespace {

/*
 * NodeTable - small helper that maps tempIndex -> UserNode*.
 * Built in O(V) so the algorithms can turn an index taken from the
 * queue back into a node without an O(V) search each time.
 * It owns a raw array, so copying is forbidden (a copy would double-free).
 */
class NodeTable {
public:
    explicit NodeTable(const SocialNetwork& net) : nodes(nullptr), size(net.getUserCount()) {
        net.assignIndices();
        if (size > 0) {
            nodes = new const UserNode*[size];
            for (int i = 0; i < size; i++) nodes[i] = nullptr;
            for (const UserNode* n = net.getHead(); n != nullptr; n = n->next) {
                if (n->tempIndex >= 0 && n->tempIndex < size) nodes[n->tempIndex] = n;
            }
        }
    }
    ~NodeTable() { delete[] nodes; }

    const UserNode* at(int index) const {
        return (nodes != nullptr && index >= 0 && index < size) ? nodes[index] : nullptr;
    }
    int getSize() const { return size; }

private:
    const UserNode** nodes;
    int size;
    NodeTable(const NodeTable&) = delete;
    NodeTable& operator=(const NodeTable&) = delete;
};

/* Out-of-range indices count as "already visited" so a bug can never crash us. */
bool isVisited(const SafeIntArray& visited, int index) {
    return visited.getOr(index, 1) != 0;
}

/* Recursive flood-fill used by connectedComponents (labels every node reachable from 'node'). */
void labelComponent(const UserNode* node, SafeIntArray& labels, int component) {
    labels.set(node->tempIndex, component);
    for (const FriendNode* f = node->friendHead; f != nullptr; f = f->next) {
        const UserNode* neighbour = f->friendPtr;
        if (neighbour != nullptr && labels.getOr(neighbour->tempIndex, 0) == -1) {
            labelComponent(neighbour, labels, component);
        }
    }
}

/* Ordering used by recommendFriends: more mutual friends first, then smaller ID. */
bool recommendComesBefore(const RecommendEntry& a, const RecommendEntry& b) {
    if (a.mutualCount != b.mutualCount) return a.mutualCount > b.mutualCount;
    return a.id < b.id;
}

/* Ordering used by rankUsersByFriends: more friends first, then smaller ID. */
bool rankComesBefore(const RankEntry& a, const RankEntry& b) {
    if (a.friendCount != b.friendCount) return a.friendCount > b.friendCount;
    return a.id < b.id;
}

} // namespace

/* ============================================================
 *                          BFS
 * ============================================================ */

/*
 * Purpose : breadth-first traversal (level by level) from startId.
 * Method  : queue + visited array.  A node is marked visited when it is
 *           ENQUEUED, so it enters the queue at most once.
 * Outputs : visitOrder = IDs in the order they were dequeued.
 * Complexity: O(V + E)
 */
Status bfsTraversal(const SocialNetwork& net, int startId, SafeIntArray& visitOrder) {
    visitOrder.clear();
    if (net.getUserCount() == 0) return ERR_NETWORK_EMPTY;
    const UserNode* start = net.findUserById(startId);
    if (start == nullptr) return ERR_NOT_FOUND;

    NodeTable table(net);                       // also runs assignIndices()
    int n = table.getSize();
    SafeIntArray visited(n);
    IntQueue queue(n);

    visited.set(start->tempIndex, 1);
    queue.enqueue(start->tempIndex);

    int current;
    while (queue.dequeue(current)) {
        const UserNode* node = table.at(current);
        if (node == nullptr) continue;
        visitOrder.append(node->id);
        for (const FriendNode* f = node->friendHead; f != nullptr; f = f->next) {
            int neighbourIndex = f->friendPtr->tempIndex;
            if (!isVisited(visited, neighbourIndex)) {
                visited.set(neighbourIndex, 1);
                queue.enqueue(neighbourIndex);
            }
        }
    }
    return OK;
}

/* ============================================================
 *                      Shortest path
 * ============================================================ */

/*
 * Purpose : fewest-hops path between two users ("degrees of separation").
 * Method  : BFS records parent[child] = node it was discovered from.
 *           Walking parent[] back from the destination gives the path
 *           in reverse; it is then reversed into pathIds.
 * Outputs : pathIds (src ... dst), distance = number of edges on the path.
 * Complexity: O(V + E)
 */
Status shortestPath(const SocialNetwork& net, int srcId, int dstId,
                    SafeIntArray& pathIds, int& distance) {
    pathIds.clear();
    distance = -1;
    if (net.getUserCount() == 0) return ERR_NETWORK_EMPTY;
    const UserNode* src = net.findUserById(srcId);
    const UserNode* dst = net.findUserById(dstId);
    if (src == nullptr || dst == nullptr) return ERR_NOT_FOUND;

    NodeTable table(net);
    int n = table.getSize();
    SafeIntArray visited(n);
    SafeIntArray parent(n);
    parent.fill(-1);
    IntQueue queue(n);

    int srcIndex = src->tempIndex;
    int dstIndex = dst->tempIndex;
    visited.set(srcIndex, 1);
    queue.enqueue(srcIndex);

    bool found = (srcIndex == dstIndex);        // same user: path of length 0
    int current;
    while (!found && queue.dequeue(current)) {
        const UserNode* node = table.at(current);
        if (node == nullptr) continue;
        for (const FriendNode* f = node->friendHead; f != nullptr && !found; f = f->next) {
            int neighbourIndex = f->friendPtr->tempIndex;
            if (!isVisited(visited, neighbourIndex)) {
                visited.set(neighbourIndex, 1);
                parent.set(neighbourIndex, current);
                queue.enqueue(neighbourIndex);
                if (neighbourIndex == dstIndex) found = true;
            }
        }
    }
    if (!found) return ERR_NO_PATH;

    // Walk parent[] from destination back to the source (parent of source is -1).
    SafeIntArray reversed;
    int index = dstIndex;
    while (index != -1) {
        const UserNode* node = table.at(index);
        if (node == nullptr) break;
        reversed.append(node->id);
        index = parent.getOr(index, -1);
    }
    // Reverse into pathIds so it reads source -> destination.
    for (int i = reversed.getSize() - 1; i >= 0; i--) {
        pathIds.append(reversed.getOr(i, 0));
    }
    distance = pathIds.getSize() - 1;
    return OK;
}

/* ============================================================
 *                          DFS
 * ============================================================ */

/*
 * Purpose : recursive helper: mark 'node', record its ID, then recurse into
 *           each unvisited neighbour (depth first).
 * Depth   : at most V <= MAX_USERS frames.
 */
void dfsVisit(const UserNode* node, SafeIntArray& visited, SafeIntArray& order) {
    if (node == nullptr) return;
    visited.set(node->tempIndex, 1);
    order.append(node->id);
    for (const FriendNode* f = node->friendHead; f != nullptr; f = f->next) {
        const UserNode* neighbour = f->friendPtr;
        if (neighbour != nullptr && !isVisited(visited, neighbour->tempIndex)) {
            dfsVisit(neighbour, visited, order);
        }
    }
}

/* Purpose: depth-first traversal from startId.  Complexity: O(V + E) */
Status dfsTraversal(const SocialNetwork& net, int startId, SafeIntArray& visitOrder) {
    visitOrder.clear();
    if (net.getUserCount() == 0) return ERR_NETWORK_EMPTY;
    const UserNode* start = net.findUserById(startId);
    if (start == nullptr) return ERR_NOT_FOUND;

    net.assignIndices();
    SafeIntArray visited(net.getUserCount());
    dfsVisit(start, visited, visitOrder);
    return OK;
}

/* ============================================================
 *                  Connected components
 * ============================================================ */

/*
 * Purpose : split the network into groups of mutually reachable users.
 * Method  : for every user still unlabelled, start a recursive DFS and give
 *           everything it reaches the same label.
 * Complexity: O(V + E)
 */
int connectedComponents(const SocialNetwork& net, SafeIntArray& componentLabel) {
    componentLabel.clear();
    int n = net.getUserCount();
    if (n == 0) return 0;

    net.assignIndices();
    componentLabel.resize(n);
    componentLabel.fill(-1);                    // -1 = not labelled yet

    int components = 0;
    for (const UserNode* node = net.getHead(); node != nullptr; node = node->next) {
        if (componentLabel.getOr(node->tempIndex, 0) == -1) {
            labelComponent(node, componentLabel, components);
            components++;
        }
    }
    return components;
}

/* ============================================================
 *                     Mutual friends
 * ============================================================ */

/*
 * Purpose : users who are friends with both A and B.
 * Method  : mark A's friends in an array, then scan B's friends once.
 * Complexity: O(V + dA + dB)
 */
Status mutualFriends(const SocialNetwork& net, int idA, int idB, SafeIntArray& resultIds) {
    resultIds.clear();
    if (net.getUserCount() == 0) return ERR_NETWORK_EMPTY;
    if (idA == idB) return ERR_INVALID_INPUT;
    const UserNode* a = net.findUserById(idA);
    const UserNode* b = net.findUserById(idB);
    if (a == nullptr || b == nullptr) return ERR_NOT_FOUND;

    net.assignIndices();
    SafeIntArray isFriendOfA(net.getUserCount());
    for (const FriendNode* f = a->friendHead; f != nullptr; f = f->next) {
        isFriendOfA.set(f->friendPtr->tempIndex, 1);
    }
    for (const FriendNode* f = b->friendHead; f != nullptr; f = f->next) {
        if (isFriendOfA.getOr(f->friendPtr->tempIndex, 0) == 1) resultIds.append(f->friendId);
    }
    return OK;
}

/* ============================================================
 *                   Friend recommendations
 * ============================================================ */

/*
 * Purpose : suggest new friends = friends-of-friends who are not the user
 *           and not already friends, ranked by mutual friend count.
 * Method  : 1) mark the user and its friends as "excluded"
 *           2) for every friend F, for every friend G of F: counts[G]++
 *              (counts[G] = number of mutual friends with the user)
 *           3) insert candidates one by one into a sorted array of at most topK
 * Complexity: O(sum of friends' degrees + V * topK)
 */
Status recommendFriends(const SocialNetwork& net, int userId, int topK,
                        RecommendEntry out[], int& outCount) {
    outCount = 0;
    if (topK < 1) return ERR_INVALID_INPUT;
    if (net.getUserCount() == 0) return ERR_NETWORK_EMPTY;
    const UserNode* user = net.findUserById(userId);
    if (user == nullptr) return ERR_NOT_FOUND;

    NodeTable table(net);
    int n = table.getSize();
    SafeIntArray excluded(n);
    SafeIntArray counts(n);

    excluded.set(user->tempIndex, 1);
    for (const FriendNode* f = user->friendHead; f != nullptr; f = f->next) {
        excluded.set(f->friendPtr->tempIndex, 1);
    }
    for (const FriendNode* f = user->friendHead; f != nullptr; f = f->next) {
        for (const FriendNode* g = f->friendPtr->friendHead; g != nullptr; g = g->next) {
            int candidate = g->friendPtr->tempIndex;
            if (excluded.getOr(candidate, 1) == 0) {
                counts.set(candidate, counts.getOr(candidate, 0) + 1);
            }
        }
    }

    for (int i = 0; i < n; i++) {
        int mutual = counts.getOr(i, 0);
        const UserNode* node = table.at(i);
        if (mutual <= 0 || node == nullptr) continue;

        RecommendEntry entry;
        entry.id = node->id;
        entry.name = node->name;
        entry.mutualCount = mutual;

        // Find where 'entry' belongs in the sorted prefix out[0..outCount-1].
        int pos = outCount;
        while (pos > 0 && recommendComesBefore(entry, out[pos - 1])) pos--;
        if (pos >= topK) continue;              // list is full and entry is worse than everything kept

        int lastSlot = (outCount < topK) ? outCount : topK - 1;   // when full, the last item drops off
        for (int j = lastSlot; j > pos; j--) out[j] = out[j - 1];
        out[pos] = entry;
        if (outCount < topK) outCount++;
    }
    return OK;
}

/* ============================================================
 *                      Ranking (sorting)
 * ============================================================ */

/*
 * Purpose : list every user ordered by friend count (most first), ties by ID.
 * Method  : copy all users into 'out', then INSERTION SORT.
 *           It is stable and O(V) on already sorted data.
 * Complexity: O(V^2) worst case
 */
Status rankUsersByFriends(const SocialNetwork& net, RankEntry out[], int& outCount) {
    outCount = 0;
    if (net.getUserCount() == 0) return ERR_NETWORK_EMPTY;

    for (const UserNode* u = net.getHead(); u != nullptr; u = u->next) {
        out[outCount].id = u->id;
        out[outCount].name = u->name;
        out[outCount].friendCount = u->friendCount;
        outCount++;
    }
    for (int i = 1; i < outCount; i++) {
        RankEntry key = out[i];
        int j = i - 1;
        while (j >= 0 && rankComesBefore(key, out[j])) {
            out[j + 1] = out[j];
            j--;
        }
        out[j + 1] = key;
    }
    return OK;
}
