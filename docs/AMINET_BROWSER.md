# Aminet Browser (Beam68 0.4)

An independent Protocol 1 plugin, implemented for Beam68 without AmiGet code or
an AmiGet dependency. It uses Aminet's public web listing and readme files.
It is not affiliated with the Aminet operators.

- `aminet:search tracker`: six package rows plus navigation.
- `aminet:info util/wb/Beam68.lha`: summary, version, architecture,
  author, requirements and actions when those fields are available.
- Select **Read full readme** to browse its text inside Beam68.
- `aminet:get util/wb/Beam68.lha`: ask, then download.
- `aminet:install util/wb/Beam68.lha`: ask, download and open the archive
  with the installed Workbench archive handler (workbench.library V44+). The user follows the package's
  own installer or instructions. The plugin does not itself extract archives or
  run their installers. Without a handler, the downloaded file remains available.

Result navigation first replaces the query. Press Enter again to make the next
request. Search and readme operations have no side effects on installed packages.
Live preview is disabled. Results can include multiple architectures; check
readme Architecture and Requires fields before installing packages.

Choose the download drawer under Preferences -> Plugins -> Aminet Browser ->
Settings. The default RAM:Beam68Downloads is temporary; choose a disk drawer to
retain downloads after reboot. Configuration uses the normal ENV/ENVARC precedence.

Downloads have their own close-to-cancel window and a single-session guard.
They acknowledge the launcher before transfer and hold that guard through the
completion dialog. Closing Beam68 or changing its query does not abort a confirmed
transfer. Search/readme requests remain subject to the launcher's normal request
limits. Failures leave existing destination files unchanged. Completed downloads
are committed by rename; crash leftovers in Beam68Aminetxx/archive.part are never
installed and can be removed when no transfer is running. Existing files are refused.

Networking requires bsdsocket.library. HTTP is unencrypted and does not authenticate
archives. No downloaded program is automatically executed by the plugin. Only a
fixed Aminet host is contacted; package paths reject traversal and shell characters.
The code never builds Shell commands from queries or package metadata.

Metadata responses are bounded to 256 KB, 20 seconds and an 8 KB parser row buffer;
archive downloads are bounded to 64 MB and five minutes. DNS resolution timeout is
controlled by the installed TCP/IP stack. The client requests HTTP/1.0 with identity
encoding; redirects, compressed/chunked responses, malformed headers, oversized and
truncated bodies fail explicitly. HTML layout changes may require a parser update.

Build: `make Plugins/AminetBrowser/AminetBrowser` with the normal native toolchain.
The portable parser/HTTP-header regression tests run as part of `make host-test`.
The public website's published-release count remains 13 for 0.1; this plugin is new
in the unreleased 0.4 source tree.

Archive downloads require a declared size and must match it before commit.
Installation preparation also checks the archive signature before opening it.

The download window shows filename, transferred size, total size and percentage
when the server supplies a total, plus a progress bar. Close it to cancel. Once
the download window opens successfully, Beam68 hides its launcher window while
remaining resident. A late handoff cannot hide a newer query.

## Transfer feedback in 0.5

The download window shows transferred bytes and average bytes per second.
For known sizes it also estimates remaining time. Unknown sizes display the
bytes received and an activity indicator without a fabricated percentage.
Very short transfers may finish before a useful speed estimate is available.
Cancel removes partial data; Retry starts a new transfer rather than appending
to an incomplete archive. Existing files are preserved on commit failures.
