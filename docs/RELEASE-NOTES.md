# Beam68 0.2

In development.

- Independent Aminet Browser plugin: search and details inside Beam68, paged
  readmes, confirmed downloads and archive-opening installation preparation.
  Requires a TCP/IP stack; no AmiGet dependency.

- Startup recovers committed index backups after interrupted saves.

- At most three outstanding plugin requests; automatic previews preserve a manual-run slot.

- Safer low-memory recovery, cache replacement and rescan selection handling.

- Coalesced text input and immediate query feedback before ranking results.
- Earlier first matches while indexing; input checked between directory entries.
- Optional icon previews load after the selection settles.
- Matcher optimizations preserve existing scores and ranking.

Requires AmigaOS 3.0+ and a 68000+ CPU; no FPU is required. AmigaOS 3.2 and a
68020 are recommended. Optional integrations require their host applications.
Physical minimum-memory/68000 certification, full clean-install coverage and
all third-party host combinations remain open. Build success and portable tests
are separate from native runtime verification.

The LHA contains the Amiga runtime. The plugin SDK ZIP contains the API,
examples, documentation and package validator. Both have individual SHA-256
files. Application source is available from the tagged Git repository.
