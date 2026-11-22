/*
 * Queue.h  -  MODULE B (Member 2): circular integer queue
 * ---------------------------------------------------------------
 * Fixed-capacity circular array queue (FIFO) with Rule of Three.
 * BFS sizes it to userCount: every node is enqueued at most once,
 * so it can never overflow during a traversal.
 *
 * Why a circular array?  A plain array queue wastes the slots freed by
 * dequeue (or shifts everything, O(n)).  Wrapping with modulo reuses
 * freed slots, so enqueue and dequeue are both O(1) and memory is fixed.
 * (A linked queue would also be O(1) but allocates one node per item.)
 */
#ifndef QUEUE_H
#define QUEUE_H

class IntQueue {
public:
    explicit IntQueue(int capacity);                 // capacity < 1 is treated as 1
    IntQueue(const IntQueue& other);                 // Rule of Three: deep copy
    IntQueue& operator=(const IntQueue& other);      // Rule of Three: deep copy
    ~IntQueue();                                     // Rule of Three: frees the array

    bool enqueue(int v);          // false if full
    bool dequeue(int& v);         // false if empty
    bool peek(int& v) const;      // false if empty (does not remove)
    bool isEmpty() const;
    bool isFull() const;
    int  size() const;            // number of items currently stored
    void clear();                 // make it empty again

private:
    int* data;
    int  capacity;
    int  front;     // index of the oldest item
    int  rear;      // index where the NEXT item will be written
    int  count;     // items stored

    void copyFrom(const IntQueue& other);
};

#endif
