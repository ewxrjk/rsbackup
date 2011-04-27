#include <config.h>
#include "Unicode.h"
#include "Errors.h"
#include <iconv.h>
#include <langinfo.h>
#include <cerrno>
#include <cstring>

// We need to specify the endianness explicitly or iconv() assumes we want a
// BOM.
#ifdef WORDS_BIGENDIAN
# define ENCODING "UTF-32BE"
#else
# define ENCODING "UTF-32LE"
#endif

void toUnicode(std::wstring &u, const std::string &mbs) {
  static iconv_t cd;

  if(cd == 0) {
    char *mbsEncoding = nl_langinfo(CODESET);
    cd = iconv_open(ENCODING, mbsEncoding);
    if(!cd)
      throw std::runtime_error(std::string("iconv_open: ") + strerror(errno));
  }

  u.clear();
  wchar_t buffer[1024];
  char *inptr = (char *)mbs.data();
  size_t inleft = mbs.size();

  while(inleft > 0) {
    char *outptr = (char *)buffer;
    size_t outleft = sizeof buffer;
    size_t n = iconv(cd, &inptr, &inleft, &outptr, &outleft);
    if(n == (size_t)-1 && n != E2BIG)
      throw std::runtime_error(std::string("iconv: ") + strerror(errno));
    u.append(buffer, (wchar_t *)outptr - buffer);
  }
}
