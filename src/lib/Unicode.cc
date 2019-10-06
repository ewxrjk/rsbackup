// Copyright © 2011, 2012 Richard Kettlewell.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <config.h>
#include "Utils.h"
#include "Errors.h"
#include <iconv.h>
#include <langinfo.h>
#include <cerrno>
#include <cstring>

// We need to specify the endianness explicitly or iconv() assumes we want a
// BOM.
#ifdef WORDS_BIGENDIAN
#define ENCODING "UTF-32BE"
#else
#define ENCODING "UTF-32LE"
#endif

void toUnicode(std::u32string &u, const std::string &mbs) {
  static iconv_t cd;

  if(cd == nullptr) {
    char *mbsEncoding = nl_langinfo(CODESET);
    cd = iconv_open(ENCODING, mbsEncoding);
    if(!cd)
      throw std::runtime_error(std::string("iconv_open: ") + strerror(errno));
  }

  u.clear();
  char32_t buffer[1024];
  char *inptr = (char *)mbs.data();
  size_t inleft = mbs.size();

  while(inleft > 0) {
    char *outptr = (char *)buffer;
    size_t outleft = sizeof buffer;
    size_t n = iconv(cd, ICONV_FIXUP & inptr, &inleft, &outptr, &outleft);
    if(n == (size_t)-1 && errno != E2BIG)
      throw SystemError(std::string("iconv: "), errno);
    u.append(buffer, (char32_t *)outptr - buffer);
  }
}
