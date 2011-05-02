// -*-C++-*-
// Copyright © 2011 Richard Kettlewell.
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
#ifndef DEFAULTS_H
#define DEFAULTS_H

#if HAVE_PATHS_H
# include <paths.h>
#endif

#ifndef _PATH_DEVNULL
# define _PATH_DEVNULL "/dev/null"
#endif
#ifndef _PATH_SENDMAIL
# define _PATH_SENDMAIL "/usr/sbin/sendmail"
#endif

/* Default values for various things */

#define DEFAULT_CONFIG "/etc/rsbackup/config"
#define DEFAULT_MAX_AGE 3
#define DEFAULT_MIN_BACKUPS 1
#define DEFAULT_PRUNE_AGE 366
#define DEFAULT_MAX_USAGE 80
#define DEFAULT_MAX_FILE_USAGE 80
#define DEFAULT_LOGS "/var/log/backup"
#define DEFAULT_SSH_TIMEOUT 60
#define DEFAULT_KEEP_PRUNE_LOGS 31
#define DEFAULT_SENDMAIL _PATH_SENDMAIL

/* Colors */

#define COLOR_GOOD 0xE0FFE0
#define COLOR_BAD 0xFF4040

/* Valid names */

#define LOWER "abcdefghijklmnopqrstuvwxyz"
#define UPPER "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define ALPHA LOWER UPPER
#define DIGIT "0123456789"

#define DEVICE_VALID ALPHA DIGIT "_."
#define HOST_VALID ALPHA DIGIT "."
#define VOLUME_VALID ALPHA DIGIT "_."

#define PATH_SEP "/"

#define MIME_BOUNDARY "a911ebf382e50dffdf966c4acf269d36e48824bb"

#endif /* DEFAULTS_H */
