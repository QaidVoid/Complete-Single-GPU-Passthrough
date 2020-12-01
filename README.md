### **Enable IOMMU**
#### ***Set the kernel paramater depending on your CPU.***
#### For GRUB user, edit grub configuration.
| /etc/default/grub |
| ----- |
| `GRUB_CMDLINE_LINUX_DEFAULT="intel_iommu=on iommu=pt ..."` |
| OR |
| `GRUB_CMDLINE_LINUX_DEFAULT="amd_iommu=on iommu=pt ..."` |
#### ***Generate grub.cfg***
```sh
grub-mkconfig -o /boot/grub/grub.cfg
```
#### Reboot your system for the changes to take effect.

### **Verify IOMMU**
#### ***If you don't see any output when running following command, IOMMU is not functioning.***
```sh
dmesg | grep 'IOMMU enabled'
```

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

#### Sometimes, you might need to start default network manually.
```sh
virsh net-start default
virsh net-autostart default
```

### **Setup Guest OS**
### ***NOTE: You should replace win10 with your VM's name where applicable***
#### ***Download [virtio](https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/stable-virtio/virtio-win.iso) driver.***
#### Launch ***virt-manager*** and create a new virtual machine. Select ***Customize before install*** on Final Step.
#### In ***Overview*** section, set ***Chipset*** to ***Q35***, and ***Firmware*** to ***UEFI***
#### In ***CPUs*** section, set ***CPU model*** to ***host-passthrough***, and ***CPU Topology*** to whatever fits your system.
#### For ***SATA*** disk of VM, set ***Disk Bus*** to ***virtio***.
#### In ***NIC*** section, set ***Device Model*** to ***virtio***
#### Add Hardware > CDROM: virtio-win.iso
#### Now, ***Begin Installation***. Windows can't detect the ***virtio disk***, so you need to ***Load Driver*** and select ***virtio-iso/amd64/win10*** when prompted.
#### After successful installation of Windows, install virtio drivers from virtio CDROM.

### **Attaching PCI devices**
#### Remove Channel Spice, Display Spice, Video XQL, Sound ich* and other unnecessary devices.
#### Now, click on ***Add Hardware***, select ***PCI Devices*** and add the PCI Host devices for your GPU's VGA and HDMI Audio
#### Some GPU vBIOS needs to be patched for UEFI Support.
----- TODO: vBIOS patching ------
#### To use patched vBIOS, edit VM's configuration
```sh
virsh edit win10
```
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

<b>See Also</b>
> [Single GPU Passthrough by joeknock90](https://github.com/joeknock90/Single-GPU-Passthrough)<br/>
> [Single GPU Passthrough by YuriAlek](https://gitlab.com/YuriAlek/vfio)<br/>
> [ArchLinux PCI Passthrough](https://wiki.archlinux.org/index.php/PCI_passthrough_via_OVMF)<br/>
> [Gentoo GPU Passthrough](https://wiki.gentoo.org/wiki/GPU_passthrough_with_libvirt_qemu_kvm)<br/>
