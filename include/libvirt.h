#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <bits/stdc++.h>
#include <sysinfo.h>

void libvirt_hook();
void hook_begin();
void hook_release();
std::string begin_script();
std::string end_script();

inline const char *hooks_path = "/etc/libvirt/hooks";
inline const char *qemu_hook = "/etc/libvirt/hooks/qemu";
inline const char *vm_path = "/etc/libvirt/hooks/qemu.d/win10";

inline const char *vm_prepare = "/etc/libvirt/hooks/qemu.d/win10/prepare";
inline const char *vm_begin = "/etc/libvirt/hooks/qemu.d/win10/prepare/begin";
inline const char *begin_sh = "/etc/libvirt/hooks/qemu.d/win10/prepare/begin/begin.sh";

inline const char *vm_release = "/etc/libvirt/hooks/qemu.d/win10/release";
inline const char *vm_end = "/etc/libvirt/hooks/qemu.d/win10/release/end";
inline const char *end_sh = "/etc/libvirt/hooks/qemu.d/win10/release/end/end.sh";

inline const std::string qemu_script = R"#(#!/bin/bash

GUEST_NAME="$1"
HOOK_NAME="$2"
STATE_NAME="$3"
MISC="${@:4}"

BASEDIR="$(dirname $0)"

HOOKPATH="$BASEDIR/qemu.d/$GUEST_NAME/$HOOK_NAME/$STATE_NAME"
set -e # If a script exits with an error, we should as well.

if [ -f "$HOOKPATH" ]; then
  eval \""$HOOKPATH"\" "$@"
elif [ -d "$HOOKPATH" ]; then
  while read file; do
    eval \""$file"\" "$@"
  done <<< "$(find -L "$HOOKPATH" -maxdepth 1 -type f -executable -print;)"
fi
)#";