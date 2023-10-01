## **Table Of Contents**
* **[IOMMU Setup](#enable--verify-iommu)**
* **[Installing Packages](#install-required-tools)**
* **[Enabling Services](#enable-required-services)**
* **[Guest Setup](#setup-guest-os)**
* **[Attching PCI Devices](#attaching-pci-devices)**
* **[Libvirt Hooks](#libvirt-hooks)**
* **[Keyboard/Mouse Passthrough](#keyboardmouse-passthrough)**
* **[Video Card Virtualisation Detection](#video-card-driver-virtualisation-detection)**
* **[Audio Passthrough](#audio-passthrough)**
* **[GPU vBIOS Patching](#vbios-patching)**

### **Enable & Verify IOMMU**
***BIOS Settings*** \
Enable ***Intel VT-d*** or ***AMD-Vi*** in BIOS settings. If these options are not present, it is likely that your hardware does not support IOMMU.

Disable ***Resizable BAR Support*** in BIOS settings. 
Cards that support Resizable BAR can cause problems with black screens following driver load if Resizable BAR is enabled in UEFI/BIOS. There doesn't seem to be a large performance penalty for disabling it, so turn it off for now until ReBAR support is available for KVM. 

***Set the kernel paramater depending on your CPU.*** \

For GRUB user, edit GRUB configuration
| /etc/default/grub |
| ----- |
| `GRUB_CMDLINE_LINUX_DEFAULT="... intel_iommu=on iommu=pt ..."` |
| OR |
| `GRUB_CMDLINE_LINUX_DEFAULT="... amd_iommu=on iommu=pt ..."` |

***Generate grub.cfg***
```sh
grub-mkconfig -o /boot/grub/grub.cfg
```

For systemd-boot user, edit this configuration
| /boot/loader/entries/*.conf |
| ----- |
| `options root=UUID=...intel_iommu=on iommu=pt..` |
| OR |
| `options root=UUID=...amd_iommu=on iommu=pt..` |

Unlike GRUB, systemd-boot doesn't require a separate command to regenerate the configuration. It automatically detects changes
Reboot your system for the changes to take effect.

***To verify IOMMU, run the following command.***
```sh
dmesg | grep IOMMU
```
The output should include the message `Intel-IOMMU: enabled` for Intel CPUs or `AMD-Vi: AMD IOMMUv2 loaded and initialized` for AMD CPUs.

To view the IOMMU groups and attached devices, run the following script:
```sh
#!/bin/bash
shopt -s nullglob
for g in `find /sys/kernel/iommu_groups/* -maxdepth 0 -type d | sort -V`; do
    echo "IOMMU Group ${g##*/}:"
    for d in $g/devices/*; do
        echo -e "\t$(lspci -nns ${d##*/})"
    done;
done;
```

When using passthrough, it is necessary to pass every device in the group that includes your GPU. \
You can avoid having to pass everything by using [ACS override patch](https://wiki.archlinux.org/title/PCI_passthrough_via_OVMF#Bypassing_the_IOMMU_groups_(ACS_override_patch)).

### **Install required tools**
<details>
  <summary><b>Gentoo Linux</b></summary>

  ```sh
  emerge -av qemu virt-manager libvirt ebtables dnsmasq
  ```
</details>

<details>
  <summary><b>Arch Linux</b></summary>

  ```sh
  pacman -S qemu libvirt edk2-ovmf virt-manager dnsmasq ebtables
  ```
</details>

<details>
  <summary><b>Fedora</b></summary>

  ```sh
  dnf install @virtualization
  ```
</details>

<details>
  <summary><b>Ubuntu</b></summary>

  ```sh
  apt install qemu-kvm qemu-utils libvirt-daemon-system libvirt-clients bridge-utils virt-manager ovmf
  ```
</details>

### **Enable required services**
<details>
  <summary><b>SystemD</b></summary>

  ```sh
  systemctl enable --now libvirtd
  ```
</details>

<details>
  <summary><b>OpenRC</b></summary>

  ```sh
  rc-update add libvirtd default
  rc-service libvirtd start
  ```
</details>

Sometimes, you might need to start default network manually.
```sh
virsh net-start default
virsh net-autostart default
```

### **Setup Guest OS**
***NOTE: You should replace win10 with your VM's name where applicable*** \
You should add your user to ***libvirt*** group to be able to run VM without root. And, ***input*** and ***kvm*** group for passing input devices.
```sh
usermod -aG kvm,input,libvirt username
```

Download [virtio](https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/stable-virtio/virtio-win.iso) driver. \
Launch ***virt-manager*** and create a new virtual machine. Select ***Customize before install*** on Final Step. \
In ***Overview*** section, set ***Chipset*** to ***Q35***, and ***Firmware*** to ***UEFI*** \
In ***CPUs*** section, set ***CPU model*** to ***host-passthrough***, and ***CPU Topology*** to whatever fits your system. \
For ***SATA*** disk of VM, set ***Disk Bus*** to ***virtio***. \
In ***NIC*** section, set ***Device Model*** to ***virtio*** \
Add Hardware > CDROM: virtio-win.iso \
Now, ***Begin Installation***. Windows can't detect the ***virtio disk***, so you need to ***Load Driver*** and select ***virtio-iso/amd64/win10*** when prompted. \
After successful installation of Windows, install virtio drivers from virtio CDROM. You can then remove virtio iso.

### **Attaching PCI devices**
Remove Channel Spice, Display Spice, Video QXL, Sound ich* and other unnecessary devices. \
Now, click on ***Add Hardware***, select ***PCI Devices*** and add the PCI Host devices for your GPU's VGA and HDMI Audio.

### **Libvirt Hooks**
Libvirt hooks automate the process of running specific tasks during VM state change. \
More info at: [PassthroughPost](https://passthroughpo.st/simple-per-vm-libvirt-hooks-with-the-vfio-tools-hook-helper/)

**Note**: Comment Unbind/rebind EFI framebuffer line from start and stop script if you're using AMD 6000 series cards (https://github.com/QaidVoid/Complete-Single-GPU-Passthrough/issues/9).
Also, move the line to unload AMD kernal module below detaching devices from host. These might also apply to older AMD cards.

<details>
  <summary><b>Create Libvirt Hook</b></summary>

  ```sh
  mkdir /etc/libvirt/hooks
  touch /etc/libvirt/hooks/qemu
  chmod +x /etc/libvirt/hooks/qemu
  ```
  <table>
  <tr>
  <th>
    /etc/libvirt/hooks/qemu
  </th>
  </tr>

  <tr>
  <td>

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

  </td>
  </tr>
  </table>
</details>

<details>
  <summary><b>Create Start Script</b></summary>
  
  ```sh
  mkdir -p /etc/libvirt/hooks/qemu.d/win10/prepare/begin
  touch /etc/libvirt/hooks/qemu.d/win10/prepare/begin/start.sh
  chmod +x /etc/libvirt/hooks/qemu.d/win10/prepare/begin/start.sh
  ```
**Note**: If you're on KDE Plasma (Wayland), you need to terminate user services alongside display-manager (https://github.com/QaidVoid/Complete-Single-GPU-Passthrough/issues/31).

  <table>
  <tr>
  <th>
    /etc/libvirt/hooks/qemu.d/win10/prepare/begin/start.sh
  </th>
  </tr>

  <tr>
  <td>

  ```sh
#!/bin/bash
set -x

# Stop display manager
systemctl stop display-manager
# systemctl --user -M YOUR_USERNAME@ stop plasma*
      
# Unbind VTconsoles: might not be needed
echo 0 > /sys/class/vtconsole/vtcon0/bind
echo 0 > /sys/class/vtconsole/vtcon1/bind

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

  </td>
  </tr>
  </table>
</details>

<details>
  <summary><b>Create Stop Script</b></summary>

  ```sh
  mkdir -p /etc/libvirt/hooks/qemu.d/win10/release/end
  touch /etc/libvirt/hooks/qemu.d/win10/release/end/stop.sh
  chmod +x /etc/libvirt/hooks/qemu.d/win10/release/end/stop.sh
  ```
  <table>
  <tr>
  <th>
    /etc/libvirt/hooks/qemu.d/win10/release/end/stop.sh
  </th>
  </tr>

  <tr>
  <td>

  ```sh
#!/bin/bash
set -x

# Attach GPU devices to host
# Use your GPU and HDMI Audio PCI host device
virsh nodedev-reattach pci_0000_01_00_0
virsh nodedev-reattach pci_0000_01_00_1

# Unload vfio module
modprobe -r vfio-pci

# Load AMD kernel module
#modprobe amdgpu

# Rebind framebuffer to host
echo "efi-framebuffer.0" > /sys/bus/platform/drivers/efi-framebuffer/bind

# Load NVIDIA kernel modules
modprobe nvidia_drm
modprobe nvidia_modeset
modprobe nvidia_uvm
modprobe nvidia

# Bind VTconsoles: might not be needed
echo 1 > /sys/class/vtconsole/vtcon0/bind
echo 1 > /sys/class/vtconsole/vtcon1/bind

# Restart Display Manager
systemctl start display-manager
```

  </td>
  </tr>
  </table>
</details>

### **Keyboard/Mouse Passthrough**
In order to be able to use keyboard/mouse in the VM, you can either passthrough the USB Host device or use Evdev passthrough.

Using USB Host Device is simple, \
***Add Hardware*** > ***USB Host Device***, add your keyboard and mouse device.

For Evdev passthrough, follow these steps: \
Modify libvirt configuration of your VM. \
**Note**: Save only after adding keyboard and mouse devices or the changes gets lost. \
Change first line to:

<table>
<tr>
<th>
virsh edit win10
</th>
</tr>

<tr>
<td>

```xml
<domain type='kvm' xmlns:qemu='http://libvirt.org/schemas/domain/qemu/1.0'>
```

</td>
</tr>
</table>

Find your keyboard and mouse devices in ***/dev/input/by-id***. You'd generally use the devices ending with ***event-kbd*** and ***event-mouse***. And the devices in your configuration right before closing ***`</domain>`*** tag. \
Replace ***MOUSE_NAME*** and ***KEYBOARD_NAME*** with your device id.

<table>
<tr>
<th>
virsh edit win10
</th>
</tr>

<tr>
<td>

```xml
...
  <qemu:commandline>
    <qemu:arg value='-object'/>
    <qemu:arg value='input-linux,id=mouse1,evdev=/dev/input/by-id/MOUSE_NAME'/>
    <qemu:arg value='-object'/>
    <qemu:arg value='input-linux,id=kbd1,evdev=/dev/input/by-id/KEYBOARD_NAME,grab_all=on,repeat=on'/>
  </qemu:commandline>
</domain>
```

</td>
</tr>
</table>

You need to include these devices in your qemu config.
<table>
<tr>
<th>
/etc/libvirt/qemu.conf
</th>
</tr>

<tr>
<td>

```sh
...
user = "YOUR_USERNAME"
group = "kvm"
...
cgroup_device_acl = [
    "/dev/input/by-id/KEYBOARD_NAME",
    "/dev/input/by-id/MOUSE_NAME",
    "/dev/null", "/dev/full", "/dev/zero",
    "/dev/random", "/dev/urandom",
    "/dev/ptmx", "/dev/kvm", "/dev/kqemu",
    "/dev/rtc","/dev/hpet", "/dev/sev"
]
...
```

</td>
</tr>
</table>

Also, switch from PS/2 devices to virtio devices. Add the devices inside ***`<devices>`*** block
<table>
<tr>
<th>
virsh edit win10
</th>
</tr>

<tr>
<td>

```xml
...
<devices>
  ...
  <input type='mouse' bus='virtio'/>
  <input type='keyboard' bus='virtio'/>
  ...
</devices>
...
```

</td>
</tr>
</table>

### **Audio Passthrough**
VM's audio can be routed to the host. You need ***Pulseaudio***. It's hit or miss. \
You can also use [Scream](https://wiki.archlinux.org/index.php/PCI_passthrough_via_OVMF#Passing_VM_audio_to_host_via_Scream) instead of Pulseaudio. \
Modify the libvirt configuration of your VM.

<table>
<tr>
<th>
virsh edit win10
</th>
</tr>

<tr>
<td>

```xml
...
  <qemu:commandline>
    ...
    <qemu:arg value="-device"/>
    <qemu:arg value="ich9-intel-hda,bus=pcie.0,addr=0x1b"/>
    <qemu:arg value="-device"/>
    <qemu:arg value="hda-micro,audiodev=hda"/>
    <qemu:arg value="-audiodev"/>
    <qemu:arg value="pa,id=hda,server=/run/user/1000/pulse/native"/>
  </qemu:commandline>
</devices>
```

</td>
</tr>
</table>

### **Video card driver virtualisation detection**
Video Card drivers refuse to run in Virtual Machine, so you need to spoof Hyper-V Vendor ID.
<table>
<tr>
<th>
virsh edit win10
</th>
</tr>

<tr>
<td>

```xml
...
<features>
  ...
  <hyperv>
    ...
    <vendor_id state='on' value='whatever'/>
    ...
  </hyperv>
  ...
</features>
...
```

</td>
</tr>
</table>

NVIDIA guest drivers also require hiding the KVM CPU leaf:
<table>
<tr>
<th>
virsh edit win10
</th>
</tr>

<tr>
<td>

```xml
...
<features>
  ...
  <kvm>
    <hidden state='on'/>
  </kvm>
  ...
</features>
...
```

</td>
</tr>
</table>

### **vBIOS Patching**
***NOTE: You are only making changes on dumped ROM file. Your hardware is safe.*** \
While most of the GPU can be passed with stock vBIOS, some GPU requires vBIOS patching to passthrough. \
In order to patch vBIOS, you need to first dump the GPU vBIOS from your system. \
If you have Windows installed, you can use [GPU-Z](https://www.techpowerup.com/gpuz) to dump vBIOS. \
To dump vBIOS on Linux, you can use following command (replace PCI id with yours):
```sh
echo 1 > /sys/bus/pci/devices/0000:01:00.0/rom
cat /sys/bus/pci/devices/0000:01:00.0/rom > path/to/dump/vbios.rom
echo 0 > /sys/bus/pci/devices/0000:01:00.0/rom
```
If you're not in root shell, you should use the above commands with sudo as:
```sh
echo 1 | sudo tee /sys/bus/pci/devices/0000:01:00.0/rom
sudo cat /sys/bus/pci/devices/0000:01:00.0/rom > path/to/dump/vbios.rom
echo 0 | sudo tee /sys/bus/pci/devices/0000:01:00.0/rom
```
To patch vBIOS, you need to use Hex Editor (eg., [Okteta](https://utils.kde.org/projects/okteta)) and trim unnecessary header. \
For NVIDIA GPU, using hex editor, search string “VIDEO”, and remove everything before HEX value 55. \
I'm not sure about AMD, but the process should be similar.

To use patched vBIOS, edit VM's configuration to include patched vBIOS inside ***hostdev*** block of VGA

  <table>
  <tr>
  <th>
  virsh edit win10
  </th>
  </tr>

  <tr>
  <td>

  ```xml
  ...
  <hostdev mode='subsystem' type='pci' managed='yes'>
    <source>
      ...
    </source>
    <rom file='/home/me/patched.rom'/>
    ...
  </hostdev>
  ...
  ```

  </td>
  </tr>
  </table>

### **See Also**
> [Single GPU Passthrough Troubleshooting](https://docs.google.com/document/d/17Wh9_5HPqAx8HHk-p2bGlR0E-65TplkG18jvM98I7V8)<br/>
> [Single GPU Passthrough by joeknock90](https://github.com/joeknock90/Single-GPU-Passthrough)<br/>
> [Single GPU Passthrough by YuriAlek](https://gitlab.com/YuriAlek/vfio)<br/>
> [Single GPU Passthrough by wabulu](https://github.com/wabulu/Single-GPU-passthrough-amd-nvidia)<br/>
> [ArchLinux PCI Passthrough](https://wiki.archlinux.org/index.php/PCI_passthrough_via_OVMF)<br/>
> [Gentoo GPU Passthrough](https://wiki.gentoo.org/wiki/GPU_passthrough_with_libvirt_qemu_kvm)<br/>
