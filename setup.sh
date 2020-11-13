if [[ -z lspcu | grep Virtualization ]]; then
    echo "ERROR: Virtualization isn't supported in this system"
    exit 1
fi

if [[ -z dmesg | grep 'IOMMU enabled' ]]; then
    echo "IOMMU isn't enabled. Enabling IOMMU... "
    if [[ -f /boot/grub/grub.cfg ]]; then
        echo "Loading GRUB configuration.. "
        config=/etc/default/grub
        if [[ lscpu | grep GenuineIntel ]]; then
            if [[ -z sed -n 's/^GRUB_CMDLINE_LINUX_DEFAULT=//p' $config | grep intel_iommu=on ]]; then
                sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="\(.*\)"/GRUB_CMDLINE_LINUX_DEFAULT="\1 intel_iommu=on"/' $config
            fi
        else
            if [[ -z sed -n 's/^GRUB_CMDLINE_LINUX_DEFAULT=//p' $config | grep amd_iommu=on ]]; then
                sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="\(.*\)"/GRUB_CMDLINE_LINUX_DEFAULT="\1 amd_iommu=on"/' $config
        fi
    elif [[ -f /boot/loader/loader.conf ]]; then
        echo "Loading systemd-boot configuration... "
        echo "Add intel_iommu=on or amd_iommu=on depending on your CPU to options in boot entry.."
        # TODO
    fi
    echo "IOMMU enabled.."
    echo "Please reboot your system to continue.."
    exit 1
fi

echo "COPYING LIBVIRT SCRIPTS"
cp scripts/* /etc/libvirt/hooks/

echo "CONFIGURATION FINISHED.."
# TODO MORE OPTIONS
