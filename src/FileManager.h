/*
 * FileManager.h  -  MODULE C (Member 3): saving and loading the network
 * ---------------------------------------------------------------
 * File formats (lines starting with '#' and blank lines are ignored):
 *   users.txt        id|name|age|city|profession|contact
 *   friendships.txt  idA|idB            (each edge stored once, smaller ID first)
 *
 * LOAD POLICY: "skip and report".  A bad record is rejected, logged with
 * file name + line number + reason, and loading continues.
 */
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include "Graph.h"
#include "Utils.h"

class FileManager {
public:
    /* Writes both files.  Returns OK | ERR_FILE_IO. */
    static Status saveNetwork(const SocialNetwork& net,
                              const std::string& usersPath,
                              const std::string& friendsPath);

    /*
     * Loads both files into 'net'.  The network is CLEARED first, so the
     * result is exactly what the files contain.
     * A missing file is NOT an error (first run): the network is just empty
     * and report.usersFileMissing / friendsFileMissing is set.
     * Returns OK (even if some records were rejected - see the report)
     * or ERR_FILE_IO if a file went bad while being read.
     */
    static Status loadNetwork(SocialNetwork& net,
                              const std::string& usersPath,
                              const std::string& friendsPath,
                              LoadReport& report);

    /*
     * Splits 'line' at every 'delimiter'.  The first maxFields pieces are stored
     * in fields[].  Returns the TOTAL number of pieces found (it can be larger
     * than maxFields, which tells the caller the line has too many fields).
     * An empty line gives 1 piece (the empty string).
     */
    static int splitFields(const std::string& line, char delimiter,
                           std::string fields[], int maxFields);

    /* Strict parse of a positive integer (digits only, 1..999999999). */
    static bool parsePositiveInt(const std::string& text, int& out);

    /* Prints the summary and the stored error lines of a load. */
    static void printLoadReport(const LoadReport& report);
};

#endif
