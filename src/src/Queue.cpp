/*
 * Queue.cpp  -  MODULE B (Member 2): IntQueue implementation
 * All operations are O(1) except construction/copy/clear-free (O(capacity)).
 */
#include "Queue.h"

IntQueue::IntQueue(int cap) : data(nullptr), capacity(cap < 1 ? 1 : cap), front(0), rear(0), count(0) {
    data = new int[capacity];
    for (int i = 0; i < capacity; i++) data[i] = 0;
}

IntQueue::IntQueue(const IntQueue& other)
    : data(nullptr), capacity(other.capacity), front(0), rear(0), count(0) {
    copyFrom(other);
}

IntQueue& IntQueue::operator=(const IntQueue& other) {
    if (this != &other) {
        delete[] data;
        data = nullptr;
        capacity = other.capacity;
        copyFrom(other);
    }
    return *this;
}

IntQueue::~IntQueue() { delete[] data; }

/* Allocates 'capacity' slots and copies the raw array plus the indices. */
void IntQueue::copyFrom(const IntQueue& other) {
    data = new int[capacity];
    for (int i = 0; i < capacity; i++) data[i] = other.data[i];
    front = other.front;
    rear = other.rear;
    count = other.count;
}

/* Purpose: add v at the rear.  Returns false if the queue is full. */
bool IntQueue::enqueue(int v) {
    if (isFull()) return false;
    data[rear] = v;
    rear = (rear + 1) % capacity;      // wrap-around
    count++;
    return true;
}

/* Purpose: remove the front item into v.  Returns false if the queue is empty. */
bool IntQueue::dequeue(int& v) {
    if (isEmpty()) return false;
    v = data[front];
    front = (front + 1) % capacity;    // wrap-around
    count--;
    return true;
}

/* Purpose: read the front item without removing it. */
bool IntQueue::peek(int& v) const {
    if (isEmpty()) return false;
    v = data[front];
    return true;
}

bool IntQueue::isEmpty() const { return count == 0; }
bool IntQueue::isFull() const { return count == capacity; }
int  IntQueue::size() const { return count; }

void IntQueue::clear() {
    front = rear = count = 0;
}
