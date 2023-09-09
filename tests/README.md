# rsbackup Test Suite

## General

* Every test uses an independent `${WORKSPACE}` for mutable data (e.g. config files, backups, backup data, logs), named `workspaces/${test name}`
* `setup.sh` generates a configuration, a couple of stores, and some volumes, to act as test data
* `tests/hook` is used to test hook invocation behavior

### Shell Functions

#### `compare`

Used to compare a generated file with an expected file.
`TEST_PATCH=true` can be set to regenerate expected files.

#### `exists` and `absent`

Used to verify that a named file exists or does not exist.

#### `s`

Used to prefix commands that are expected to succeed,
and responsible for setting up their environment.

#### `fails`

Used to prefix commands that are expected to succeed,
and responsible for setting up their environment
(currently in a more limited way that `s`).

### Environment Variables

#### `STDOUT` and `STDERR`

Where to send output for the command.

#### `RSBACKUP_TIME`

Overrides the time that `rsbackup` thinks it is.

## Hook Testing

A single file `tests/hook` acts as all hooks and uses `${RSBACKUP_HOOK}` to distinguish.

The `pre-` hooks all create a stamp file (`volume-hook.stamp` or `device-hook.stamp`)
and the `post-` hooks remove it.
Both check that it is absent or, respectively, present.

If `${RUN}` is set then a `.ran` stamp file is created to indicate that the hook was run.
Normally `.acted` file is also created; the exception is in dry-run mode.

### Environment Variables for Hook Testing

#### `RUN`

Provides a name (scoped to the particular test) for this invocation of the command.

#### `RSBACKUP_HOOK`

Identifies the hook that `rsbackup` is running.

#### `PRE_VOLUME_HOOK_STDERR`, `POST_VOLUME_HOOK_STDERR`, etc

Contains the required stderr output for a hook.

#### `PRE_VOLUME_HOOK_STATUS`, `POST_VOLUME_HOOK_STATUS`, etc

Contains the required exit status for a hook.
