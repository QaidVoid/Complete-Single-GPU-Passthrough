## Automated Setup
#### Install required tools
```sh
sudo pacman -S qemu libvirt edk2-ovmf virt-manager dnsmasq ebtables iptables
```


## Manual Setup
<b>Note</b>: Replace win10 with your virtual machine's name on libvirt hooks and virsh commands.

<details>
  <summary><b>Enable IOMMU</b></summary>

  ```sh
  sudo nano /etc/default/grub
  ```

  Append <b>iommu=pt</b>, and <b>intel_iommu=on</b> or <b>amd_iommu=on</b> kernel options to <b>GRUB_CMDLINE_LINUX_DEFAULT</b> for your CPU.
  Then, update grub configuration.
    
  ```sh
  sudo grub-mkconfig -o /boot/grub/grub.cfg
  ```
  <details>
    <summary><b>Verify IOMMU</b></summary>

    After setting kernel parameter on grub config, reboot your system and verify IOMMU is enabled.

  ```sh
  dmesg | grep 'IOMMU enabled'
  ```
  </details>
</details>

<details>
  <summary><b>Install required tools</b></summary>

  ```sh
  sudo pacman -S qemu libvirt edk2-ovmf virt-manager dnsmasq ebtables iptables
  ```
</details>

<details>
  <summary><b>Enable services</b></summary>
  Enable libvirtd service and start default network.

  ```sh
  sudo systemctl enable --now libvirtd
  sudo virsh net-start default
  sudo virsh net-autostart default
  ```
</details>

<details>
  <summary><b>Setup Guest OS</b></summary>
  
  Download [virtio](https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/stable-virtio/virtio-win.iso) driver. 

  Launch <b>virt-manager</b> and create new virtual machine. Check <b>Customize before install</b> on final step.
  
  <b>Overview</b> > Chipset: Q35, Firmware: UEFI<br/>
  <b>CPUs</b> > CPU model: host-passthrough, CPU topology: Best for your machine :)<br/>
  <b>SATA Disk</b> > Disk bus: virtio<br/>
  <b>NIC</b> > Device model: virtio<br/>
  <b>Add Hardware</b> > CDROM: virtio-win.iso
  
  <b>Begin Installation</b> and install Windows.. Windows can't detect virtio devices, so you need to <b>Load Driver</b> from <b>virtio-disk/amd64/win10</b> when prompted.

  After successful installation, install virtio-drivers from virtio CDROM.
</details>

<details>
  <summary><b>Attach PCI Devices</b></summary>
  Remove <b>Tablet, Spice Channel, Video XQL</b> and other applicable devices.
  
  <b>Add Hardware</b> > PCI Devices: GPU and HDMI audio
</details>

<details>
  <summary><b>NVIDIA GPU</b></summary>
  Nvidia GPU vBIOS requires patching to work. Though, some NVIDIA GPU works without patching.

  <details>
  <summary><b>Dump GPU vBIOS</b></summary>
  
  <b>Windows</b> > Use [GPU-Z](https://www.techpowerup.com/gpuz/)<br/>
  <b>Linux</b> > Try following but doesn't seem to work:

  ```sh
  su
  echo 1 > /sys/bus/pci/devices/0000:01:00.0/rom
  cat /sys/bus/pci/devices/0000:01:00.0/rom > vbios.rom
  echo 0 > /sys/bus/pci/devices/0000:01:00.0/rom
  ```

  Or, you can download vBIOS from [TechPowerUp](https://www.techpowerup.com/vgabios/), which might (not) be already patched.
  </details>
  
  <details>
    <summary><b>Patching vBIOS</b></summary>
    Use Hex Editor and search for string VIDEO, and remove everything before char <b>U</b>, i.e. HEX value 55.
  </details>

  <details>
  <summary><b>Using Patched vBIOS</b></summary>

  ```sh
  sudo virsh edit win10
  ```
  Search for <b>hostdev</b>. Add
  ```xml
  <rom file="path/to/your/patched_vbios.rom"/>
  ```
  before
  ```xml
  <address>
  ```
  </details>
</details>

<details>
  <summary><b>Libvirt Hooks</b></summary>
  <b>Libvirt hooks automate the process of running specific tasks during VM state change.</b>
  
  More info at: [PassthroughPost](https://passthroughpo.st/simple-per-vm-libvirt-hooks-with-the-vfio-tools-hook-helper/)

<details>
  <summary><b>Create Libvirt hook</b></summary>

  ```sh
  sudo mkdir /etc/libvirt/hooks
  sudo touch /etc/libvirt/hooks/qemu
  sudo chmod +x /etc/libvirt/hooks/qemu
  ```

  /etc/libvirt/hooks/qemu | 
---------- |
  ```sh
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
  ```

  <b>Restart libvirtd</b>
  ```sh
  sudo systemctl restart libvirtd
  ```
</details>

<details>
  <summary><b>Start Script</b></summary>

  ```sh
  sudo mkdir -p /etc/libvirt/hooks/qemu.d/win10/prepare/begin
  sudo touch /etc/libvirt/hooks/qemu.d/win10/prepare/begin/start.sh
  sudo chmod +x /etc/libvirt/hooks/qemu.d/win10/prepare/begin/start.sh
  ```
  
  /etc/libvirt/hooks/qemu.d/win10/prepare/begin/start.sh |
  -------------- |
  ```sh
  #!/bin/bash
  set -x
  
  # Stop display manager
  systemctl stop display-manager
  
  # Unbind EFI Framebuffer
  echo efi-framebuffer.0 > /sys/bus/platform/drivers/efi-framebuffer/unbind
  
  # Unload NVIDIA kernel modules
  modprobe -r nvidia_drm nvidia_modeset nvidia_uvm nvidia
  
  # Unload AMD kernel module
  # modprobe -r amdgpu
  
  # Detach GPU devices from host
  # Use your GPU and HDMI Audio PCI host device
  virsh nodedev-detach pci_0000_01_00_0
  virsh nodedev-detach pci_0000_01_00_1
  
  # Load vfio module
  modprobe vfio-pci
  ```
</details>

<details>

<summary><b>Stop Script</b></summary>

  ```sh
  sudo mkdir -p /etc/libvirt/hooks/qemu.d/win10/release/end
  sudo touch /etc/libvirt/hooks/qemu.d/win10/release/end/stop.sh
  sudo chmod +x /etc/libvirt/hooks/qemu.d/win10/release/end/stop.sh
```

/etc/libvirt/hooks/qemu.d/win10/release/end/stop.sh |
---------- |
  ```sh
  #!/bin/bash
  set -x
  
  # Unload vfio module
  modprobe -r vfio-pci
  
  # Attach GPU devices to host
  # Use your GPU and HDMI Audio PCI host device
  virsh nodedev-reattach pci_0000_01_00_0
  virsh nodedev-reattach pci_0000_01_00_1
  
  # Rebind framebuffer to host
  echo "efi-framebuffer.0" > /sys/bus/platform/drivers/efi-framebuffer/bind
  
  # Load NVIDIA kernel modules
  modprobe nvidia_drm
  modprobe nvidia_modeset
  modprobe nvidia_uvm
  modprobe nvidia
  
  # Load AMD kernel module
  # modprobe amdgpu
  
  # Restart Display Manager
  systemctl start display-manager
  ```
</details>
</details>

[Video card driver virtualisation detection](https://wiki.archlinux.org/index.php/PCI_passthrough_via_OVMF#Video_card_driver_virtualisation_detection)<br/>
[Keyboard/Mouse Passthrough](https://wiki.archlinux.org/index.php/PCI_passthrough_via_OVMF#Passing_keyboard/mouse_via_Evdev)<br/>
[Audio Passthrough](https://wiki.archlinux.org/index.php/PCI_passthrough_via_OVMF#Passing_VM_audio_to_host_via_PulseAudio)<br/>
[Troubleshooting](https://wiki.archlinux.org/index.php/PCI_passthrough_via_OVMF#Troubleshooting)

<b>See Also</b>
> [Single GPU Passthrough by joeknock90](https://github.com/joeknock90/Single-GPU-Passthrough)<br/>
> [Single GPU Passthrough by YuriAlek](https://gitlab.com/YuriAlek/vfio)<br/>
> [ArchLinux PCI Passthrough](https://wiki.archlinux.org/index.php/PCI_passthrough_via_OVMF)<br/>
> [Gentoo GPU Passthrough](https://wiki.gentoo.org/wiki/GPU_passthrough_with_libvirt_qemu_kvm)<br/>