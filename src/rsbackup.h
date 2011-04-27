//-*-C++-*-
#ifndef RSBACKUP_H
#define RSBACKUP_H

#include <vector>
#include <string>

class Document;

void makeBackups();
void pruneBackups();
void prunePruneLogs();
void generateReport(Document &d);

extern char stylesheet[];

#endif /* RSBACKUP_H */
