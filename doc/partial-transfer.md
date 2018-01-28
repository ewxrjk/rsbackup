Partial Transfer
================

What is the ‘partial transfer’ warning about?

It corresponds to an error status of 24 from `rsync`, which documents
it as ‘Partial transfer due to vanished source files’.

Within `rsync`, this is:

    #define RERR_VANISHED   24      /* file(s) vanished on sender side */

...and:

    { RERR_VANISHED   , "some files vanished before they could be transferred" },

...and:

    /* VANISHED is not an error, only a warning */
    if (code == RERR_VANISHED) {
        rprintf(FWARNING, "rsync warning: %s (code %d) at %s(%d) [%s=%s]\n",
            name, code, file, line, who_am_i(), RSYNC_VERSION);
    } else {
        rprintf(FERROR, "rsync error: %s (code %d) at %s(%d) [%s=%s]\n",
            name, code, file, line, who_am_i(), RSYNC_VERSION);
    }

It is set during cleanup:

    if (exit_code == 0) {
        if (code)
            exit_code = code;
        if (io_error & IOERR_DEL_LIMIT)
            exit_code = RERR_DEL_LIMIT;
        if (io_error & IOERR_VANISHED)
            exit_code = RERR_VANISHED;
        if (io_error & IOERR_GENERAL || got_xfer_error)
            exit_code = RERR_PARTIAL;
    }

`IOERR_VANISHED` is set in three places. The general idea seems to be
that the file (or link or whatever) formerly existed (e.g. in a
directory listing) but has now gone away. So this reflects either a
backup target changing ‘underfoot’, or strange filesystem semantics.

