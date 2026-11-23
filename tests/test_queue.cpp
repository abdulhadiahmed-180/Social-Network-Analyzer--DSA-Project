/* test_queue.cpp - IntQueue: normal use, wrap-around, full, empty, copying */
#include "TestHelper.h"
#include "Queue.h"

static void testBasicFifo() {
    IntQueue q(5);
    int v = -1;
    CHECK(q.isEmpty(), "new queue is empty");
    CHECK(!q.isFull(), "new queue is not full");
    CHECK(q.size() == 0, "new queue size 0");
    CHECK(!q.dequeue(v), "dequeue on empty fails");
    CHECK(!q.peek(v), "peek on empty fails");

    CHECK(q.enqueue(10) && q.enqueue(20) && q.enqueue(30), "enqueue 3 items");
    CHECK(q.size() == 3, "size is 3");
    CHECK(q.peek(v) && v == 10, "peek shows oldest (10)");
    CHECK(q.size() == 3, "peek does not remove");
    CHECK(q.dequeue(v) && v == 10, "FIFO: first out is 10");
    CHECK(q.dequeue(v) && v == 20, "FIFO: second out is 20");
    CHECK(q.dequeue(v) && v == 30, "FIFO: third out is 30");
    CHECK(q.isEmpty(), "empty again");
}

static void testFull() {
    IntQueue q(3);
    CHECK(q.enqueue(1) && q.enqueue(2) && q.enqueue(3), "fill to capacity");
    CHECK(q.isFull(), "queue is full");
    CHECK(!q.enqueue(4), "enqueue on full fails");
    CHECK(q.size() == 3, "size unchanged after failed enqueue");
    int v;
    CHECK(q.dequeue(v) && v == 1, "oldest item survived the failed enqueue");
}

static void testWrapAround() {
    IntQueue q(3);
    int v = 0;
    q.enqueue(1); q.enqueue(2); q.enqueue(3);
    CHECK(q.dequeue(v) && v == 1, "dequeue 1");
    CHECK(q.enqueue(4), "enqueue 4 reuses the freed slot (rear wraps to index 0)");
    CHECK(q.isFull(), "full again after wrap");
    CHECK(q.dequeue(v) && v == 2, "order kept after wrap: 2");
    CHECK(q.dequeue(v) && v == 3, "order kept after wrap: 3");
    CHECK(q.dequeue(v) && v == 4, "order kept after wrap: 4");
    CHECK(q.isEmpty(), "empty after draining");

    // many cycles around a tiny queue
    bool ok = true;
    for (int i = 0; i < 100; i++) {
        if (!q.enqueue(i)) ok = false;
        if (!q.dequeue(v) || v != i) ok = false;
    }
    CHECK(ok, "100 enqueue/dequeue cycles through a 3-slot queue");
}

static void testClearAndCapacity() {
    IntQueue q(4);
    q.enqueue(1); q.enqueue(2);
    q.clear();
    CHECK(q.isEmpty() && q.size() == 0, "clear empties the queue");
    CHECK(q.enqueue(9) && q.enqueue(8) && q.enqueue(7) && q.enqueue(6), "full capacity usable after clear");
    CHECK(!q.enqueue(5), "still bounded by capacity");

    IntQueue tiny(0);       // invalid capacity is treated as 1
    CHECK(tiny.enqueue(1) && !tiny.enqueue(2), "capacity 0 behaves as capacity 1");
    IntQueue negative(-5);
    CHECK(negative.enqueue(1), "negative capacity behaves as capacity 1");
}

static void testRuleOfThree() {
    IntQueue a(4);
    a.enqueue(1); a.enqueue(2); a.enqueue(3);
    int v;
    a.dequeue(v);                     // front is now not at index 0

    IntQueue b(a);                    // copy constructor
    CHECK(b.size() == 2, "copy has same size");
    a.enqueue(99);
    CHECK(b.size() == 2, "copy is independent of the original");
    CHECK(b.dequeue(v) && v == 2, "copy keeps order (2)");
    CHECK(b.dequeue(v) && v == 3, "copy keeps order (3)");

    IntQueue c(2);
    c.enqueue(5);
    c = a;                            // copy assignment (different capacity)
    CHECK(c.size() == 3, "assignment copies size");
    CHECK(c.dequeue(v) && v == 2, "assignment copies content");
    c = c;                            // self-assignment must be harmless
    CHECK(c.size() == 2, "self-assignment is safe");
}

int main() {
    testBasicFifo();
    testFull();
    testWrapAround();
    testClearAndCapacity();
    testRuleOfThree();
    return TEST_SUMMARY("test_queue");
}
