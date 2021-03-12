## PLAIN QEMU WITHOUT LIBVIRT
### WORK IN PROGRESS
### ONLY USE THIS IF YOU KNOW WHAT YOU'RE DOING

I don't use VMs so I didn't test it extensively. \
Only tested it for if it actually passes GPU, and it seems to do the job. \
You need to figure out missing pieces on your own.

Passthrough can be done with plain qemu without using libvirt. \
The general idea of using plain qemu is to avoid managing libvirt configurations and rather use a single portable script.

Stuffs taken from YuriAlek's [script](https://gitlab.com/YuriAlek/vfio/-/blob/master/scripts/windows-basic.sh).

Create disk image with QEMU.
```sh
qemu-img create -f raw Disk.img 256G
```

The script should be run as superuser. \
Run with nohup to make the script run even after terminal closes.
```sh
doas nohup ./passthrough.sh > ~/qemu.log 2>&1
```

Use ``lspci -nn`` command to get device ids.
```
01:00.0 VGA compatible controller [0300]: NVIDIA Corporation GP106 [GeForce GTX 1060 6GB] [10de:1c03] (rev a1)
01:00.1 Audio device [0403]: NVIDIA Corporation GP106 High Definition Audio Controller [10de:10f1] (rev a1)
```

```sh
#!/bin/sh

ULIMIT=$(ulimit -a | grep "max locked memory" | awk '{print $6}')

# Replace 4 with the amount of RAM you supply to the VM (in GB)
ULIMIT_TARGET=$((4 * 1048576 + 100000))

# 

VIDEOID="10de 1c03"
VIDEOBUSID="0000:01:00.0"
AUDIOID="10de 10f1"
AUDIOBUSID="0000:01:00.1"

ulimit -l $ULIMIT_TARGET

# Should be run before VM starts
function init_vm() {
	# Stop display manager (if any exists)
	# rc-service xdm stop
	# systemctl stop display-manager 
	pkill -9 emacs 

	sleep 1
	# Unbind EFI-framebuffer
	echo efi-framebuffer.0 > /sys/bus/platform/drivers/efi-framebuffer/unbind
	
	# Unload GPU modules
	modprobe -r nvidia_drm nvidia_modeset nvidia_uvm nvidia snd_hda_intel
	# modprobe -r amdgpu 
	
	# Load vfio modules
	modprobe vfio-pci

	# Bind drivers to vfio
	echo $VIDEOID > /sys/bus/pci/drivers/vfio-pci/new_id
	echo $VIDEOBUSID > /sys/bus/pci/devices/0000:01:00.0/driver/unbind
	echo $VIDEOBUSID > /sys/bus/pci/drivers/vfio-pci/bind
	echo $VIDEOID > /sys/bus/pci/drivers/vfio-pci/remove_id

	echo $AUDIOID > /sys/bus/pci/drivers/vfio-pci/new_id
	echo $AUDIOBUSID > /sys/bus/pci/devices/0000:01:00.1/driver/unbind
	echo $AUDIOBUSID > /sys/bus/pci/drivers/vfio-pci/bind
	echo $AUDIOID > /sys/bus/pci/drivers/vfio-pci/remove_id
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
		-nographic \
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

	# Load GPU modules
	modprobe nvidia_drm
	modprobe nvidia_modeset
	modprobe nvidia_uvm
	modprobe nvidia
	# modprobe amdgpu
	
	# Bind EFI framebuffer to host
	echo "efi-framebuffer.0" > /sys/bus/platform/drivers/efi-framebuffer/bind
	
	# Restart Display Manager - I don't have display manager to test
	# rc-service xdm start
	# systemctl start display-manager

	# Couldn't get display back on host..
	# Rebooting seems to be the best option
	reboot
}

init_vm
start_vm
stop_vm
```
