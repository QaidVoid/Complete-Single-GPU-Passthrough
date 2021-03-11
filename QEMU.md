## PLAIN QEMU WITHOUT LIBVIRT
### WORK IN PROGRESS

Passthrough can be done with plain qemu without using libvirt. \
The general idea of using plain qemu is to avoid managing libvirt configurations and rather use a single script.. \

Stuffs directly taken from YuriAlek's [script](https://gitlab.com/YuriAlek/vfio/-/blob/master/scripts/windows-basic.sh).

Create disk image with QEMU.
```sh
qemu-img create -f raw Disk.img 256G
```

The script should be run as superuser.

```sh
#!/bin/sh

ULIMIT=$(ulimit -a | grep "max locked memory" | awk '{print $6}')

# Replace 4 with the amount of RAM you supply to the VM (in GB)
ULIMIT_TARGET=$((4 * 1048576 + 100000))

ulimit -l $ULIMIT_TARGET

# Should be run before VM starts
function init_vm() {
	# Stop display manager (if any exists)
	# rc-service xdm stop
	# systemctl stop display-manager 
	pkill -9 dwm

	sleep 1
	# Unbind EFI-framebuffer
	echo efi-framebuffer.0 > /sys/bus/platform/drivers/efi-framebuffer/unbind
	
	# Unload GPU modules
	modprobe -r nvidia_drm nvidia_modeset nvidia_uvm nvidia snd_hda_intel
	# modprobe -r amdgpu 
	
	# Load vfio modules
	modprobe vfio-pci

	# Bind drivers to vfio
	echo "10de 1c03" > /sys/bus/pci/drivers/vfio-pci/new_id
	echo "0000:01:00.0" > /sys/bus/pci/devices/0000:01:00.0/driver/unbind
	echo "0000:01:00.0" > /sys/bus/pci/drivers/vfio-pci/bind
	echo "10de 1c03" > /sys/bus/pci/drivers/vfio-pci/remove_id

	echo "10de 10f1" > /sys/bus/pci/drivers/vfio-pci/new_id
	echo "0000:01:00.1" > /sys/bus/pci/devices/0000:01:00.1/driver/unbind
	echo "0000:01:00.1" > /sys/bus/pci/drivers/vfio-pci/bind
	echo "10de 10f1" > /sys/bus/pci/drivers/vfio-pci/remove_id
}

# Start VM
function start_vm() {
	qemu-system-x86_64 \
		-runas qaidvoid \
		-nodefaults \
		-enable-kvm \
		-machine kernel_irqchip=on \
		-cpu host,kvm=off,hv_vendor_id=nil \
		-m 4G \
		-name "Guix" \
		-smp cores=2 \
		-drive file=/home/qaidvoid/VM/Disk.img,if=virtio,format=raw \
		-drive file=/home/qaidvoid/Downloads/guix-system-install-1.2.0.x86_64-linux.iso,media=cdrom \
		-serial none \
		-net nic \
		-net user \
		-vga none \
		-device vfio-pci,host=01:00.0,x-vga=on,multifunction=on \
		-device vfio-pci,host=01:00.1 \
		-object input-linux,id=mouse1,evdev=/dev/input/by-id/usb-PixArt_HP_USB_Optical_Mouse-event-mouse \
		-object input-linux,id=kbd1,evdev=/dev/input/by-id/usb-Chicony_USB_Keyboard-event-kbd,grab_all=on,repeat=on \
		-device virtio-keyboard \
		-device virtio-mouse
}

# Should be run on VM shutdown
function stop_vm() {
	# Unload VFIO modules
	modprobe -r vfio-pci
	
	ulimit -l $ULIMIT

	# Reboot system, because I have no idea how to rebind drivers
	reboot

	# Load GPU modules
	modprobe nvidia_drm
	modprobe nvidia_modeset
	modprobe nvidia_uvm
	modprobe nvidia
	# modprobe amdgpu

	# Attach GPU to host ? driver doesn't exist ? 
	echo "0000:01:00.0" > /sys/bus/pci/devices/0000:01:00.0/driver/bind
	echo "0000:01:00.1" > /sys/bus/pci/devices/0000:01:00.1/driver/bind

	# Bind EFI framebuffer to host
	echo "efi-framebuffer.0" > /sys/bus/platform/drivers/efi-framebuffer/bind
	
	# Restart Display Manager
	# rc-service xdm start
	# systemctl start display-manager
	# su - qaidvoid -c startx
}

init_vm
start_vm
stop_vm
```
