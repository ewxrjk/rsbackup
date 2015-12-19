// -*-C++-*-
// Copyright © 2011, 2012, 2014, 2015 Richard Kettlewell.
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
/** @file Defaults.h
 * @brief Default values, etc
 */

#if HAVE_PATHS_H
# include <paths.h>
#endif

/** @cond fixup */
#ifndef _PATH_DEVNULL
# define _PATH_DEVNULL "/dev/null"
#endif
#ifndef _PATH_SENDMAIL
# define _PATH_SENDMAIL "/usr/sbin/sendmail"
#endif
/** @endcond */

/* Default values for various things */

/** @brief Path to configuration file */
#define DEFAULT_CONFIG "/etc/rsbackup/config"

/** @brief Default maximum age */
#define DEFAULT_MAX_AGE 3

/** @brief Default minimum backups */
#define DEFAULT_MIN_BACKUPS "1"

/** @brief Default pruning age */
#define DEFAULT_PRUNE_AGE "366"

/** @brief Default pruning age */
#define DEFAULT_DECAY_START "1"

/** @brief Default decay window size */
#define DEFAULT_DECAY_WINDOW "1"

/** @brief Default decay scale */
#define DEFAULT_DECAY_SCALE "2"

/** @brief Default pruning policy */
#define DEFAULT_PRUNE_POLICY "age"

/** @brief Default age for pruning logs in report */
#define DEFAULT_PRUNE_REPORT_AGE 3

/** @brief Default maximum disk usage */
#define DEFAULT_MAX_USAGE 80

/** @brief Default maximum inode usage */
#define DEFAULT_MAX_FILE_USAGE 80

/** @brief Default log directory */
#define DEFAULT_LOGS "/var/log/backup"

/** @brief Default SSH timeout */
#define DEFAULT_SSH_TIMEOUT 60

/** @brief Default days to keep pruning logs */
#define DEFAULT_KEEP_PRUNE_LOGS 31

/** @brief Default path to email injector */
#define DEFAULT_SENDMAIL _PATH_SENDMAIL

/* Colors */

/** @brief "Good" color in HTML report */
#define COLOR_GOOD 0xE0FFE0

/** @brief "Bad" color in HTML report */
#define COLOR_BAD 0xFF4040

/** @brief Default color-picking strategy */
#define DEFAULT_COLOR_STRATEGY "equidistant-value"

/* Valid names */

/** @brief Lower case letters */
#define LOWER "abcdefghijklmnopqrstuvwxyz"

/** @brief Upper case letters */
#define UPPER "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

/** @brief Letters */
#define ALPHA LOWER UPPER

/** @brief Digits */
#define DIGIT "0123456789"

/** @brief Valid characters in a device name */
#define DEVICE_VALID ALPHA DIGIT "_."

/** @brief Valid characters in a host name */
#define HOST_VALID ALPHA DIGIT ".-"

/** @brief Valid characters in a volume name */
#define VOLUME_VALID ALPHA DIGIT "_."

/** @brief Valid characters in a prune-parameter name */
#define PRUNE_PARAMETER_VALID ALPHA DIGIT "-"

/** @brief Path separator */
#define PATH_SEP "/"

/** @brief MIME boundary string */
#define MIME_BOUNDARY "a911ebf382e50dffdf966c4acf269d36e48824bb"

#endif /* DEFAULTS_H */
