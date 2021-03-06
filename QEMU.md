## PLAIN QEMU WITHOUT LIBVIRT
#### DRAFT
Passthrough can be done with plain qemu without using libvirt. \
The general idea of using plain qemu is to avoid managing libvirt configurations and rather use a single script.. \
So, how can this be achieved? \
The script should be written in this order to achieve the same thing which is currently done by libvirt.. \

```sh
# SCRIPT TO RUN BEFORE VM LAUNCH - same as libvirt begin

# LAUNCH VM HERE USING QEMU

# SCRIPT TO RUN ON VM END - same as libvirt end
```
