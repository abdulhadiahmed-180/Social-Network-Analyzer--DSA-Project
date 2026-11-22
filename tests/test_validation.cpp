/* test_validation.cpp - string helpers, validators, parsing, splitFields */
#include "TestHelper.h"
#include "FileManager.h"

static void testStringHelpers() {
    CHECK(trim("  hi  ") == "hi", "trim both sides");
    CHECK(trim("\t x y \r\n") == "x y", "trim tabs/newlines, keep inner space");
    CHECK(trim("   ") == "", "trim all-space string");
    CHECK(trim("") == "", "trim empty string");
    CHECK(toLowerChar('A') == 'a' && toLowerChar('z') == 'z' && toLowerChar('5') == '5', "toLowerChar");
    CHECK(equalsIgnoreCase("AbC", "aBc"), "equalsIgnoreCase true");
    CHECK(!equalsIgnoreCase("abc", "abcd"), "equalsIgnoreCase different length");
    CHECK(!equalsIgnoreCase("abc", "abd"), "equalsIgnoreCase different content");
    CHECK(containsIgnoreCase("Software Engineer", "ENGIN"), "contains partial, any case");
    CHECK(containsIgnoreCase("Karachi", "karachi"), "contains whole word");
    CHECK(!containsIgnoreCase("Karachi", "lahore"), "contains false");
    CHECK(!containsIgnoreCase("ab", "abc"), "needle longer than haystack");
    CHECK(containsIgnoreCase("anything", ""), "empty needle matches");
    CHECK(padRight("ab", 5) == "ab   ", "padRight pads");
    CHECK(padRight("abcdef", 3) == "abcdef", "padRight never truncates");
}

static void testParsing() {
    int v = 0;
    CHECK(parseInteger("42", v) && v == 42, "parse 42");
    CHECK(parseInteger("  7 ", v) && v == 7, "parse with spaces");
    CHECK(parseInteger("-15", v) && v == -15, "parse negative");
    CHECK(parseInteger("0", v) && v == 0, "parse zero");
    v = 123;
    CHECK(!parseInteger("", v) && v == 123, "empty string rejected, value untouched");
    CHECK(!parseInteger("abc", v), "letters rejected");
    CHECK(!parseInteger("12a", v), "trailing letter rejected");
    CHECK(!parseInteger("1.5", v), "decimal rejected");
    CHECK(!parseInteger("-", v), "lone minus rejected");
    CHECK(!parseInteger("+5", v), "plus sign rejected");
    CHECK(!parseInteger("1234567890", v), "10 digits rejected (overflow guard)");
    CHECK(parseInteger("999999999", v) && v == 999999999, "9 digits accepted");

    CHECK(FileManager::parsePositiveInt("5", v) && v == 5, "positive int ok");
    CHECK(!FileManager::parsePositiveInt("0", v), "zero is not positive");
    CHECK(!FileManager::parsePositiveInt("-3", v), "negative is not positive");
    CHECK(!FileManager::parsePositiveInt("x", v), "non-numeric rejected");
}

static void testSplitFields() {
    std::string f[3];
    CHECK(FileManager::splitFields("a|b|c", '|', f, 3) == 3 && f[0] == "a" && f[1] == "b" && f[2] == "c",
          "split three fields");
    CHECK(FileManager::splitFields("a||c", '|', f, 3) == 3 && f[1] == "", "empty middle field");
    CHECK(FileManager::splitFields("a|b|", '|', f, 3) == 3 && f[2] == "", "empty last field");
    CHECK(FileManager::splitFields("", '|', f, 3) == 1, "empty line = one empty field");
    CHECK(FileManager::splitFields("a|b|c|d|e", '|', f, 3) == 5, "returns TOTAL count when too many");
    CHECK(f[2] == "c", "only the first maxFields are stored");
    CHECK(FileManager::splitFields("only", '|', f, 3) == 1, "no delimiter = 1 field");
}

static void testFieldValidators() {
    CHECK(isValidId(1) && isValidId(MAX_ID), "valid IDs");
    CHECK(!isValidId(0) && !isValidId(-1) && !isValidId(MAX_ID + 1), "invalid IDs");

    CHECK(isValidAge(MIN_AGE) && isValidAge(MAX_AGE) && isValidAge(30), "valid ages");
    CHECK(!isValidAge(MIN_AGE - 1) && !isValidAge(MAX_AGE + 1) && !isValidAge(-5), "invalid ages");

    CHECK(isValidTextField("Ali"), "simple text ok");
    CHECK(isValidTextField("  Ali Khan  "), "text with outer spaces ok (trimmed)");
    CHECK(!isValidTextField(""), "empty text rejected");
    CHECK(!isValidTextField("    "), "blank text rejected");
    CHECK(!isValidTextField("A|B"), "pipe rejected");
    CHECK(!isValidTextField("tab\there"), "control character rejected");
    CHECK(isValidTextField(std::string(MAX_FIELD_LEN, 'x')), "exactly 50 chars ok");
    CHECK(!isValidTextField(std::string(MAX_FIELD_LEN + 1, 'x')), "51 chars rejected");

    CHECK(isValidContact("03001234567"), "local number ok");
    CHECK(isValidContact("+923001234567"), "+ prefix ok");
    CHECK(isValidContact("1234567"), "minimum 7 digits ok");
    CHECK(isValidContact("+123456789012345"), "maximum 15 digits ok");
    CHECK(!isValidContact("123456"), "6 digits rejected");
    CHECK(!isValidContact("1234567890123456"), "16 digits rejected");
    CHECK(!isValidContact("+"), "plus only rejected");
    CHECK(!isValidContact(""), "empty rejected");
    CHECK(!isValidContact("0300-1234567"), "dash rejected");
    CHECK(!isValidContact("0300 1234567"), "inner space rejected");
    CHECK(!isValidContact("03001a34567"), "letter rejected");
    CHECK(!isValidContact("++1234567"), "double plus rejected");
    CHECK(!isValidContact("1234567+"), "trailing plus rejected");
}

static void testValidateUserFields() {
    std::string reason;
    CHECK(validateUserFields(1, "A", 20, "B", "C", "1234567", &reason) == OK, "all fields valid");
    CHECK(validateUserFields(0, "A", 20, "B", "C", "1234567", &reason) == ERR_INVALID_INPUT && !reason.empty(),
          "bad id gives a reason");
    CHECK(validateUserFields(1, "", 20, "B", "C", "1234567", &reason) == ERR_INVALID_INPUT, "bad name");
    CHECK(validateUserFields(1, "A", 5, "B", "C", "1234567", &reason) == ERR_INVALID_INPUT, "bad age");
    CHECK(validateUserFields(1, "A", 20, "B|", "C", "1234567", &reason) == ERR_INVALID_INPUT, "bad city");
    CHECK(validateUserFields(1, "A", 20, "B", "", "1234567", &reason) == ERR_INVALID_INPUT, "bad profession");
    CHECK(validateUserFields(1, "A", 20, "B", "C", "12", &reason) == ERR_INVALID_INPUT, "bad contact");
    CHECK(validateUserFields(1, "A", 20, "B", "C", "12") == ERR_INVALID_INPUT, "works without a reason pointer");
}

static void testSafeIntArray() {
    SafeIntArray a(3);
    int v = -1;
    CHECK(a.getSize() == 3, "initial size");
    CHECK(a.at(0, v) && v == 0, "starts zeroed");
    CHECK(a.set(2, 7) && a.at(2, v) && v == 7, "set/at in range");
    v = 55;
    CHECK(!a.at(3, v) && v == 55, "at(size) fails and leaves value alone");
    CHECK(!a.at(-1, v), "at(-1) fails");
    CHECK(!a.set(3, 1) && !a.set(-1, 1), "set out of range fails");
    CHECK(a.getOr(99, -5) == -5 && a.getOr(2, -5) == 7, "getOr fallback / real value");
    a.fill(4);
    CHECK(a.getOr(0, 0) == 4 && a.getOr(2, 0) == 4, "fill");

    for (int i = 0; i < 100; i++) a.append(i);
    CHECK(a.getSize() == 103 && a.getOr(102, -1) == 99, "append grows the array");
    CHECK(a.resize(2) && a.getSize() == 2, "resize smaller");
    CHECK(a.resize(5) && a.getOr(4, -1) == 0, "resize bigger gives zeros");
    CHECK(!a.resize(-1), "negative resize rejected");
    a.clear();
    CHECK(a.getSize() == 0 && !a.at(0, v), "clear");

    SafeIntArray empty;
    CHECK(empty.getSize() == 0 && !empty.set(0, 1), "default array is empty and safe");
    SafeIntArray negative(-4);
    CHECK(negative.getSize() == 0, "negative initial size = 0");

    SafeIntArray b(2);
    b.set(0, 11); b.set(1, 22);
    SafeIntArray c(b);
    b.set(0, 99);
    CHECK(c.getOr(0, 0) == 11, "copy constructor is a deep copy");
    SafeIntArray d;
    d = b;
    b.set(1, 77);
    CHECK(d.getOr(1, 0) == 22, "assignment is a deep copy");
    d = d;
    CHECK(d.getSize() == 2, "self assignment safe");
}

int main() {
    testStringHelpers();
    testParsing();
    testSplitFields();
    testFieldValidators();
    testValidateUserFields();
    testSafeIntArray();
    return TEST_SUMMARY("test_validation");
}
