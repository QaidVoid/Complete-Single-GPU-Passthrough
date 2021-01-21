## **Troubleshoot your troubles**
### Where is the log?
The logs are stored in /var/libvirt/qemu/vm_name.log, where vm_name is the name of the VM you're trying to boot. \
By reading the few lines at the bottom of the log file, you get general idea of what might be an issue.

### warning: host doesn't support requested feature, what is this?
This warning usually means that the feature guest is requesting is not available on host. \
After the warning message, on same line, you'll see something like MSR(490H).vmx-entry-load-perf-global-ctrl \
That's the feature it's missing, but I have no idea what most of them indicate. \
If you've set Firmware for VM to BIOS, change to UEFI. \
In the cpu section in log file, you might see something like this: \
***-cpu Skylake,vme=on,ss=on,vmx=on,...*** \
Make sure you've set ***host-passthrough*** CPU model which'll produce ***-cpu host,...*** instead. \
If it doesn't, I don't have any other ideas.
