/*
 * Utils.h  -  Shared definitions used by every module
 * ---------------------------------------------------------------
 * Contains: return codes, constants, result structs, the
 * bounds-checked SafeIntArray, hand-written string helpers and the
 * field validators shared by addUser / updateUser / menu / file load.
 *
 * Allowed headers only: <string> here (no STL containers).
 */
#ifndef UTILS_H
#define UTILS_H

#include <string>

/* ===================== Return codes ===================== */
enum Status {
    OK,
    ERR_DUPLICATE_ID,
    ERR_NOT_FOUND,
    ERR_SELF_FRIEND,
    ERR_DUPLICATE_EDGE,
    ERR_INVALID_INPUT,
    ERR_NETWORK_EMPTY,
    ERR_NO_PATH,
    ERR_FILE_IO,
    ERR_MALFORMED_RECORD,
    ERR_CAPACITY
};

enum SearchField { BY_NAME, BY_CITY, BY_PROFESSION };

/* ===================== Constants ===================== */
const int MAX_USERS            = 1000;  // also bounds DFS recursion depth
const int MAX_FIELD_LEN        = 50;
const int MIN_AGE              = 13;
const int MAX_AGE              = 120;
const int MIN_CONTACT_DIGITS   = 7;
const int MAX_CONTACT_DIGITS   = 15;
const int MAX_ERRORS           = 100;   // max error lines stored in a LoadReport

/* Extra constants (small additions to the original spec) */
const int MAX_ID               = 999999999;                        // largest allowed user ID (9 digits, fits in int)
const int MAX_FLAGS_PER_USER   = 4;                                // R1..R4
const int MAX_FLAGS            = MAX_FLAGS_PER_USER * MAX_USERS;   // size of a Flag[] big enough for analyze()
const int R3_MIN_FRIENDS       = 3;     // R3 needs >= 3 friends; with 1-2 friends a triangle is nearly
                                        // impossible and every such user would be flagged for no reason

/* Security rule IDs stored in Flag::ruleId */
const int RULE_MANY_FRIENDS    = 1;     // R1
const int RULE_ISOLATED        = 2;     // R2
const int RULE_LOW_CLUSTERING  = 3;     // R3
const int RULE_SHARED_CONTACT  = 4;     // R4

/* Default data file locations (run the program from the project root) */
const char* const DEFAULT_USERS_FILE   = "data/users.txt";
const char* const DEFAULT_FRIENDS_FILE = "data/friendships.txt";

/* ===================== Result structs ===================== */
struct RankEntry {
    int id;
    std::string name;
    int friendCount;
    RankEntry() : id(0), friendCount(0) {}
};

struct RecommendEntry {
    int id;
    std::string name;
    int mutualCount;
    RecommendEntry() : id(0), mutualCount(0) {}
};

struct Flag {
    int userId;
    int ruleId;              // one of the RULE_* constants
    std::string reason;      // human readable ("potentially suspicious ...")
    int observedValue;
    int thresholdValue;
    Flag() : userId(0), ruleId(0), observedValue(0), thresholdValue(0) {}
};

struct SecurityConfig {
    int  maxFriendThreshold;     // R1: flag if friendCount > this
    bool isolationEnabled;       // R2 on/off
    int  lowClusteringPercent;   // R3: flag if < X% of friends are interconnected (0 = R3 off)
    bool sharedContactEnabled;   // R4 on/off
    SecurityConfig()
        : maxFriendThreshold(50), isolationEnabled(true),
          lowClusteringPercent(20), sharedContactEnabled(true) {}
};

/*
 * LoadReport - summary of one loadNetwork() call.
 * linesRead counts RECORD lines only (blank lines and '#' comments are
 * not counted), so:  linesRead == usersLoaded + edgesLoaded + rejectedCount
 * Only the first MAX_ERRORS error messages are stored; rejectedCount
 * keeps counting beyond that.
 */
struct LoadReport {
    int linesRead, usersLoaded, edgesLoaded, rejectedCount;
    bool usersFileMissing, friendsFileMissing;
    std::string errors[MAX_ERRORS];

    LoadReport() { reset(); }
    void reset() {
        linesRead = usersLoaded = edgesLoaded = rejectedCount = 0;
        usersFileMissing = friendsFileMissing = false;
        for (int i = 0; i < MAX_ERRORS; i++) errors[i].clear();
    }
    int storedErrorCount() const { return rejectedCount < MAX_ERRORS ? rejectedCount : MAX_ERRORS; }
};

/* ===================== SafeIntArray ===================== */
/*
 * Bounds-checked dynamic int array (Rule of Three).
 * An out-of-range index NEVER crashes: at()/set() return false, and
 * getOr() returns the fallback value you give it.
 * Also grows on demand with append() (capacity doubles -> O(1) amortised).
 */
class SafeIntArray {
public:
    explicit SafeIntArray(int initialSize = 0);         // elements start at 0
    SafeIntArray(const SafeIntArray& other);            // copy constructor (deep copy)
    SafeIntArray& operator=(const SafeIntArray& other); // copy assignment (deep copy)
    ~SafeIntArray();

    bool at(int index, int& value) const;   // false if index out of range
    bool set(int index, int value);         // false if index out of range
    int  getOr(int index, int fallback) const; // convenience: value, or fallback if out of range
    void fill(int value);                   // set every element
    int  getSize() const;
    bool resize(int newSize);               // keeps old values, new ones are 0; false if newSize < 0
    bool append(int value);                 // add at the end
    void clear();                           // size becomes 0 (memory is kept for reuse)

private:
    int* data;
    int  size;
    int  capacity;

    void ensureCapacity(int needed);
    void copyFrom(const SafeIntArray& other);
};

/* ===================== String helpers (hand-written) ===================== */
std::string trim(const std::string& text);                   // strip spaces, tabs, \r, \n at both ends
char        toLowerChar(char c);                             // 'A'..'Z' -> 'a'..'z'
bool        equalsIgnoreCase(const std::string& a, const std::string& b);
bool        containsIgnoreCase(const std::string& haystack, const std::string& needle); // empty needle -> true
std::string padRight(const std::string& text, int width);    // pad with spaces (for table output)
bool        parseInteger(const std::string& text, int& out); // optional '-', 1..9 digits; out untouched on failure

/* ===================== Validators (Section 7a) ===================== */
bool isValidId(int id);                          // 1..MAX_ID
bool isValidTextField(const std::string& text);  // non-empty after trim, <= MAX_FIELD_LEN, no '|', no control chars
bool isValidAge(int age);                        // MIN_AGE..MAX_AGE
bool isValidContact(const std::string& text);    // optional '+', then 7..15 digits

/*
 * Validates every user field at once. Returns OK or ERR_INVALID_INPUT.
 * If 'reason' is not null, it receives a short explanation on failure.
 */
Status validateUserFields(int id, const std::string& name, int age,
                          const std::string& city, const std::string& profession,
                          const std::string& contact, std::string* reason = nullptr);

/* Short human-readable message for a Status code. */
const char* statusToString(Status s);

#endif
