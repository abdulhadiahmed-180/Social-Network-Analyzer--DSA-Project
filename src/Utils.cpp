/*
 * Utils.cpp  -  Implementation of SafeIntArray, string helpers, validators
 */
#include "Utils.h"

/* ============================================================
 *                        SafeIntArray
 * ============================================================ */

/* Purpose: create an array of 'initialSize' zeros.  Complexity: O(n). */
SafeIntArray::SafeIntArray(int initialSize) : data(nullptr), size(0), capacity(0) {
    if (initialSize > 0) {
        data = new int[initialSize];
        capacity = initialSize;
        size = initialSize;
        for (int i = 0; i < size; i++) data[i] = 0;
    }
}

/* Copy constructor: deep copy. */
SafeIntArray::SafeIntArray(const SafeIntArray& other) : data(nullptr), size(0), capacity(0) {
    copyFrom(other);
}

/* Copy assignment: deep copy, safe against self-assignment. */
SafeIntArray& SafeIntArray::operator=(const SafeIntArray& other) {
    if (this != &other) {
        delete[] data;
        data = nullptr;
        size = capacity = 0;
        copyFrom(other);
    }
    return *this;
}

SafeIntArray::~SafeIntArray() { delete[] data; }

/* Helper for the Rule of Three: allocate exactly other.size and copy. */
void SafeIntArray::copyFrom(const SafeIntArray& other) {
    if (other.size > 0) {
        data = new int[other.size];
        capacity = other.size;
        size = other.size;
        for (int i = 0; i < size; i++) data[i] = other.data[i];
    }
}

/* Read element 'index' into 'value'. Returns false (value untouched) if out of range. */
bool SafeIntArray::at(int index, int& value) const {
    if (index < 0 || index >= size) return false;
    value = data[index];
    return true;
}

/* Write element 'index'. Returns false if out of range. */
bool SafeIntArray::set(int index, int value) {
    if (index < 0 || index >= size) return false;
    data[index] = value;
    return true;
}

/* Read element, or return 'fallback' when the index is out of range. */
int SafeIntArray::getOr(int index, int fallback) const {
    if (index < 0 || index >= size) return fallback;
    return data[index];
}

void SafeIntArray::fill(int value) {
    for (int i = 0; i < size; i++) data[i] = value;
}

int SafeIntArray::getSize() const { return size; }

/* Make sure the buffer can hold at least 'needed' elements (doubling growth). */
void SafeIntArray::ensureCapacity(int needed) {
    if (needed <= capacity) return;
    int newCapacity = capacity * 2;
    if (newCapacity < needed) newCapacity = needed;
    if (newCapacity < 4) newCapacity = 4;
    int* bigger = new int[newCapacity];
    for (int i = 0; i < size; i++) bigger[i] = data[i];
    delete[] data;
    data = bigger;
    capacity = newCapacity;
}

/* Change the size. Old values are kept, new slots are zero. */
bool SafeIntArray::resize(int newSize) {
    if (newSize < 0) return false;
    ensureCapacity(newSize);
    for (int i = size; i < newSize; i++) data[i] = 0;
    size = newSize;
    return true;
}

/* Add one value at the end. Amortised O(1). */
bool SafeIntArray::append(int value) {
    ensureCapacity(size + 1);
    data[size] = value;
    size++;
    return true;
}

void SafeIntArray::clear() { size = 0; }

/* ============================================================
 *                      String helpers
 * ============================================================ */

/* True for the characters that trim() removes. */
static bool isSpaceChar(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/* Purpose: remove leading/trailing whitespace.  Complexity: O(n). */
std::string trim(const std::string& text) {
    int start = 0;
    int end = (int)text.length() - 1;
    while (start <= end && isSpaceChar(text[start])) start++;
    while (end >= start && isSpaceChar(text[end])) end--;
    return text.substr(start, end - start + 1);
}

char toLowerChar(char c) {
    if (c >= 'A' && c <= 'Z') return (char)(c - 'A' + 'a');
    return c;
}

bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) return false;
    for (int i = 0; i < (int)a.length(); i++) {
        if (toLowerChar(a[i]) != toLowerChar(b[i])) return false;
    }
    return true;
}

/* Purpose: case-insensitive substring test.  Complexity: O(|haystack| * |needle|). */
bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    int h = (int)haystack.length();
    int n = (int)needle.length();
    if (n == 0) return true;
    for (int start = 0; start + n <= h; start++) {
        int k = 0;
        while (k < n && toLowerChar(haystack[start + k]) == toLowerChar(needle[k])) k++;
        if (k == n) return true;
    }
    return false;
}

std::string padRight(const std::string& text, int width) {
    std::string result = text;
    while ((int)result.length() < width) result += ' ';
    return result;
}

/* Purpose: strict integer parse (no letters, no overflow: at most 9 digits). */
bool parseInteger(const std::string& text, int& out) {
    std::string s = trim(text);
    int len = (int)s.length();
    int pos = 0;
    bool negative = false;
    if (len > 0 && s[0] == '-') { negative = true; pos = 1; }
    int digits = len - pos;
    if (digits < 1 || digits > 9) return false;
    int value = 0;
    for (; pos < len; pos++) {
        if (s[pos] < '0' || s[pos] > '9') return false;
        value = value * 10 + (s[pos] - '0');
    }
    out = negative ? -value : value;
    return true;
}

/* ============================================================
 *                         Validators
 * ============================================================ */

bool isValidId(int id) { return id >= 1 && id <= MAX_ID; }

bool isValidAge(int age) { return age >= MIN_AGE && age <= MAX_AGE; }

/*
 * Text field rule: after trimming it must be 1..MAX_FIELD_LEN characters,
 * contain no '|' (the file separator) and no control characters.
 */
bool isValidTextField(const std::string& text) {
    std::string s = trim(text);
    int len = (int)s.length();
    if (len < 1 || len > MAX_FIELD_LEN) return false;
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '|' || c < 32 || c == 127) return false;
    }
    return true;
}

/* Contact rule: optional leading '+', then only digits, 7..15 of them. */
bool isValidContact(const std::string& text) {
    std::string s = trim(text);
    int len = (int)s.length();
    int pos = (len > 0 && s[0] == '+') ? 1 : 0;
    int digits = len - pos;
    if (digits < MIN_CONTACT_DIGITS || digits > MAX_CONTACT_DIGITS) return false;
    for (; pos < len; pos++) {
        if (s[pos] < '0' || s[pos] > '9') return false;
    }
    return true;
}

/* Small helper: store a message only if the caller asked for one. */
static void setReason(std::string* reason, const char* message) {
    if (reason != nullptr) *reason = message;
}

Status validateUserFields(int id, const std::string& name, int age,
                          const std::string& city, const std::string& profession,
                          const std::string& contact, std::string* reason) {
    if (!isValidId(id)) {
        setReason(reason, "ID must be a positive integer (1 to 999999999)");
        return ERR_INVALID_INPUT;
    }
    if (!isValidTextField(name)) {
        setReason(reason, "invalid name (1-50 characters, no '|')");
        return ERR_INVALID_INPUT;
    }
    if (!isValidAge(age)) {
        setReason(reason, "age must be between 13 and 120");
        return ERR_INVALID_INPUT;
    }
    if (!isValidTextField(city)) {
        setReason(reason, "invalid city (1-50 characters, no '|')");
        return ERR_INVALID_INPUT;
    }
    if (!isValidTextField(profession)) {
        setReason(reason, "invalid profession (1-50 characters, no '|')");
        return ERR_INVALID_INPUT;
    }
    if (!isValidContact(contact)) {
        setReason(reason, "invalid contact (optional '+', then 7-15 digits)");
        return ERR_INVALID_INPUT;
    }
    return OK;
}

const char* statusToString(Status s) {
    switch (s) {
        case OK:                   return "OK";
        case ERR_DUPLICATE_ID:     return "A user with this ID already exists.";
        case ERR_NOT_FOUND:        return "User (or friendship) not found.";
        case ERR_SELF_FRIEND:      return "A user cannot be friends with themselves.";
        case ERR_DUPLICATE_EDGE:   return "These users are already friends.";
        case ERR_INVALID_INPUT:    return "Invalid input.";
        case ERR_NETWORK_EMPTY:    return "The network is empty.";
        case ERR_NO_PATH:          return "No path exists between these users.";
        case ERR_FILE_IO:          return "File error (could not read or write the file).";
        case ERR_MALFORMED_RECORD: return "Malformed record.";
        case ERR_CAPACITY:         return "Capacity exceeded (maximum number of users reached).";
    }
    return "Unknown error.";
}
