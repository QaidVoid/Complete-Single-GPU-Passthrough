#!/bin/sh

INIT_SYSTEM="runit" # systemd, openrc, runit
GPU="nvidia" # nvidia, amd
DISPLAY_MANAGER="yes" # yes, no
WINDOW_MANAGER="niri" # required if not using display manager
START_WINDOW_MANAGER="dbus-run-session niri --session" # not sure if this works
GPU_PCI="pci_0000_01_00_0"
GPU_AUDIO_PCI="pci_0000_01_00_1"
VM_NAME="win10" # name of VM set during creation

mkdir -pv tmp

sudo_cmd="sudo"
if command -v doas >/dev/null; then
  sudo_cmd="doas"
fi

start_service() {
    case $INIT_SYSTEM in
        "systemd") echo "systemctl start $1";;
        "openrc") echo "rc-service $1 start";;
        "runit") echo "sv up $1";;
    esac
}

stop_service() {
    case $INIT_SYSTEM in
        "systemd") echo "systemctl stop $1";;
        "openrc") echo "openrc $1 stop";;
        "runit") echo "sv down $1";;
    esac
}

cat > ./tmp/qemu_hook<< "EOF"
#!/bin/bash

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
EOF

if [ "$DISPLAY_MANAGER" = "yes" ]; then
  # TODO: use service provided by package if display-manager is not valid service
  dm_start=$(start_service "display-manager")
  dm_stop=$(stop_service "display-manager")

  if [ "$INIT_SYSTEM" = "systemd" ]; then
    plasma_stop="systemctl --user -M $USER@ stop plasma"
  fi
else
  dm_stop="pkill -x $WINDOW_MANAGER"
  dm_start="su -l $USER -c $START_WINDOW_MANAGER"
fi

if [ "$GPU" = "amd" ]; then
  gpu_modules="amdgpu"
elif [ "$GPU" = "nvidia" ]; then
  gpu_modules="nvidia_drm nvidia_modeset nvidia_uvm nvidia"
fi

cat > ./tmp/libvirt_hook_start.sh<< EOF
#!/bin/bash
set -x

$dm_stop
$plasma_stop

# Unbind VTconsoles
echo 0 > /sys/class/vtconsole/vtcon0/bind
echo 0 > /sys/class/vtconsole/vtcon1/bind

# Unbind EFI Framebuffer
echo efi-framebuffer.0 > /sys/bus/platform/drivers/efi-framebuffer/unbind

modprobe -r $gpu_modules

virsh nodedev-detach $GPU_PCI
virsh nodedev-detach $GPU_AUDIO_PCI

modprobe vfio-pci
EOF

cat > ./tmp/libvirt_hook_stop.sh<< EOF
#!/bin/bash
set -x

# Attach GPU devices to host
virsh nodedev-reattach $GPU_PCI
virsh nodedev-reattach $GPU_AUDIO_PCI

# Unload vfio module
modprobe -r vfio-pci

# Rebind framebuffer to host
echo "efi-framebuffer.0" > /sys/bus/platform/drivers/efi-framebuffer/bind

# Load NVIDIA kernel modules
$(for module in $gpu_modules; do
  echo "modprobe $module"
done)

# Bind VTconsoles
echo 1 > /sys/class/vtconsole/vtcon0/bind
echo 1 > /sys/class/vtconsole/vtcon1/bind

$dm_start
EOF

get_yes_no_input() {
    while true; do
        echo -n "$1 (yes/no): "
        read yn
        case $yn in
            [Yy]* ) return 0;;
            [Nn]* ) return 1;;
            * ) echo "Please answer yes or no.";;
        esac
    done
}

chmod +x -v tmp/qemu_hook
chmod +x -v tmp/libvirt_hook_start.sh
chmod +x -v tmp/libvirt_hook_stop.sh

if get_yes_no_input "Want me to copy hooks files?"; then
  $sudo_cmd mkdir -pv /etc/libvirt/hooks
  $sudo_cmd mkdir -pv /etc/libvirt/hooks/qemu.d/$VM_NAME/prepare/begin
  $sudo_cmd mkdir -pv /etc/libvirt/hooks/qemu.d/$VM_NAME/release/end

  $sudo_cmd cp -v --preserve ./tmp/qemu_hook /etc/libvirt/hooks/qemu
  $sudo_cmd cp -v --preserve ./tmp/libvirt_hook_start.sh /etc/libvirt/hooks/qemu.d/$VM_NAME/prepare/begin/start.sh
  $sudo_cmd cp -v --preserve ./tmp/libvirt_hook_stop.sh /etc/libvirt/hooks/qemu.d/$VM_NAME/release/end/stop.sh
  echo "\033[1;32m✓ DONE \033[0m"
else
  echo "\033[1;32m✓ You can manually copy hooks from tmp directory.\033[0m"
fi
