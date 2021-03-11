## PLAIN QEMU WITHOUT LIBVIRT

Passthrough can be done with plain qemu without using libvirt. \
The general idea of using plain qemu is to avoid managing libvirt configurations and rather use a single script.. \
You'd still need libvirt daemon running because we're using virsh command. \

Create disk image with QEMU.
```sh
qemu-img create -f raw Disk.img 256G
```

The script should be run as superuser. \
It's possible to get it working as user but it's tricky to setup. \
Also, it should be run from TTY..

```sh
#!/bin/sh

# Should be run before VM starts
function init_vm() {
	# Stop display manager (if any exists)
	# rc-service xdm stop
	# systemctl stop display-manager 
	
	# Unbind EFI-framebuffer
	echo efi-framebuffer.0 > /sys/bus/platform/drivers/efi-framebuffer/unbind
	
	# Unload GPU modules
	modprobe -r nvidia_drm nvidia_modeset nvidia_uvm nvidia 
	# modprobe -r amdgpu 
	
	# Detach GPU from host
	virsh nodedev-detach pci_0000_01_00_0
	virsh nodedev-detach pci_0000_01_00_1
	
	# Load vfio modules
	modprobe vfio-pci
}

# Start VM
function start_vm() {
	qemu-system-x86_64 \
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
	
	# Attach GPU to host
	virsh nodedev-reattach pci_0000_01_00_0
	virsh nodedev-reattach pci_0000_01_00_1
	
	# Bind EFI framebuffer to host
	echo "efi-framebuffer.0" > /sys/bus/platform/drivers/efi-framebuffer/bind
	
	# Load GPU modules
	modprobe nvidia_drm
	modprobe nvidia_modeset
	modprobe nvidia_uvm
	modprobe nvidia
	# modprobe amdgpu
	
	# Restart Display Manager? Doesn't seem to work quite well
	# rc-service xdm start
	# systemctl start display-manager
  reboot
}

init_vm
start_vm
stop_vm

```
