/*
 * Graph.cpp  -  MODULE A (Member 1): SocialNetwork implementation
 *
 * V = number of users, E = number of friendships, d = a user's friend count.
 */
#include "Graph.h"
#include <iostream>

/* ============================================================
 *              Construction / destruction / copying
 * ============================================================ */

SocialNetwork::SocialNetwork() : head(nullptr), tail(nullptr), userCount(0), edgeCount(0) {}

/* Destructor: frees every user and every friend list.  O(V + E). */
SocialNetwork::~SocialNetwork() { clear(); }

/* Copy constructor: deep copy.  Starts empty, then copyFrom() does the work. */
SocialNetwork::SocialNetwork(const SocialNetwork& other)
    : head(nullptr), tail(nullptr), userCount(0), edgeCount(0) {
    copyFrom(other);
}

/* Copy assignment: free our data first, then deep copy.  Self-assignment safe. */
SocialNetwork& SocialNetwork::operator=(const SocialNetwork& other) {
    if (this != &other) {
        clear();
        copyFrom(other);
    }
    return *this;
}

/*
 * Purpose : deep-copy 'other' into this (empty) network.
 * Order   : users first (pass 1), then edges (pass 2), so that every edge
 *           endpoint already exists.  Each edge is copied once (smaller ID first).
 * Note    : the order inside friend lists may differ from the original.
 * Complexity: O(V^2 + E*V) because it reuses the validated public methods.
 */
void SocialNetwork::copyFrom(const SocialNetwork& other) {
    for (const UserNode* u = other.head; u != nullptr; u = u->next) {
        addUser(u->id, u->name, u->age, u->city, u->profession, u->contact);
    }
    for (const UserNode* u = other.head; u != nullptr; u = u->next) {
        for (const FriendNode* f = u->friendHead; f != nullptr; f = f->next) {
            if (u->id < f->friendId) addFriendship(u->id, f->friendId);
        }
    }
}

/* Frees one user's friend list (does NOT touch the neighbours). */
void SocialNetwork::freeFriendList(UserNode* user) {
    FriendNode* f = user->friendHead;
    while (f != nullptr) {
        FriendNode* nextNode = f->next;
        delete f;
        f = nextNode;
    }
    user->friendHead = nullptr;
    user->friendCount = 0;
}

/* Purpose: delete every user -> empty network.  Complexity: O(V + E). */
void SocialNetwork::clear() {
    UserNode* u = head;
    while (u != nullptr) {
        UserNode* nextUser = u->next;
        freeFriendList(u);
        delete u;
        u = nextUser;
    }
    head = tail = nullptr;
    userCount = edgeCount = 0;
}

/* ============================================================
 *                           Users
 * ============================================================ */

/*
 * Purpose : add a new user at the END of the master list.
 * Inputs  : all user fields (validated by validateUserFields; text is stored trimmed)
 * Outputs : OK | ERR_INVALID_INPUT | ERR_DUPLICATE_ID | ERR_CAPACITY
 * Complexity: O(V) for the duplicate check, then O(1) append via the tail pointer.
 */
Status SocialNetwork::addUser(int id, const std::string& name, int age, const std::string& city,
                              const std::string& profession, const std::string& contact) {
    Status check = validateUserFields(id, name, age, city, profession, contact);
    if (check != OK) return check;
    if (findUserById(id) != nullptr) return ERR_DUPLICATE_ID;
    if (userCount >= MAX_USERS) return ERR_CAPACITY;

    UserNode* node = new UserNode();
    node->id = id;
    node->name = trim(name);
    node->age = age;
    node->city = trim(city);
    node->profession = trim(profession);
    node->contact = trim(contact);

    if (tail == nullptr) head = tail = node;   // first user
    else { tail->next = node; tail = node; }
    userCount++;
    return OK;
}

/* Purpose: find a user by ID.  Returns nullptr if absent.  Complexity: O(V). */
UserNode* SocialNetwork::findUserById(int id) const {
    for (UserNode* u = head; u != nullptr; u = u->next) {
        if (u->id == id) return u;
    }
    return nullptr;
}

/*
 * Purpose : case-insensitive PARTIAL match on name, city or profession.
 * Inputs  : field to search, keyword (trimmed; an empty keyword matches nothing)
 * Outputs : resultIds is cleared and filled with matching IDs in list order;
 *           the return value is the number of matches.
 * Complexity: O(V * field length * keyword length)
 */
int SocialNetwork::searchUsers(SearchField field, const std::string& keyword,
                               SafeIntArray& resultIds) const {
    resultIds.clear();
    std::string key = trim(keyword);
    if (key.empty()) return 0;

    for (const UserNode* u = head; u != nullptr; u = u->next) {
        const std::string& target = (field == BY_NAME) ? u->name
                                  : (field == BY_CITY) ? u->city
                                                       : u->profession;
        if (containsIgnoreCase(target, key)) resultIds.append(u->id);
    }
    return resultIds.getSize();
}

/*
 * Purpose : replace a user's details (the ID can never change).
 * Outputs : OK | ERR_INVALID_INPUT | ERR_NOT_FOUND  (the user is untouched on error)
 * Complexity: O(V)
 */
Status SocialNetwork::updateUser(int id, const std::string& name, int age, const std::string& city,
                                 const std::string& profession, const std::string& contact) {
    Status check = validateUserFields(id, name, age, city, profession, contact);
    if (check != OK) return check;
    UserNode* user = findUserById(id);
    if (user == nullptr) return ERR_NOT_FOUND;

    user->name = trim(name);
    user->age = age;
    user->city = trim(city);
    user->profession = trim(profession);
    user->contact = trim(contact);
    return OK;
}

/*
 * Purpose : delete a user and every friendship it had.
 * Steps   : 1) find user (and its predecessor in the master list)
 *           2) remove the user from every neighbour's friend list
 *           3) free its own friend list, unlink it, fix head/tail, delete
 * Outputs : OK | ERR_NOT_FOUND
 * Complexity: O(V + sum of neighbour degrees)
 */
Status SocialNetwork::removeUser(int id) {
    UserNode* prev = nullptr;
    UserNode* cur = head;
    while (cur != nullptr && cur->id != id) {
        prev = cur;
        cur = cur->next;
    }
    if (cur == nullptr) return ERR_NOT_FOUND;

    // Step 2: detach from all neighbours (each friend node = one edge)
    for (FriendNode* f = cur->friendHead; f != nullptr; f = f->next) {
        if (f->friendPtr != nullptr) removeFromFriendList(f->friendPtr, id);
        edgeCount--;
    }
    // Step 3: free own list and unlink
    freeFriendList(cur);
    if (prev != nullptr) prev->next = cur->next;
    else head = cur->next;
    if (cur == tail) tail = prev;     // removed the last node (prev is nullptr if the list is now empty)

    delete cur;
    userCount--;
    return OK;
}

/* ============================================================
 *                        Friendships
 * ============================================================ */

/* Adds 'other' at the END of owner's friend list (keeps insertion order). O(d). */
void SocialNetwork::appendFriend(UserNode* owner, UserNode* other) {
    FriendNode* node = new FriendNode();
    node->friendId = other->id;
    node->friendPtr = other;

    if (owner->friendHead == nullptr) {
        owner->friendHead = node;
    } else {
        FriendNode* last = owner->friendHead;
        while (last->next != nullptr) last = last->next;
        last->next = node;
    }
    owner->friendCount++;
}

/* Removes the entry for 'friendId' from owner's list. Returns false if absent. O(d). */
bool SocialNetwork::removeFromFriendList(UserNode* owner, int friendId) {
    FriendNode* prev = nullptr;
    FriendNode* cur = owner->friendHead;
    while (cur != nullptr && cur->friendId != friendId) {
        prev = cur;
        cur = cur->next;
    }
    if (cur == nullptr) return false;

    if (prev != nullptr) prev->next = cur->next;
    else owner->friendHead = cur->next;
    delete cur;
    owner->friendCount--;
    return true;
}

/*
 * Purpose : create an undirected friendship (stored in both lists).
 * Outputs : OK | ERR_SELF_FRIEND | ERR_NOT_FOUND | ERR_DUPLICATE_EDGE
 * Complexity: O(V + dA + dB)
 */
Status SocialNetwork::addFriendship(int idA, int idB) {
    if (idA == idB) return ERR_SELF_FRIEND;
    UserNode* a = findUserById(idA);
    UserNode* b = findUserById(idB);
    if (a == nullptr || b == nullptr) return ERR_NOT_FOUND;

    for (const FriendNode* f = a->friendHead; f != nullptr; f = f->next) {
        if (f->friendId == idB) return ERR_DUPLICATE_EDGE;   // also catches the "reversed" edge B-A
    }
    appendFriend(a, b);
    appendFriend(b, a);
    edgeCount++;
    return OK;
}

/*
 * Purpose : delete a friendship from both lists.
 * Outputs : OK | ERR_SELF_FRIEND (same ID twice) | ERR_NOT_FOUND (a user or the edge is missing)
 * Complexity: O(V + dA + dB)
 */
Status SocialNetwork::removeFriendship(int idA, int idB) {
    if (idA == idB) return ERR_SELF_FRIEND;
    UserNode* a = findUserById(idA);
    UserNode* b = findUserById(idB);
    if (a == nullptr || b == nullptr) return ERR_NOT_FOUND;

    if (!removeFromFriendList(a, idB)) return ERR_NOT_FOUND;   // no such edge
    removeFromFriendList(b, idA);
    edgeCount--;
    return OK;
}

/* Purpose: are the two users friends?  False if either does not exist.  O(V + dA). */
bool SocialNetwork::areFriends(int idA, int idB) const {
    const UserNode* a = findUserById(idA);
    if (a == nullptr || findUserById(idB) == nullptr) return false;
    for (const FriendNode* f = a->friendHead; f != nullptr; f = f->next) {
        if (f->friendId == idB) return true;
    }
    return false;
}

/* ============================================================
 *                          Queries
 * ============================================================ */

int SocialNetwork::getUserCount() const { return userCount; }
int SocialNetwork::getEdgeCount() const { return edgeCount; }

/* Friend count of a user, or -1 if the user does not exist.  O(V). */
int SocialNetwork::getFriendCount(int id) const {
    const UserNode* u = findUserById(id);
    return (u == nullptr) ? -1 : u->friendCount;
}

const UserNode* SocialNetwork::getHead() const { return head; }

/* Purpose: number the users 0..V-1 in master-list order.  Complexity: O(V). */
void SocialNetwork::assignIndices() const {
    int index = 0;
    for (const UserNode* u = head; u != nullptr; u = u->next) {
        u->tempIndex = index++;
    }
}

/* ============================================================
 *                          Display
 * ============================================================ */

/* Prints one user on a single line. */
void SocialNetwork::displayUser(int id) const {
    const UserNode* u = findUserById(id);
    if (u == nullptr) {
        std::cout << "User with ID " << id << " not found.\n";
        return;
    }
    std::cout << "ID: " << u->id << " | Name: " << u->name << " | Age: " << u->age
              << " | City: " << u->city << " | Profession: " << u->profession
              << " | Contact: " << u->contact << " | Friends: " << u->friendCount << "\n";
}

/* Prints a table of all users. */
void SocialNetwork::displayAllUsers() const {
    if (head == nullptr) {
        std::cout << "The network is empty.\n";
        return;
    }
    std::cout << padRight("ID", 11) << padRight("Name", 22) << padRight("Age", 6)
              << padRight("City", 18) << padRight("Profession", 22)
              << padRight("Contact", 18) << "Friends\n";
    for (const UserNode* u = head; u != nullptr; u = u->next) {
        std::cout << padRight(std::to_string(u->id), 11) << padRight(u->name, 22)
                  << padRight(std::to_string(u->age), 6) << padRight(u->city, 18)
                  << padRight(u->profession, 22) << padRight(u->contact, 18)
                  << u->friendCount << "\n";
    }
    std::cout << userCount << " user(s), " << edgeCount << " friendship(s).\n";
}

/* Prints the friend list of one user.  Outputs: OK | ERR_NOT_FOUND. */
Status SocialNetwork::displayFriends(int id) const {
    const UserNode* u = findUserById(id);
    if (u == nullptr) return ERR_NOT_FOUND;

    std::cout << "Friends of " << u->name << " (ID " << u->id << "): " << u->friendCount << "\n";
    if (u->friendHead == nullptr) {
        std::cout << "  (no friends yet)\n";
        return OK;
    }
    for (const FriendNode* f = u->friendHead; f != nullptr; f = f->next) {
        std::cout << "  - " << f->friendPtr->name << " (ID " << f->friendId << ")\n";
    }
    return OK;
}
