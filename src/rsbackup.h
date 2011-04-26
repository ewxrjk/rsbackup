//-*-C++-*-
#ifndef RSBACKUP_H
#define RSBACKUP_H

#include <vector>
#include <string>

class Document;

void pruneBackups();
void prunePruneLogs();
void generateReport(Document &d);

int execute(const std::vector<std::string> &command);

extern char stylesheet[];

#endif /* RSBACKUP_H */
