#include <tools.h>

void install_tools()
{
    struct stat buf;
    if (stat("/etc/arch-release", &buf) == 0)
        system("pacman -S qemu libvirt edk2-ovmf virt-manager dnsmasq ebtables");
    else if (stat("/etc/gentoo-release", &buf) == 0)
        system("emerge -a qemu libvirt virt-manager dnsmasq ebtables");
    else if (stat("/etc/debian_version", &buf) == 0)
        system("apt install qemu-kvm libvirt-clients libvirt-daemon-system ovmf dnsmasq ebtables");
    else if (stat("/etc/fedora-release", &buf) == 0)
        system("dnf install @virtualization");
    else
        throw std::runtime_error("\n\033[1;31mUnsupported system. Install required tools manually.\033[0m");
}

void enable_services()
{
    std::string init_sys = get_init();
    if (init_sys == "systemd")
        system("systemctl enable --now libvirtd");
    else // Other init systems?
        std::cout << "Enable libvirtd service on your system manually..\n";
    std::string res = shell_cmd("virsh net-list --all | grep -i default");

    if (res.empty())
    {
        system("virsh net-start default");
        system("virsh net-autostart default");
    }
}

std::string get_init()
{
    std::ifstream bin("/sbin/init", std::ios::binary);
    std::string line;
    while (std::getline(bin, line))
    {
        if (line.find("/lib/systemd") != std::string::npos)
            return "systemd";
        else if (line.find("sysvinit") != std::string::npos)
            return "sysvinit";
        else if (line.find("upstart") != std::string::npos)
            return "upstart";
    }
    return "";
}

void user_mod()
{
    std::string res = shell_cmd("usermod -aG kvm,input,libvirt $(logname)");
    if (!res.empty())
    {
        std::string emsg = "\n\033[1;31m" + res + "\033[0m";
        throw std::runtime_error(emsg);
    }
}