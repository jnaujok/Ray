#!/bin/sh
# Checks the Linux release archive's ray-editor launcher (packaging/linux/ray-editor.sh):
# with a library missing it names the library and prints the install commands; with no
# display it says so; with everything there it runs the editor.
#
#   launcher_test.sh <launcher> <work directory>
#
# Needs a C compiler (cc) and ldd.

set -u
launcher=$1
work=$2
failures=0

fail() {
    echo "FAILED: $1" >&2
    failures=$((failures + 1))
}

mkdir -p "$work"
cd "$work" || exit 1

# A program linked against a library that is then removed, as if GTK weren't installed.
echo 'int fake_gtk_init(void) { return 0; }' > fakegtk.c
echo 'int fake_gtk_init(void); int main(void) { return fake_gtk_init(); }' > fakeeditor.c
cc -shared -fPIC -o libfakegtk-3.so.0 fakegtk.c -Wl,-soname,libfakegtk-3.so.0 || exit 1
cc -o fakeeditor fakeeditor.c ./libfakegtk-3.so.0 -Wl,-rpath,/nonexistent || exit 1
rm -f libfakegtk-3.so.0

output=$(DISPLAY=:0 RAY_EDITOR_BIN="$work/fakeeditor" sh "$launcher" 2>&1)
status=$?
[ $status -ne 0 ] || fail "a missing library should stop the launcher"
echo "$output" | grep -q "libfakegtk-3.so.0" || fail "the missing library should be named"
echo "$output" | grep -q "sudo apt install libgtk-3-0" || fail "the Debian command should be printed"
echo "$output" | grep -q "sudo dnf install gtk3" || fail "the Fedora command should be printed"
echo "$output" | grep -q "sudo yum install gtk3" || fail "the RHEL 7 command should be printed"

# Nothing missing but no display.
cp /bin/true "$work/trueeditor"
output=$(env -u DISPLAY -u WAYLAND_DISPLAY RAY_EDITOR_BIN="$work/trueeditor" sh "$launcher" 2>&1)
status=$?
[ $status -ne 0 ] || fail "no display should stop the launcher"
echo "$output" | grep -q "no display" || fail "no display should be reported"

# Everything there: the editor runs, with its arguments.
printf '#!/bin/sh\necho "ran with $*"\n' > "$work/okeditor"
chmod +x "$work/okeditor"
output=$(DISPLAY=:0 RAY_EDITOR_BIN="$work/okeditor" sh "$launcher" scene.ray 2>&1)
status=$?
[ $status -eq 0 ] || fail "with everything there the launcher should run the editor"
[ "$output" = "ran with scene.ray" ] || fail "the editor should get the launcher's arguments (got: $output)"

if [ $failures -ne 0 ]; then
    exit 1
fi
echo "launcher checks passed"
