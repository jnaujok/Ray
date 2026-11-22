#!/bin/sh
# Starts Ray's scene editor from the Linux release archive, after checking that the libraries it
# needs (GTK 3 and OpenGL) are installed and that there is a display to show it on. Without these
# checks a missing library stops the program before it can say anything ("error while loading
# shared libraries"), so this script looks first and says what to install.
#
# The editor itself is bin/ray-editor-bin beside this script. RAY_EDITOR_BIN overrides that (the
# tests use it).

here=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
editor=${RAY_EDITOR_BIN:-"$here/bin/ray-editor-bin"}

if [ ! -x "$editor" ]; then
    echo "ray-editor: can't find the editor program ($editor)." >&2
    exit 1
fi

# The libraries the dynamic loader can't find, one per line.
missing=$(ldd "$editor" 2>/dev/null | awk '/not found/ { print $1 }')

if [ -n "$missing" ]; then
    echo "ray-editor needs GTK 3 and OpenGL libraries that aren't installed:" >&2
    for library in $missing; do
        echo "    $library" >&2
    done
    echo "" >&2
    echo "Install them with your package manager, then run ray-editor again:" >&2
    echo "" >&2
    echo "  Debian / Ubuntu / Mint:  sudo apt install libgtk-3-0 libgl1 libglu1-mesa" >&2
    echo "  Fedora / RHEL 8 and up:  sudo dnf install gtk3 mesa-libGL mesa-libGLU" >&2
    echo "  RHEL 7 / CentOS 7:       sudo yum install gtk3 mesa-libGL mesa-libGLU" >&2
    echo "" >&2
    echo "(raythrower and rayengine need none of these.)" >&2
    exit 1
fi

if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
    echo "ray-editor: there is no display to open a window on (neither DISPLAY nor WAYLAND_DISPLAY" >&2
    echo "is set). Run it from a desktop session, or over SSH with X forwarding (ssh -X)." >&2
    echo "To render without a window, use raythrower." >&2
    exit 1
fi

exec "$editor" "$@"
