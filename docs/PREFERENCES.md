# Beam68 0.2 Preferences

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
