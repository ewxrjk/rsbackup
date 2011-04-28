//-*-C++-*-
#ifndef RSBACKUP_H
#define RSBACKUP_H

#include <vector>
#include <string>

class Document;

void makeBackups();
void retireVolumes();
void retireDevices();
void pruneBackups();
void prunePruneLogs();
void generateReport(Document &d);

extern char stylesheet[];
extern int errors;                      // count of errors

#endif /* RSBACKUP_H */
