#!/bin/sh
# Rebuild Beam68 and stage its AmigaOS runtime package in <root>/Beam68.

set -eu

project_root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
destination_root=${1:-${BEAM68_STAGE_DIR:-}}
application_dir=${destination_root}/Beam68

if [ "$#" -gt 1 ] || [ -z "$destination_root" ]; then
    echo "Usage: $0 destination-root (or set BEAM68_STAGE_DIR)" >&2
    exit 2
fi

case "$destination_root" in
    ""|/)
        echo "Beam68: refusing unsafe destination root: '$destination_root'" >&2
        exit 2
        ;;
esac

runtime_executables="
Beam68
Beam68Prefs
Beam68Ctl
CreateIcons
Plugins/Calc/Calc
Plugins/Clock/Clock
Plugins/Memory/Memory
Plugins/FileFinder/FileFinder
Plugins/SystemCommands/SystemCommands
Plugins/DosTools/DosTools
Plugins/TaskManager/TaskManager
Plugins/ClipboardTools/ClipboardTools
Plugins/DateUtilities/DateUtilities
Plugins/WindowSwitcher/WindowSwitcher
Plugins/DeveloperTools/DeveloperTools
Plugins/ARexxRunner/ARexxRunner
Plugins/MusicControls/MusicControls
"

runtime_data="
Install
Install.info
installer/Beam68-base.cfg
installer/Beam68Start.info
Beam68.guide
Beam68.guide.info
Snippets.txt
themes/Amber.theme
themes/GreenScreen.theme
themes/Paper.theme
themes/MagicGrey.theme
catalogs/deutsch/Beam68.catalog
catalogs/svenska/Beam68.catalog
catalogs/deutsch/Beam68Prefs.catalog
catalogs/svenska/Beam68Prefs.catalog
catalogs/deutsch/Beam68Plugins.catalog
catalogs/svenska/Beam68Plugins.catalog
Beam68.info
Beam68Prefs.info
Beam68.cfg
Aliases.txt
VERSION
Plugins/Calc/Calc.plugin
Plugins/Clock/Clock.plugin
Plugins/Clock/Clock.cfg
Plugins/Memory/Memory.plugin
Plugins/FileFinder/FileFinder.plugin
Plugins/SystemCommands/SystemCommands.plugin
Plugins/DosTools/DosTools.plugin
Plugins/TaskManager/TaskManager.plugin
Plugins/ClipboardTools/ClipboardTools.plugin
Plugins/DateUtilities/DateUtilities.plugin
Plugins/WindowSwitcher/WindowSwitcher.plugin
Plugins/DeveloperTools/DeveloperTools.plugin
Plugins/ARexxRunner/ARexxRunner.plugin
Plugins/ARexxRunner/ARexxRunner.cfg
Plugins/MusicControls/MusicControls.plugin
Plugins/MusicControls/MusicControls.cfg
scripts/Beam68Status.rexx
scripts/Install-Beam68
scripts/Run-Runtime-Test
scripts/Run-Music-Test
"

echo "Beam68: rebuilding classic AmigaOS binaries..."
"$project_root/scripts/build-amiga.sh" clean all

# Verify the complete set before modifying the destination.
for relative_path in $runtime_executables $runtime_data; do
    if [ ! -f "$project_root/$relative_path" ]; then
        echo "Beam68: missing build/package file: $relative_path" >&2
        exit 20
    fi
done

mkdir -p "$application_dir/Plugins" "$application_dir/scripts"

for relative_path in $runtime_executables $runtime_data; do
    mkdir -p "$application_dir/$(dirname "$relative_path")"
done

for relative_path in $runtime_executables; do
    cp -p "$project_root/$relative_path" "$application_dir/$relative_path"
    chmod 755 "$application_dir/$relative_path"
done

for relative_path in $runtime_data; do
    cp -p "$project_root/$relative_path" "$application_dir/$relative_path"
    chmod 644 "$application_dir/$relative_path"
done

echo "Beam68: staged AmigaOS 0.1 runtime package in:"
echo "$application_dir"
echo "With the amigaos-dev_020 profile, run Bin:Beam68/Beam68 or"
echo "CD Bin:Beam68 and Execute scripts/Install-Beam68."
