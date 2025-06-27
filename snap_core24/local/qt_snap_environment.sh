#!/bin/bash

# Save off parameters before any manipulation
original_args=("$@")

function append_dir() {
  local -n var="$1"
  local dir="$2"
  # We can't check if the dir exists when the dir contains variables
  if [[ "$dir" == *"\$"*  || -d "$dir" ]]; then
    export "${!var}=${var:+$var:}${dir}"
  fi
}

function ensure_dir_exists() {
  [ -d "$1" ] ||  mkdir -p "$@"
}

if [ "$SNAP_ARCH" = "amd64" ]; then
  export ARCH="x86_64-linux-gnu"
elif [ "$SNAP_ARCH" = "arm64" ]; then
  export ARCH="aarch64-linux-gnu"
fi

# Add QT_PLUGIN_PATH (Qt Modules).
append_dir QT_PLUGIN_PATH "$SNAP/usr/lib/$ARCH/qt5/plugins"
append_dir QT_PLUGIN_PATH "$SNAP/usr/lib/$ARCH"
# And QML2_IMPORT_PATH (Qt Modules).
append_dir QML2_IMPORT_PATH "$SNAP/usr/lib/$ARCH/qt5/qml"
append_dir QML2_IMPORT_PATH "$SNAP/lib/$ARCH"

# Fix locating the QtWebEngineProcess executable
export QTWEBENGINEPROCESS_PATH="$SNAP/usr/lib/$ARCH/qt5/libexec/QtWebEngineProcess"

# Removes Qt warning: Could not find a location
# of the system Compose files
export QTCOMPOSE="$SNAP/usr/share/X11/locale"
export QT_XKB_CONFIG_ROOT="/usr/share/X11/xkb"

export XDG_CONFIG_HOME="$SNAP_USER_DATA/.config"
ensure_dir_exists "$XDG_CONFIG_HOME"
export XDG_DATA_HOME="$SNAP_USER_DATA/.local/share"
ensure_dir_exists "$XDG_DATA_HOME"
export XDG_CACHE_HOME="$SNAP_USER_DATA/.cache"
ensure_dir_exists "$XDG_CACHE_HOME"

# Ensure QtChooser behaves.
export QTCHOOSER_NO_GLOBAL_DIR=1
export QT_SELECT=5
# qtchooser hardcodes reference paths, we'll need to rewrite them properly
ensure_dir_exists "$XDG_CONFIG_HOME/qtchooser"
echo "$SNAP/usr/lib/qt5/bin" > "$XDG_CONFIG_HOME/qtchooser/5.conf"
echo "$SNAP/usr/lib/$ARCH" >> "$XDG_CONFIG_HOME/qtchooser/5.conf"
echo "$SNAP/usr/lib/qt5/bin" > "$XDG_CONFIG_HOME/qtchooser/default.conf"
echo "$SNAP/usr/lib/$ARCH" >> "$XDG_CONFIG_HOME/qtchooser/default.conf"

# This relies on qtbase patch
# 0001-let-qlibraryinfo-fall-back-to-locate-qt.conf-via-XDG.patch
# to make QLibraryInfo look in XDG_* locations for qt.conf. The paths configured
# here are applied to everything that uses QLibraryInfo as final fallback and
# has no XDG_* fallback before that. Currently the most interesting offender
# is QtWebEngine which will not work unless the Data path is correctly set.
cat << EOF > "$XDG_CONFIG_HOME/qt.conf"
[Paths]
Data = $SNAP/usr/share/qt5/
Translations = $SNAP/usr/share/qt5/translations
EOF

# Remove the Qt: Session management error: Could not open network socket
export -n SESSION_MANAGER

# If set by the user that launched the app, it prevents GUI from launching
unset DBUS_SESSION_BUS_ADDRESS

exec "${original_args[@]}"
