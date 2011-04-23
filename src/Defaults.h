// -*-C++-*-
#ifndef DEFAULTS_H
#define DEFAULTS_H

#if HAVE_PATHS_H
# include <paths.h>
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
#ifdef _PATH_SENDMAIL
# define DEFAULT_SENDMAIL _PATH_SENDMAIL
#else
# define DEFAULT_SENDMAIL "/usr/sbin/sendmail"
#endif

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

#endif /* DEFAULTS_H */
