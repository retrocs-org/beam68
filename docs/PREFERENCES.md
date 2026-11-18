# Beam68 0.4 Preferences

Open Beam68Prefs from SYS:Prefs or press F5 in the launcher.

## Six sections

| Section | Controls |
| --- | --- |
| General | Startup, hotkey, behaviour, source and resource options |
| Indexing | Shared directories, exclusions, file patterns and finite or unlimited depth |
| Plugins | Enable/disable, priority, independent versions, details, settings and import |
| Appearance | Themes, text and readability, window layout |
| Favorites | Pin programs and organize groups such as `fav:games` |
| Application shortcuts | Extra search keywords and custom named launch actions |

The sidebar abbreviates Application shortcuts to App shortcuts. Automatic
application discovery is configured under Indexing; shortcuts customize individual
programs. Escape hides the launcher, leaving its resident hotkey available.

## Defaults and optional integrations

A fresh installation enables Calculator, Clock, Memory, File Finder, System
Commands, and Assigns and Volumes. All other bundled plugins start disabled.
Network plugins, including Aminet Browser and Wikipedia, are always opt-in on
fresh installations. Updates retain explicit saved choices.

Every installer mode installs all bundled packages. There is no plugin-selection
step. Novice uses automatic defaults; Intermediate and Expert expose destination
and startup choices. Existing installations offer update or uninstall. Uninstall
lets you keep or remove settings, including temporary settings when removing them.

WHDLoad uses the shared indexing roots, exclusions and depth. Enabling it starts
background game discovery and provides game launch actions. Music Library owns
music indexing and player configuration. DOpus owns Directory Opus configuration.
ARexx Runner owns macro execution; see each plugin's documentation for dependencies.
Wikipedia shows search results and native article summaries; it requires curl with
HTTPS support. Aminet shows download progress and supports cancellation.
A successful download-window handoff hides the launcher without stopping it.

## Plugin controls

The list shows priority, name, independent version, state and trigger. Priority
up/down reorder plugins and update their numbers. Toggle changes activation.
Import accepts additional plugin packages. Remove is disabled for bundled plugins.
Settings is disabled when a plugin has no settings. Typing preview is disabled
when unsupported; supported previews have an enable control and numeric delay.
Details shows plugin information together in a larger window.

Plugin settings have their own window, Save and Cancel. These controls apply only
to that plugin; they do not close or save unrelated main Preferences edits.

## Use, Save and Cancel

Use applies the main configuration for the current session. Save applies it and
also persists it across restarts. Both wait for confirmation from a running
launcher. If applying fails, the window stays open and reports the failure.
Cancel discards unapplied main-window edits.

Temporary Use configuration is read from `T:Beam68/Beam68.cfg` before ENV: and
ENVARC:. Persistent settings are under `ENVARC:Beam68`. Removing settings during
uninstall clears temporary configuration and its backup too, so it cannot override
a later fresh installation. Updates that keep settings retain user choices.

## Palettes

Mocha is the default theme. Rose is the reddish theme previously named Custom.
Every theme has an editable palette and Restore original palette. Imported and
exported theme files use five RGB colours; restoring one theme does not reset
another theme's saved palette.

## Read-only plugin requests

Wikipedia searches and summaries, and Music Library searches, run when explicitly
requested without an extra SYSTEM confirmation. Music playback still asks for
confirmation. Automatic previews remain unavailable for these SYSTEM integrations.

## 0.4 compact layouts and plugin settings

Indexing uses a Directories / Scan options selector. Switching between these
views preserves depth, exclusions and unsaved edits. Plugin controls and settings
rows use compact spacing on short screens. Plugin Details explains integration
requirements; missing TCP/IP and the Wikipedia downloader produce local guidance
before a request starts. This does not certify internet connectivity or HTTPS
compatibility. OpenURL remains optional when reading summaries.

Plugin Save commits only that plugin's file and notifies the resident to reload
plugins. It does not save unsaved parent Preferences edits. Cancel and Escape
close the plugin dialog without writing. Unrecognized settings and comments are
preserved during Save. A write failure leaves the dialog open for correction.

## Reviewing plugin imports

Import reviews the plugin identity, version, author, declared access and target.
Updates identify upgrades, same-version reinstalls and downgrades. A second
review shows the file count, total bytes and up to six file paths; larger
inventories are explicitly marked as partial. Cancel either step to leave the
installation and registration unchanged. Import does not execute the plugin.

New packages and bare-manifest references start disabled; use Toggle to enable
them, then Use or Save. Updates preserve the existing activation choice, unless
a command conflict requires disabling the plugin. Duplicate bare-manifest IDs
are refused; import a complete package when updating. Bundled plugins are
updated through the Beam68 installer. Directory packages containing links or
exceeding the bounded review limits are refused. Review is not authentication
of the author or a security boundary around native code.

## Restoring plugin settings defaults

Restore defaults fills the open plugin dialog from the manifest's declared
defaults. It does not write settings or notify the running launcher. Save validates
and commits those values; Cancel, Escape or closing the window discards pending
changes. Restore defaults preserves unknown file keys when eventually saved.
A plugin's Save is independent: cancelling the main Preferences window later
does not undo settings already saved in the plugin dialog.

### AmiHASS

The optional AmiHASS plugin starts disabled. Enable it after configuring and
starting AmiHASS. `ha:status` checks connection status; `ha:kitchen` searches
entities. Select an entity for its state and supported actions. Every action
requires confirmation. Connection credentials remain in AmiHASS; Beam68 has
no separate network configuration for this plugin.

### Plugin search history and favorites

Use F7/Shift+F7 in a plugin query to recall that plugin's read-only history.
The application history stays separate, including when older mixed history
files are present. Histories are bounded to 12 entries per plugin and are read
on demand. Unknown or action entries in edited plugin-history files are ignored.

F6 pins or unpins a read-only query or entity result. Saved query favorites use
`QUERY:<query>` records in the usual Favorites.txt and retain favorite groups.
Opening them only fills the query field. Download, playback, device-control and
macro-execution commands cannot be pinned. F6 updates temporary, active and
saved Favorites copies so old Use settings cannot override the new choice.
