//-*-C++-*-
#ifndef UNICODE_H
#define UNICODE_H

#include <string>

// Convert mbs from native multibyte encoding to a Unicode string.  We
// assume that wchar_t is UTF-32.
void toUnicode(std::wstring &u, const std::string &mbs);

#endif /* UNICODE_H */

