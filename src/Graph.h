/*
 * Graph.h  -  MODULE A (Member 1): the social network graph
 * ---------------------------------------------------------------
 * Representation: ADJACENCY LIST.
 *   - All users live in ONE singly linked "master list" (head..tail).
 *   - Each user owns a singly linked list of FriendNodes (its edges).
 *   - Friendship is undirected, so every edge is stored twice
 *     (once in each user's list).  edgeCount counts each edge once.
 *
 * Why this structure?
 *   - Adjacency matrix: O(V^2) memory (1,000,000 cells for 1000 users even
 *     if only a few friendships exist) and O(V) to list one user's friends.
 *   - Array of users: deleting a user means shifting O(V) elements;
 *     a linked list unlinks in O(1) once the node is found.
 *   - Adjacency list: O(V + E) memory and a traversal touches each edge once.
 */
#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include "Utils.h"

struct UserNode;

/* One entry of a user's friend list. */
struct FriendNode {
    int         friendId;
    UserNode*   friendPtr;   // direct pointer: traversals never need an O(V) ID lookup
    FriendNode* next;
    FriendNode() : friendId(0), friendPtr(nullptr), next(nullptr) {}
};

/* One user = one vertex of the graph. */
struct UserNode {
    int          id;            // unique, positive, immutable
    std::string  name, city, profession, contact;
    int          age;
    int          friendCount;
    FriendNode*  friendHead;
    UserNode*    next;
    /* Scratch index 0..V-1 set by assignIndices() before each algorithm.
     * 'mutable' so read-only (const) algorithms may refresh it. */
    mutable int  tempIndex;
    UserNode() : id(0), age(0), friendCount(0), friendHead(nullptr), next(nullptr), tempIndex(-1) {}
};

class SocialNetwork {
public:
    SocialNetwork();
    SocialNetwork(const SocialNetwork& other);             // Rule of Three: deep copy (users first, then edges)
    SocialNetwork& operator=(const SocialNetwork& other);  // Rule of Three: deep copy
    ~SocialNetwork();                                      // Rule of Three: frees every user and friend list

    /* ---- Users ---- */
    Status addUser(int id, const std::string& name, int age, const std::string& city,
                   const std::string& profession, const std::string& contact);
    UserNode* findUserById(int id) const;
    int searchUsers(SearchField field, const std::string& keyword, SafeIntArray& resultIds) const;
    Status updateUser(int id, const std::string& name, int age, const std::string& city,
                      const std::string& profession, const std::string& contact);
    Status removeUser(int id);

    /* ---- Friendships ---- */
    Status addFriendship(int idA, int idB);
    Status removeFriendship(int idA, int idB);
    bool   areFriends(int idA, int idB) const;

    /* ---- Queries ---- */
    int getUserCount() const;
    int getEdgeCount() const;
    int getFriendCount(int id) const;          // -1 if the user does not exist
    const UserNode* getHead() const;           // read-only iteration for Modules B and C

    /* Sets tempIndex = 0..V-1 along the master list.
     * It is const because tempIndex is mutable scratch data (see UserNode). */
    void assignIndices() const;

    /* ---- Display (console) ---- */
    void   displayUser(int id) const;
    void   displayAllUsers() const;
    Status displayFriends(int id) const;

    void clear();                              // remove everything -> empty network

private:
    UserNode* head;
    UserNode* tail;
    int       userCount;
    int       edgeCount;

    void copyFrom(const SocialNetwork& other);
    static void appendFriend(UserNode* owner, UserNode* other);
    static bool removeFromFriendList(UserNode* owner, int friendId);
    static void freeFriendList(UserNode* user);
};

#endif
