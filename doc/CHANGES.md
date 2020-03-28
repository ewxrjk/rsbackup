# Changes To rsbackup

Please see [rsbackup in git](https://github.com/ewxrjk/rsbackup) for detailed change history.

## Changes In rsbackup 7.0

* The `--force` option is extended to override backup policies.
* In `/etc/rsbackup/defaults`, the `hourly`, `daily`, `weekly` and `monthly` settings are gone. Instead, use backup policies. Fixes [issue #59](https://github.com/ewxrjk/rsbackup/issues/59).
* The hook directives are renamed, to (`pre`,`post`)`-`(`device`,`volume`)`-hook`. **Advance warning**: In some future version the old names will be removed.
* `pre-volume-hook` and `post-volume-hook` can be suppressed (e.g. in a volume that has inherited a hook from its host) by passing an empty command.. Fixes [issue #71](https://github.com/ewxrjk/rsbackup/issues/71).
* `RSBACKUP_STATUS`, `RSBACKUP_DEVICE` and `RSBACKUP_STORE` are no longer provided to volume hooks. `pre-volume-hook` output is no longer logged as part of a backup record.
* `pre-volume-hook` is now run only once, before all backups of a volume, and `post-volume-hook` is now run only once, after all backups of a volume. Fixes [issue #17](https://github.com/ewxrjk/rsbackup/issues/17).
* A new `rsync-link-dest` option allows use of the `rsync --link-dest` option to be suppressed, for instance to deal with volumes which only have constantly changing files. Fixes [issue #70](https://github.com/ewxrjk/rsbackup/issues/70).
* The parameters for `decay-limit`, `decay-start`, `decay-window`, `hook-timeout`, `keep-prune-logs`,  `max-age`, `min-interval`, `prune-age`, `prune-logs`, `rsync-timeout` and `ssh-timeout` now include a units suffix. **Advance warning**: In some future version the suffix will be mandatory.
* **Incompatible change**: old logfiles from releases before 2.0 are no longer upgraded. Instead, if such logfiles are detected, an error is reported. You must use a release between 2.0 and 6.0 to upgrade such log files before version 7.0 will run. In a future release even the detection of the old logfiles will be removed.
* **Incompatible change**: the `always-up` directive has been removed. Instead, use `host-check always-up`.

## Changes In rsbackup 6.0

* **Incompatible change**: the `min-backups` and `prune-age` directives have been removed. Instead, use `prune-parameter min-backups` and `prune-parameter prune-age`.
* **Incompatible change**: the arguments to the `public`, `always-up` and `check-mounted` directives are now mandatory.
* **Incompatible change**: the deprecated `colors` and `report-prune-logs` directives have been removed. Use `colors-good`, `colors-bad` and `report` instead.
* **Incompatible change**: the `exec` prune policy now uses absolute timestamps instead of ages. This is a side-effect of fixing [issue #49](https://github.com/ewxrjk/rsbackup/issues/49) (see below).
* Concurrent backup of distinct hosts to distinct devices. See the `group` directive for fine-grained control and [issue #18](https://github.com/ewxrjk/rsbackup/issues/18) for discussion.
* More conservative CSS syntax is used, improving interoperability with AquaMail. Fixes [issue #52](https://github.com/ewxrjk/rsbackup/issues/52).
* Line lengths in encoded images are bounded, improving interoperability with Exim. Fixes [issue #53](https://github.com/ewxrjk/rsbackup/issues/53).
* Images in emails use attachments rather than inline `data:` URLs. This improves interoperability with GMail. Fixes [issue #54](https://github.com/ewxrjk/rsbackup/issues/54).
* New `database` directive controls the database filename. Fixes [issue #55](https://github.com/ewxrjk/rsbackup/issues/55).
* The backup size is now included in the report. Fixes [issue #51](https://github.com/ewxrjk/rsbackup/issues/51).
* Backups can now happen more than once per day. See the `backup-policy` directive for fine-grained control over backup frequency. The default remains one backup per day but backup filenames have changed to reflect the possibility of more than one. Fixes [issue #49](https://github.com/ewxrjk/rsbackup/issues/49).
* Removal of old prune logs was broken and has been re-enabled, and volume retire no longer tries to remove pruned backups. Fixes [issue #56](https://github.com/ewxrjk/rsbackup/issues/56).

## Changes In rsbackup 5.1

* Store directories are now normally required to be mount points. See the description of `store` and `store-pattern` in [rsbackup(5)](rsbackup.5.html) and `--unmounted-store` in [rsbackup(1)](rsbackup.1.html) for options to restore the previous behavior. Fixes [issue #42](https://github.com/ewxrjk/rsbackup/issues/42).
* Minor build fixes.

## Changes In rsbackup 5.0

* **Incompatible change**: Configuration file parsing has changed slightly, with stricter rules about indentation. See [rsbackup(5)](rsbackup.5.html) for details.
* **Incompatible change**: The default location for snapshots has changed to `/var/lib/rsbackup/snapshots`. Fixes [issue #28](https://github.com/ewxrjk/rsbackup/issues/28). **If you use snapshots you must adjust your configuration.**
* New `host-check` directive controlling how to test whether hosts are up or down. Fixes [issue #26](https://github.com/ewxrjk/rsbackup/issues/26).  
**Advance warning**: the old `always-up` directive is now deprecated and will produce a warning. In some future version it will be removed.
* ACLs and extended attributes are now backed up. Note that the options used assume a modern version of `rsync`, and are not supported by the version installed under macOS; also this feature can also cause some trouble with Windows filesystems. Set the `rsync-extra-options` as discussed in [rsbackup(5)](rsbackup.5.html) to work around this. Fixes [issue #37](https://github.com/ewxrjk/rsbackup/issues/37) and [issue #41](https://github.com/ewxrjk/rsbackup/issues/41).
* The `--retire` option now always requests confirmation from the user. Fixes [issue #38](https://github.com/ewxrjk/rsbackup/issues/38).
* New `--forget-only` option used with `--retire` to drop database records for backups without deleting the backups themselves. Fixes [issue #41](https://github.com/ewxrjk/rsbackup/issues/41).
* `pre-backup-hook` scripts may now exit with a distinct exit status to indicate a transient failure, equivalent to a `check-file` or `check-mounted` test failing. Addresses [issue #43](https://github.com/ewxrjk/rsbackup/issues/43).

## Changes In rsbackup 4.0

* A new tool, `rsbackup-graph`, has been introduced. This generates a graphical representation of available backups.
* The `colors` directive is now split into `colors-good` and `colors-bad` which can take either RGB or HSV parameters.  
**Advance warning**: the old `colors` directive is now deprecated and will produce a warning. In some future version it will be removed.
* Report contents can now be parameterized using the new `report` directive.  
**Advance warning**: the old `report-prune-logs` directive is now deprecated and will produce a warning. In some future version it will be removed.
* Configuration file documentation has been moved to a new man page, [rsbackup(5)](rsbackup.5.html).
* Various minor bugs have been fixed.

## Changes In rsbackup 3.1

* Don’t throw exceptions from destructors. Addresses [Debian bug #811705](https://bugs.debian.org/811705).
* Fix error handling in character encoding translation.
* Patch from Maria Valentina Marin to use consistent mtimes during `.deb` build. Fixes [Debian bug #793716](https://bugs.debian.org/793716).
* Stop cron scripts exiting nonzero if `.deb` is removed. Fixes [Debian bug #810335](https://bugs.debian.org/810335).
* Patch from Jonathon Wiltshire to use `install` rather than `cp` during `.deb` builds.
* Correct distribution of scripts.
* Add missing `.deb` build dependencies.

## Changes In rsbackup 3.0

* Pruning now supports selectable, and pluggable, pruning policies. See the `PRUNING` section of the man page for further information. The default behavior matches previous versions. [Fixes issue #7](https://github.com/ewxrjk/rsbackup/issues/7).  
**Advance warning**: the `min-backups` and `prune-age` directives are now deprecated in their current form and will produce a warning. In some future version they will be removed. Instead, use `prune-parameter min-backups` and `prune-parameter prune-age`.
* **Advance warning**: the `public`, `always-up`, `check-mounted` and `traverse` directives now take an explicit boolean argument. Using them without an argument is now deprecated (but has not changed in meaning). In some future version the argument will become mandatory.
* Removal of backups (when pruning, or retiring a volume) now parallelizes removal across devices. [Fixes issue #24](https://github.com/ewxrjk/rsbackup/issues/24).
* The `rsync-timeout` and `hook-timeout` directives are now inherited, as documented. `ssh-timeout` becomes inherited too. The `sendmail` directive is now documented.
* Host and volume names may no longer start with &ldquo;`-`&rdquo;.
* `--dump-config --verbose` now annotates its output. Some options missed by `--dump-config` are now output.
* A C++11 compiler and Boost are now required.

## Changes In rsbackup 2.1

* `rsbackup.cron` will always run the prune and report steps, even if the earlier steps fail.
* `rsbackup-snapshot-hook` copes better with aliases for logical volumes. [Fixes issue #23](https://github.com/ewxrjk/rsbackup/issues/23).
* Pruning logs in the report are now limited by the `report-prune-logs` configuration setting.

## Changes In rsbackup 2.0

* **Incompatible change**: pre-backup and post-backup hooks are now run even in `--dry-run` mode. The environment variables `RSBACKUP_ACT` can be used by the script to distinguish the two situations. `rsbackup-snapshot-hook` has been modified accordingly. [Fixes issue #9](https://github.com/ewxrjk/rsbackup/issues/9).
* **Incompatible change**: The log format has changed. The old per-backup logfiles are gone, replaced by a SQLite database. Old installations are automatically upgraded. [Fixes issue #11](https://github.com/ewxrjk/rsbackup/issues/11).
* New `check-mounted` option verifies that a volume is mounted before backing it up. [Fixes issue #13](https://github.com/ewxrjk/rsbackup/issues/13).
* New `store-pattern` option allows stores to be specified by a glob pattern instead of individually. [Fixes issue #5](https://github.com/ewxrjk/rsbackup/issues/5).
* New `stylesheet` and `colors` options allow operator control of the stylesheet and coloration in the HTML version of the report. [Fixes issue #6](https://github.com/ewxrjk/rsbackup/issues/6).
* The semantics of `lock` are now documented. [Fixes issue #20](https://github.com/ewxrjk/rsbackup/issues/20).
* Shell scripts supplied with `rsbackup` no longer depend on Bash.
* Dashes are now allowed in hostnames. [Fixes issue #21](https://github.com/ewxrjk/rsbackup/issues/21).
* The order in which hosts are backed up can now be controlled with the `priority` option. [Fixes issue #19](https://github.com/ewxrjk/rsbackup/issues/19).
* Reports now include counts of various error/warning conditions in the summary section; email reports reflect these in the subject line. The `always-up` option is slightly modified: backups of always-up hosts are attempted, resulting in error logs, even if they do not seem to be available. [Fixes issue #22](https://github.com/ewxrjk/rsbackup/issues/22).
* New `--database` option allows the path to the database to be overridden.

## Changes In rsbackup 1.2

* Quoting and completeness fixes to `--dump-config` option.
* OSX builds work again.
* The cron scripts no longer attempt to run `rsbackup.cron` when it has been removed. Fixes [Debian bug #766455](https://bugs.debian.org/766455).
* Some fixes to Debian packaging.

## Changes In rsbackup 1.1

* Error messages about missing unavailable devices with `--store` are now more accurate. [Fixes issue #10](https://github.com/ewxrjk/rsbackup/issues/10).
* The `include` command now skips filenames that start with `#`. [Fixes issue #12](https://github.com/ewxrjk/rsbackup/issues/12).
* The command-line parser now rejects invalid host and volume names (rather than accepting ones that will never match anything). Zero-length device, host and volume names are now rejected (in all contexts).
* The test suite has been expanded, and supports concurrent execution if a sufficiently recent version of Automake is used. [Fixes issue #14](https://github.com/ewxrjk/rsbackup/issues/14).
* `rsbackup-snapshot-hook` no longer fails if `fsck` finds and fixes errors. It is also now tested. [Fixes issue #15](https://github.com/ewxrjk/rsbackup/issues/15).

## Changes In rsbackup 1.0

* New `--dump-config` option to verify configuration file parse.
* New `--check` option to `rsbackup-mount`.
* Configuration files are now read in a fixed order ([issue #8](https://github.com/ewxrjk/rsbackup/issues/8)).
* The `--force` option no longer implies the `--verbose` option. (This was a bug.)
* Minor bug fixes.

## Changes In rsbackup 0.4.4

* Correct `RSBACKUP_STATUS` value passed to post-backup hook. (Bug spotted by Jacob Nevins.)

## Changes In rsbackup 0.4.2

* `--retire` no longer fails if a host directory has already been removed.
* Fixed recalculation of per-device backup counts, visible as self-inconsistent reports when generated in the same invocation of `rsbackup` as some other operation.

## Changes In rsbackup 0.4.1

* Fix a crash with the `--html` option (Jon Amery).
* Fix to `--prune-incomplete` option, which wouldn't work in the absence of some other option (Jacob Nevins).

## Changes In rsbackup 0.4

* The new `pre-access-hook` and `post-access-hook` options support running “hook” scripts before and after any access to backup storage devices.
* The new `pre-backup-hook` and `post-backup-hook` options support running “hook” scripts before and after a backup. Although these can be used for any purpose, the motivation is to enable the creation of LVM snapshots of the subject filesystems (and their destruction afterwards), resulting in more consistent backups. The supplied hook script only knows about the Linux logical volume system.
* The new `devices` option allows a host or volume to be restricted to a subset of devices, identified by a filename glob pattern.
* The new `rsync-timeout` option allows a time limit to be imposed on a backup.
* The new `check-file` option allows backups of a volume to be suppressed when it is not available (for instance, because it is only sometimes mounted).
* `--verbose` (and therefore `--dry-run`) is now more verbose.
* `--text` and `--html` now accept `-` to write to standard output.
* Improved error reporting.
* Minor bug fixes and portability and build script improvements.
* `rsbackup-mount` now supports unencrypted devices and separate key material files (contributed by Matthew Vernon).

## Changes In rsbackup 0.3

* `--prune` honours command-line selections again.
* The “oldest” backup for a host with no backups now shows up as “none” rather than “1980-01-01”.
* The new `--logs` option controls which logfiles are included in the HTML report. The default is to only include the logfile of the most recent backup if it failed. Also, if the most recent attempt to backup a volume to a given device failed, its heading is highlighted (in red).
* The tool scripts now have proper `--version` options. Single-letter command line options are now supported (in fact they existed in many cases already but weren’t documented).
* Retiring a volume no longer makes a (futile and harmless!) attempt to remove `.` and `..`.
* The `.incomplete` files used by the Perl script to indicate partial backups are now created by the C++ version too. They are created both before starting a backup and before pruning it. rsbackup itself does not rely on them itself but they are an important hint to the operator when doing bulk restores.
* Logfiles of backups where pruning has commenced are now updated to reflect this, so that they will not be counted as viable backups in the report.
* Error output from failed backups is now more visible. The old behaviour can be restored with the `--no-errors` option.
* Missing or misconfigured stores are now reported in more detail. If it looks like a store is present but misconfigured (for instance, wrong permissions), this is always reported as an error. If it looks like the store is absent then this is only reported if `--warn-store` is given, but if _no_ configured store is present then the problems found with all configured stores are listed. The documentation on how to set up stores has also been clarified.
* Prune logs now include detail about why a backup was eligible for pruning.

## Changes In rsbackup 0.2

`rsbackup` has been rewritten in C++. The behaviour is largely same except as follows:

* New `--text` option generates a plaintext version of the report. In addition the email report includes both the text and HTML versions.
* `--prune-unknown` is removed. It is replaced by `--retire`, which is used to remove backups of volumes (and hosts) that are no longer in use and `--retire-device` which is used to remove logs for devices that are no longer in use.
* The `rsync` command now includes the `--delete` option, meaning that interrupted backups no longer include stray files from the first attempt.
* `.incomplete` files are no longer created. Instead the logs are used to distinguish complete from incomplete backups.
* Various `--warn-` options to control what is warned about.
* New `always-up` option to indicate that a host is expected to always be available to back up.
* There are now test scripts.
