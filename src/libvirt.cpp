#include <libvirt.h>

void libvirt_hook()
{
    char cnf;
    std::cout << "\033[1;34mSetting up libvirt hooks. If you have already setup libvirt hooks, they'll be replaced. Continue? (y/n) \033[0m";
    std::cin >> cnf;
    if (tolower(cnf) != 'y')
        throw std::runtime_error("\033[1;31mEXITING!\033[0m");

    struct stat buf;
    if (stat(hooks_path, &buf) != 0)
        if (mkdir(hooks_path, 0755) == -1)
            throw std::runtime_error(strerror(errno));
    if (stat(qemu_dir, &buf) != 0)
        if (mkdir(qemu_dir, 0755) == -1)
            throw std::runtime_error(strerror(errno));
    std::ofstream file(qemu_hook);
    file << qemu_script;
    file.close();
    if (chmod(qemu_hook, 0755) == -1)
        throw std::runtime_error(strerror(errno));
    std::cout << "\033[1;32m/etc/libvirt/hooks/qemu \033[1;32m\u2714\033[0m \033[0m" << std::endl;
    hook_begin();
    hook_release();
    system("systemctl restart libvirtd");
}

void hook_begin()
{
    struct stat buf;
    if (stat(vm_path, &buf) != 0)
        if (mkdir(vm_path, 0755) == -1)
            throw std::runtime_error(strerror(errno));
    if (stat(vm_prepare, &buf) != 0)
        if (mkdir(vm_prepare, 0755) == -1)
            throw std::runtime_error(strerror(errno));
    if (stat(vm_begin, &buf) != 0)
        if (mkdir(vm_begin, 0755) == -1)
            throw std::runtime_error(strerror(errno));
    std::ofstream file(begin_sh);
    file << begin_script();
    file.close();
    if (chmod(begin_sh, 0755) == -1)
        throw std::runtime_error(strerror(errno));
    std::cout << "\033[1;32m/etc/libvirt/hooks/qemu.d/win10/prepare/begin/begin.sh \033[1;32m\u2714\033[0m \033[0m" << std::endl;
}

void hook_release()
{
    struct stat buf;
    if (stat(vm_path, &buf) != 0)
        if (mkdir(vm_path, 0755) == -1)
            throw std::runtime_error(strerror(errno));
    if (stat(vm_release, &buf) != 0)
        if (mkdir(vm_release, 0755) == -1)
            throw std::runtime_error(strerror(errno));
    if (stat(vm_end, &buf) != 0)
        if (mkdir(vm_end, 0755) == -1)
            throw std::runtime_error(strerror(errno));
    std::ofstream file(end_sh);
    file << end_script();
    file.close();
    if (chmod(end_sh, 0755) == -1)
        throw std::runtime_error(strerror(errno));
    std::cout << "\033[1;32m/etc/libvirt/hooks/qemu.d/win10/release/end/end.sh \033[1;32m\u2714\033[0m \033[0m" << std::endl;
}

std::string begin_script()
{
    std::string gpu = get_gpu_vendor();

    std::string modules;
    modules = gpu == "nvidia" ? modules = "nvidia_drm nvidia_modeset nvidia_uvm nvidia" : modules = "amdgpu";
    std::string cont = R"(#!/bin/bash
set -x

# Stop display manager
systemctl stop display-manager

# Unbind EFI Framebuffer
echo efi-framebuffer.0 > /sys/bus/platform/drivers/efi-framebuffer/unbind)";
    cont += "\n\nmodprobe -r " + modules;
    cont += "\nvirsh nodedev-detach " + get_vga_slot();
    cont += "\nvirsh nodedev-detach " + get_audio_slot();
    cont += "\n\n#Load vfio module\nmodprobe vfio-pci";
    return cont;
}

std::string end_script()
{
    std::string gpu = get_gpu_vendor();

    std::string modules = "amdgpu";
    if (gpu == "nvidia")
        modules = "nvidia_drm nvidia_modeset nvidia_uvm nvidia";
    std::string cont = R"(#!/bin/bash
set -x

# Unload vfio module
modprobe -r vfio-pci
)";
    cont += "\nvirsh nodedev-reattach " + get_vga_slot();
    cont += "\nvirsh nodedev-reattach " + get_audio_slot();
    cont += "\n\n#Unbind EFI Framebuffer\necho \"efi-framebuffer.0\" > /sys/bus/platform/drivers/efi-framebuffer/bind";
    cont += gpu == "nvidia" ? "\n\n# Load NVIDIA kernel modules\nmodprobe nvidia_drm\nmodprobe nvidia_modeset\nmodprobe nvidia_uvm\nmodprobe nvidia" : "\n\n# Load AMD kernel module\nmodprobe amdgpu";
    cont += "\n\n# Restart Display Manager\nsystemctl start display-manager";
    return cont;
}