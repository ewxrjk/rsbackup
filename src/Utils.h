//-*-C++-*-
#ifndef UTILS_H
#define UTILS_H

#include <string>

// Display a prompt and insist on a yes/no reply.
// Overridden by --force (which means 'always yes').
bool check(const char *format, ...);

// rm -rf PATH
void BulkRemove(const std::string &path);

// Convert mbs from native multibyte encoding to a Unicode string.  We
// assume that wchar_t is UTF-32.
void toUnicode(std::wstring &u, const std::string &mbs);

#endif /* UTILS_H */

