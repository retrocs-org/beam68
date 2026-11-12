# Beam68 Plugin SDK (protocol 1)

Write a plugin, pack it, and anyone can install it from **Beam68Prefs →
Plugins** with one click. This SDK contains everything a third-party author
needs:

```
include/beam68plugin.h     the plugin API (input decoding, row messages)
src/beam68plugin.c         its implementation: compile it into your plugin
Example/HelloWorld/        a complete plugin: source + manifest
Example/Makefile           vbcc and m68k-amigaos-gcc builds, `make package`
tools/package-plugin.py    validates a plugin drawer and creates the archive
```

## 1. How a plugin works

A plugin is an ordinary AmigaOS executable. When the user types the plugin's trigger or prefix and presses Enter, Beam68 runs:

```text
"<plugin drawer>/MyPlugin" <INPUT-AS-UPPERCASE-HEX> "T:Beam68-<unique id>" BEAM68_REPLY <serial>
```

- `argv[1]` is hex input (`-` for empty), at most 120 bytes.
- `argv[2]` is a reserved request-specific reply path for the ARexx transport.
- `argv[3]` is `BEAM68_REPLY`; `argv[4]` is the decimal 32-bit request serial.
- Native plugins send one Exec message containing up to eight selectable rows.
  The message version and manifest `VERSION` are both **1**, the first public protocol.
- `beam68_plugin_input()` validates the invocation and decodes the input.
  `beam68_plugin_text(argc, argv, text)` and `beam68_plugin_error(argc, argv, text)`
  send a single text row and return 0 on delivery or 20 on failure.

```c
#include <stdio.h>
#include "beam68plugin.h"

int main(int argc, char **argv)
{
    static char input[BEAM68_PLUGIN_MAX_INPUT+1];   /* static: small stack */
    static char reply[BEAM68_PLUGIN_MAX_RESULT+1];
    if (!beam68_plugin_input(argc, argv, input, sizeof(input))) return 20;
    if (!*beam68_trim(input)) return beam68_plugin_error(argc, argv, "Type a name");
    sprintf(reply, "Hello, %.100s!", beam68_trim(input));
    return beam68_plugin_text(argc, argv, reply);
}
```

### ARexx plugins

`COMMAND=PLUGINDIR:MyPlugin.rexx` makes the launcher run
`RX MyPlugin.rexx <HEX> <reply-file>`. Decode the input with `X2C(hex)`
and write one `TEXT=`/`ERROR=` line to `<reply-file>.part`, then rename it;
`Example/HelloRexx/` shows the complete pattern. ARexx scripts use the Protocol 1 text transport and need RexxMast running.
They receive only the hex input and reply-file arguments, and return one
text or error result. Native ARexx host-control plugins still use row messages.

### Live previews

`PREVIEW=YES` asks the launcher to run the plugin about 400 ms after the
user stops typing and show the answer under the query. Make such plugins
quick and side-effect free; previews never count towards quarantine and
SYSTEM plugins never preview.

### Selectable results in Protocol 1

Every native manifest declares `VERSION=1`. Return text, paths or new queries:

```c
beam68_rows_begin();
beam68_row(BEAM68_ROW_OPEN,  "Work:Docs/ReadMe", "Work:Docs/ReadMe"); /* opens it */
beam68_row(BEAM68_ROW_QUERY, "Search again", "find:readme");          /* new query */
beam68_row(BEAM68_ROW_TEXT,  "42 matches", NULL);                     /* copies text */
return beam68_rows_send(argc, argv, 0);
```

The rows arrive as one message on the launcher's public port. The launcher
owns and frees the public allocation after receipt. Request serials keep late
replies from replacing a newer result. If the port is missing or memory cannot
be allocated, delivery fails with 20; native replies never fall back to a
file or discard extra rows. Rebuild pre-release plugins with this SDK: experimental invocation formats are not supported.

## 2. The package layout

A package is **one drawer** containing the manifest, the executable and any
optional files:

```text
HelloWorld/
├── HelloWorld.plugin     manifest (exactly one .plugin file)
├── HelloWorld            the executable
├── HelloWorld.info       optional icon
├── HelloWorld.guide      optional AmigaGuide help
└── ...                   anything else your plugin needs
```

Beam68Prefs installs it as `<Beam68 drawer>/Plugins/HelloWorld/`. Inside the
manifest, `PLUGINDIR:` means "this drawer", so the package works wherever
Beam68 is installed. When your executable runs, its `PROGDIR:` is also this
drawer, so it can read its own files with `PROGDIR:file`.

## 3. The manifest

```ini
VERSION=1                         # first public plugin protocol
ID=org.example.helloworld         # unique and permanent: identifies upgrades
PLUGIN_VERSION=1.0                # your version, dotted digits
MIN_BEAM68=0.1                    # lowest Beam68 release you support
NAME=Hello World                  # shown in Preferences and results
AUTHOR=Your Name
LICENSE=MIT
URL=https://example.org/helloworld
TRIGGER=+                         # one printable character
PREFIX=HELLO:                     # optional readable form, one word
COMMAND=PLUGINDIR:HelloWorld      # required form for packages
DESCRIPTION=Greets whoever you type.
CATEGORY=Examples
CAPABILITIES=READONLY             # see below
EXAMPLE=hello:Amiga               # inserted by the F1 help browser
HELP=Type hello: followed by a name and press Enter.
ICON=PLUGINDIR:HelloWorld         # optional, or PROGDIR:Beam68
SETTINGS=ENVARC:Beam68/Plugins/HelloWorld.cfg              # optional
SETTING=GREETING|Greeting word|CHOICE|Hello|Hello,Hi,Hey   # optional, max 6
```

Package rules (checked by `package-plugin.py` and again by Beam68Prefs):

- `ID`, `PLUGIN_VERSION`, `NAME`, `TRIGGER` and `COMMAND` are required.
- `COMMAND` and `ICON` must stay inside the drawer: `PLUGINDIR:<file>`, no
  `:`, no leading `/` (the AmigaDOS parent), no `//`.
- `SETTINGS`, if used, must be a flat file in `ENVARC:Beam68/Plugins/`. It
  lives outside your drawer so upgrades and uninstalls keep the user's
  settings. `SETTING=` lines (`KEY|Label|TEXT/NUMBER/BOOL/CHOICE|Default|
  choices`) give your plugin a native settings page; read the file yourself.
- A plugin whose `MIN_BEAM68` is newer than the installed Beam68 is refused.

### Capabilities

Declare what your plugin does; users see it before installing.

| Capability | Meaning |
| --- | --- |
| `READONLY` | only reads information |
| `FILES` | reads or writes files |
| `SHELL` | runs AmigaDOS commands |
| `AREXX` | sends ARexx messages |
| `CLIPBOARD` | reads or changes the clipboard |
| `SYSTEM` | may reboot, signal tasks or change the system; Beam68 asks for a second Enter before every run |

### Help and translations

- `GUIDE=PLUGINDIR:MyPlugin.guide` names an AmigaGuide file in your drawer;
  the Help key on the Plugins page of Beam68Prefs opens it.
- Translate your replies with `beam68_catalog_open("MyPlugin.catalog")` and
  `beam68_str(id, "English text")`; ship catalogs in your drawer as
  `Catalogs/<language>/MyPlugin.catalog` (locale.library looks in the
  program's own drawer first). Build catalogs with CatComp or FlexCat.

## 4. Build and package

```sh
cd Example
make vbcc        # or: make gcc
make package     # -> Beam68-Plugin-HelloWorld-1.0.lha (or .zip)
```

`package-plugin.py` checks the manifest and that the executable is a real
AmigaOS (HUNK) program, then writes an archive with exactly one top-level
drawer. It produces `.lha` when an `lha` command is installed (preferred on
Aminet), otherwise `.zip`. `--check` validates without packing.

## 5. How users install it

In **Beam68Prefs → Plugins**, drop the archive or drawer on the window, or
type/Browse its path and click **Add**. Preferences:

1. lists the archive and refuses absolute or parent-relative paths,
2. unpacks `.lha`/`.lzh` with `C:LhA`, `.zip` with `C:UnZip` (both on
   Aminet; without them, unpack it yourself and install the drawer),
3. validates the manifest and executable,
4. shows name, version, author, licence, capabilities and whether it is an
   upgrade or downgrade, and asks for confirmation,
5. copies it to `Plugins/<Name>.new`, then swaps it into place, and enables it
   (unless its trigger/prefix is already taken).

The same plugin `ID` with a new version upgrades in place. **Remove** offers
to delete the installed files.

## 6. Rules for a good plugin

- **68000 and AmigaOS 3.0+.** No FPU, no MUI/ReAction, no 020-only code.
- **Small stack.** Plugins run on a ~4 KB Shell stack: make buffers `static`.
- **Be quick.** Beam68 stops waiting after the user's timeout (default 30 s)
  and does not kill the child; three consecutive failures quarantine a plugin.
- **Bounded replies.** At most eight rows of 120 display bytes. No console output, no prompts, no escape sequences.
- **Treat input as untrusted.** Validate everything; never pass it to a Shell
  unquoted.
- **Clean up.** Close what you open; leave nothing running.
