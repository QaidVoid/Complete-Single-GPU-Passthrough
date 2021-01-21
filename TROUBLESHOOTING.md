## **Troubleshoot your troubles**
### Where is the log?
The logs are stored in ***/var/log/libvirt/qemu/vm_name.log***, where vm_name is the name of the VM you're trying to boot. \
By reading the few lines at the bottom of the log file, you get general idea of what might be an issue.

### **warning: host doesn't support requested feature, what is this?**
This warning usually means that the feature guest is requesting is not available on host. \
After the warning message, on same line, you'll see something like MSR(490H).vmx-entry-load-perf-global-ctrl \
That's the feature it's missing, but I have no idea what most of them indicate.

#### How to fix it?
Make sure you've enabled ***Intel VT-d*** or ***AMD-Vi*** in BIOS Settings. \
If you've set ***BIOS*** as Firmware for Guest, change to ***UEFI***. \
In the cpu section in log file, you might see something like this: \
***-cpu Skylake,vme=on,ss=on,vmx=on,...***. Although, it works in most cases, it doesn't in some. \
Make sure you've set ***host-passthrough*** CPU model which'll produce ***-cpu host,...*** instead.

### **shutting down, reason=failed, but no actual error?**
If the VM fails without an error, SSH into your host machine, and run libvirt start script from SSH Client. \
If the script works without producing error, you'd get blank screen, and you can try starting VM from SSH Client. \
Both commands should be run as superuser.
```sh
sh /etc/libvirt/hooks/qemu.d/win10/prepare/begin/start.sh
virsh start win10
```

If your troubles aren't done yet, I don't have any other ideas.
