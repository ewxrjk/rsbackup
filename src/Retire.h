#ifndef RETIRE_H
#define RETIRE_H

#include <string>
#include <vector>

void removeObsoleteLogs(const std::vector<std::string> &obsoleteLogs,
                        bool removeBackup);

#endif /* RETIRE_H */

