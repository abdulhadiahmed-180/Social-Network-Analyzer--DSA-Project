/* test_graph.cpp - SocialNetwork (Module A): users, friendships, removal, copying */
#include "TestHelper.h"

static void testAddAndFind() {
    SocialNetwork net;
    CHECK(net.getUserCount() == 0 && net.getEdgeCount() == 0, "new network is empty");
    CHECK(net.getHead() == nullptr, "empty network has no head");
    CHECK(net.addUser(1, "Ali Khan", 21, "Karachi", "Student", "+923001234567") == OK, "add first user");
    CHECK(net.addUser(2, "Sara", 30, "Lahore", "Engineer", "03001234567") == OK, "add second user");
    CHECK(net.getUserCount() == 2, "user count is 2");
    CHECK(net.findUserById(1) != nullptr && net.findUserById(1)->name == "Ali Khan", "find user 1");
    CHECK(net.findUserById(99) == nullptr, "find missing user gives nullptr");
    CHECK(net.getHead()->id == 1 && net.getHead()->next->id == 2, "users kept in insertion order");

    CHECK(net.addUser(1, "Dup", 25, "X", "Y", "1234567") == ERR_DUPLICATE_ID, "duplicate ID rejected");
    CHECK(net.getUserCount() == 2, "count unchanged after duplicate");
    CHECK(net.findUserById(1)->name == "Ali Khan", "original user not overwritten by duplicate");

    CHECK(net.addUser(0, "A", 20, "B", "C", "1234567") == ERR_INVALID_INPUT, "ID 0 rejected");
    CHECK(net.addUser(-4, "A", 20, "B", "C", "1234567") == ERR_INVALID_INPUT, "negative ID rejected");
    CHECK(net.addUser(5, "", 20, "B", "C", "1234567") == ERR_INVALID_INPUT, "empty name rejected");
    CHECK(net.addUser(5, "A|B", 20, "B", "C", "1234567") == ERR_INVALID_INPUT, "pipe in name rejected");
    CHECK(net.addUser(5, "A", 12, "B", "C", "1234567") == ERR_INVALID_INPUT, "age 12 rejected");
    CHECK(net.addUser(5, "A", 121, "B", "C", "1234567") == ERR_INVALID_INPUT, "age 121 rejected");
    CHECK(net.addUser(5, "A", 20, "B", "C", "12ab567") == ERR_INVALID_INPUT, "bad contact rejected");
    CHECK(net.addUser(5, std::string(51, 'x'), 20, "B", "C", "1234567") == ERR_INVALID_INPUT, "overlong name rejected");
    CHECK(net.getUserCount() == 2, "count unchanged after invalid adds");

    CHECK(net.addUser(7, "  Padded  ", 20, "  City ", " Job ", " 1234567 ") == OK, "fields with outer spaces accepted");
    CHECK(net.findUserById(7)->name == "Padded" && net.findUserById(7)->contact == "1234567", "stored trimmed");
    CHECK(net.addUser(8, "Same Contact", 20, "B", "C", "1234567") == OK, "duplicate contact numbers are allowed");
}

static void testCapacity() {
    SocialNetwork net;
    bool allOk = true;
    for (int i = 1; i <= MAX_USERS; i++) if (addTestUser(net, i) != OK) allOk = false;
    CHECK(allOk && net.getUserCount() == MAX_USERS, "can add MAX_USERS users");
    CHECK(addTestUser(net, MAX_USERS + 1) == ERR_CAPACITY, "user MAX_USERS+1 rejected with ERR_CAPACITY");
    CHECK(addTestUser(net, 5) == ERR_DUPLICATE_ID, "duplicate still reported when full");
    CHECK(net.removeUser(500) == OK && addTestUser(net, MAX_USERS + 1) == OK, "space is available again after a removal");
}

static void testSearch() {
    SocialNetwork net;
    net.addUser(1, "Ali Khan", 21, "Karachi", "Software Engineer", "1234567");
    net.addUser(2, "Alina Shah", 25, "Lahore", "Doctor", "1234568");
    net.addUser(3, "Omar", 40, "karachi", "Teacher", "1234569");
    SafeIntArray r;
    CHECK(net.searchUsers(BY_NAME, "ali", r) == 2, "name partial match, any case");
    const int expectedAli[] = {1, 2};
    CHECK(arrayEquals(r, expectedAli, 2), "matches returned in list order");
    CHECK(net.searchUsers(BY_CITY, "KARACHI", r) == 2, "city match is case-insensitive");
    CHECK(net.searchUsers(BY_PROFESSION, "engin", r) == 1 && r.getOr(0, 0) == 1, "profession partial match");
    CHECK(net.searchUsers(BY_NAME, "zzz", r) == 0 && r.getSize() == 0, "no match gives 0 and empty result");
    CHECK(net.searchUsers(BY_NAME, "   ", r) == 0, "blank keyword matches nothing");
    SocialNetwork empty;
    CHECK(empty.searchUsers(BY_NAME, "a", r) == 0, "search on empty network");
}

static void testUpdate() {
    SocialNetwork net;
    net.addUser(1, "Ali", 21, "Karachi", "Student", "1234567");
    CHECK(net.updateUser(1, "Ali Raza", 22, "Lahore", "Teacher", "+9230011223344") == OK, "update succeeds");
    const UserNode* u = net.findUserById(1);
    CHECK(u->name == "Ali Raza" && u->age == 22 && u->city == "Lahore" &&
          u->profession == "Teacher" && u->contact == "+9230011223344", "all fields changed");
    CHECK(u->id == 1, "ID unchanged");
    CHECK(net.updateUser(1, "", 22, "L", "T", "1234567") == ERR_INVALID_INPUT, "invalid update rejected");
    CHECK(net.findUserById(1)->name == "Ali Raza", "user untouched after invalid update");
    CHECK(net.updateUser(9, "X", 22, "L", "T", "1234567") == ERR_NOT_FOUND, "update missing user");
}

static void testFriendships() {
    SocialNetwork net;
    addTestUsers(net, 4);
    CHECK(net.addFriendship(1, 2) == OK, "add 1-2");
    CHECK(net.getEdgeCount() == 1, "edge count 1 (edge counted once)");
    CHECK(net.areFriends(1, 2) && net.areFriends(2, 1), "friendship is symmetric");
    CHECK(net.getFriendCount(1) == 1 && net.getFriendCount(2) == 1, "both friend counts 1");
    CHECK(net.addFriendship(1, 1) == ERR_SELF_FRIEND, "self friendship rejected");
    CHECK(net.addFriendship(1, 99) == ERR_NOT_FOUND, "missing second user");
    CHECK(net.addFriendship(99, 1) == ERR_NOT_FOUND, "missing first user");
    CHECK(net.addFriendship(1, 2) == ERR_DUPLICATE_EDGE, "duplicate edge rejected");
    CHECK(net.addFriendship(2, 1) == ERR_DUPLICATE_EDGE, "reversed edge rejected");
    CHECK(net.getEdgeCount() == 1, "edge count unchanged by rejected adds");
    CHECK(!net.areFriends(1, 3), "non-friends");
    CHECK(!net.areFriends(1, 99) && !net.areFriends(99, 98), "areFriends false for missing users");
    CHECK(net.getFriendCount(99) == -1, "friend count of missing user is -1");

    // friend list keeps insertion order and direct pointers
    net.addFriendship(1, 3);
    net.addFriendship(1, 4);
    const UserNode* u1 = net.findUserById(1);
    CHECK(u1->friendHead->friendId == 2 && u1->friendHead->next->friendId == 3 &&
          u1->friendHead->next->next->friendId == 4, "friend list in insertion order");
    CHECK(u1->friendHead->friendPtr == net.findUserById(2), "friendPtr points at the real node");

    CHECK(net.removeFriendship(1, 3) == OK, "remove 1-3");
    CHECK(!net.areFriends(1, 3) && !net.areFriends(3, 1), "removed from both directions");
    CHECK(net.getEdgeCount() == 2 && net.getFriendCount(1) == 2 && net.getFriendCount(3) == 0, "counts updated");
    CHECK(net.removeFriendship(1, 3) == ERR_NOT_FOUND, "removing a non-existent edge");
    CHECK(net.removeFriendship(1, 99) == ERR_NOT_FOUND, "removing edge with missing user");
    CHECK(net.removeFriendship(2, 2) == ERR_SELF_FRIEND, "removing self edge");
    CHECK(net.addFriendship(1, 3) == OK, "can re-add after removal");
    CHECK(net.removeFriendship(4, 1) == OK && !net.areFriends(1, 4), "remove given in reversed order");
}

static void testRemoveUser() {
    SocialNetwork net;
    addTestUsers(net, 12);
    for (int i = 2; i <= 11; i++) net.addFriendship(1, i);   // hub with 10 friends
    net.addFriendship(2, 3);
    CHECK(net.getEdgeCount() == 11, "11 edges before removal");

    CHECK(net.removeUser(1) == OK, "remove the hub (head of list)");
    CHECK(net.getUserCount() == 11, "user count 11");
    CHECK(net.getEdgeCount() == 1, "only edge 2-3 remains");
    CHECK(net.findUserById(1) == nullptr, "hub is gone");
    bool neighboursClean = true;
    for (int i = 4; i <= 11; i++) if (net.getFriendCount(i) != 0) neighboursClean = false;
    CHECK(neighboursClean, "no neighbour still lists the removed user");
    CHECK(net.getFriendCount(2) == 1 && net.areFriends(2, 3), "unrelated edge 2-3 intact");
    CHECK(net.getHead()->id == 2, "head moved to next user");

    CHECK(net.removeUser(12) == OK, "remove the tail");
    CHECK(addTestUser(net, 50) == OK, "adding after removing the tail works (tail pointer fixed)");
    const UserNode* last = net.getHead();
    while (last->next != nullptr) last = last->next;
    CHECK(last->id == 50, "new user is last in the list");

    CHECK(net.removeUser(5) == OK, "remove a middle user");
    CHECK(net.removeUser(5) == ERR_NOT_FOUND, "removing again fails");
    CHECK(net.removeUser(777) == ERR_NOT_FOUND, "removing a non-existent user");

    // remove everything one by one: no crash, ends empty and reusable
    SocialNetwork small;
    addTestUsers(small, 3);
    small.addFriendship(1, 2);
    CHECK(small.removeUser(1) == OK && small.removeUser(2) == OK && small.removeUser(3) == OK, "remove all users");
    CHECK(small.getUserCount() == 0 && small.getEdgeCount() == 0 && small.getHead() == nullptr, "network empty again");
    CHECK(addTestUser(small, 1) == OK && small.getHead()->id == 1, "network reusable after being emptied");
}

static void testCopyAndClear() {
    SocialNetwork a;
    addTestUsers(a, 5);
    a.addFriendship(1, 2); a.addFriendship(2, 3); a.addFriendship(3, 1); a.addFriendship(4, 5);

    SocialNetwork b(a);                              // copy constructor
    CHECK(b.getUserCount() == 5 && b.getEdgeCount() == 4, "copy has same counts");
    CHECK(b.areFriends(1, 2) && b.areFriends(2, 3) && b.areFriends(3, 1) && b.areFriends(4, 5), "copy has same edges");
    CHECK(b.findUserById(3) != a.findUserById(3), "copy has its own nodes (deep copy)");
    CHECK(b.findUserById(1)->friendHead->friendPtr == b.findUserById(2) ||
          b.findUserById(1)->friendHead->friendPtr == b.findUserById(3), "copy's friendPtr points inside the copy");
    a.removeUser(1);
    CHECK(b.getUserCount() == 5 && b.areFriends(1, 2), "changing the original does not touch the copy");
    b.removeFriendship(4, 5);
    CHECK(a.areFriends(4, 5), "changing the copy does not touch the original");

    SocialNetwork c;
    addTestUser(c, 100);
    c = a;                                           // copy assignment replaces old content
    CHECK(c.findUserById(100) == nullptr && c.getUserCount() == 4, "assignment replaces previous content");
    c = c;
    CHECK(c.getUserCount() == 4, "self assignment is safe");

    c.clear();
    CHECK(c.getUserCount() == 0 && c.getEdgeCount() == 0 && c.getHead() == nullptr, "clear empties the network");
    c.clear();
    CHECK(c.getUserCount() == 0, "clear on an empty network is safe");
}

static void testIndicesAndDisplay() {
    SocialNetwork net;
    addTestUsers(net, 4);
    net.removeUser(2);
    net.assignIndices();
    const UserNode* u = net.getHead();
    int expectedIndex = 0;
    bool ok = true;
    for (; u != nullptr; u = u->next) { if (u->tempIndex != expectedIndex++) ok = false; }
    CHECK(ok && expectedIndex == 3, "assignIndices numbers users 0..V-1 in list order");

    SocialNetwork empty;
    empty.assignIndices();                           // must not crash
    CHECK(empty.displayFriends(1) == ERR_NOT_FOUND, "displayFriends on missing user");
    CHECK(net.displayFriends(1) == OK, "displayFriends on user without friends");
}

int main() {
    testAddAndFind();
    testCapacity();
    testSearch();
    testUpdate();
    testFriendships();
    testRemoveUser();
    testCopyAndClear();
    testIndicesAndDisplay();
    return TEST_SUMMARY("test_graph");
}
