# Debian standards checklist

Things that need attention are listed below.
Their presence here doesn't mean a change is definitely required, but it should be looked into.

## 3.9.7

* 12.3 recommend to ship additional documentation for package "pkg" in a separate package "pkg-doc" and install it into "/usr/share/doc/pkg".
  * See also 4.0.0.0 12.3 (*-doc deps should be at most Recommends)

## 3.9.8

Nothing relevant.

## 4.0.0

* 4.3 "config.sub" and "config.guess" should be updated at build time or replaced with the versions from autotools-dev.
* 4.9.1 New "DEB\_BUILD\_OPTIONS" tag, "nodoc", which says to suppress documentation generation (but continue to build all binary packages, even documentation packages, just let them be mostly empty).

## 4.0.1

Nothing relevant.

## 4.1.0

* ~~4.11 If upstream provides OpenPGP signatures, including the upstream signing key as "debian/upstream/signing-key.asc" in the source package and using the "pgpsigurlmangle" option in "debian/watch" configuration to indicate how to find the upstream signature for new releases is recommended.~~
* 4.15 Packages should build reproducibly when certain factors are held constant; see 4.15 for the list.

## 4.1.1

Nothing relevant.

## 4.1.2

Nothing relevant.

## 4.1.3

* ~~5.6.26 URLs given in "VCS-*" headers should use a scheme that provides confidentiality ("https", for example) if the VCS repository supports it. "[vcs-field-uses-insecure-uri]" _should add VCS headers_~~

## 4.1.4

Nothing relevant.

## 4.1.5

Nothing relevant.

## 4.2.0

* ~~4.9 The package build should be as verbose as reasonably possible. This means that "debian/rules" should pass to the commands it invokes options that cause them to produce verbose output.~~
* 12.7 Upstream release notes, when available, should be installed as "/usr/share/doc/package/NEWS.gz". Upstream changelogs may be made available as "/usr/share/doc/package/changelog.gz".

## 4.2.1

Nothing relevant.

## 4.3.0

* ~~10.1 Binaries should be stripped using "strip --strip-unneeded --remove- section=.comment --remove-section=.note" (as dh\_strip already does).~~

