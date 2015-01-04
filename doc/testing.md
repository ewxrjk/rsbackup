rsbackup testing
================

rsbackup contains, broadly speaking, three collections of tests.

Unit Tests
----------

These are `src/test-*.cc` and test individual aspects of the source
code.  Currently they are strongest on utility functions and classes
and weakest on the functional logic of rsbackup.

Missing things include:

* The `Backup` class.
* Error/warning reporting.
* Store identification.
* Device access hook invocation.
* Most of the implementation of the following classes:
  * `Host`
  * `Volume`
  * `Conf`
* Most of the hard bits of the `Subprocess` class.
* Much of the `Command` class.
* Error handling in the following classes:
  * `Directory`
  * `IO`
  * `Unicode`
* Functional logic such as `MakeBackup.cc` and `Prune.cc`.

Functional Tests
----------------

These can be found in the `tests` directory.  They execute the
`rsbackup` commands in various ways and verify that the behavior and
output are as expected.

Missing things include:
* Sending email.
* Responses to subprocess, IO and database errors.
* Prompting.
* Warning options.
* Config file syntax errors.
* Certain config file options.
* Store permission checking.
* Handling of ‘incomplete’ backups.
* Removal of old pruning records.
* Remote backups (needs a ‘fake’ `ssh`).
* 

Script Tests
------------

These are `tools/t-*` and test some of the scripts that accompany
rsbackup.  They use `scripts/fakeshell.sh` to mock commands invoked by
the scripts.

Missing:

* `rsbackup-mount`
* `rsbackup.cron`

Running Tests
-------------

Tests can be run via `make check` in the root directory, or in one of
the individual directories listed above.

To record test coverage information, configure with:

    ./configure --with-gcov
