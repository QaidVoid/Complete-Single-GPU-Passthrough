#include <iommu.h>

void configure_iommu()
{
    check_vt_support();
    enable_iommu();
}

void check_vt_support()
{
    std::cout << "\033[1;36mChecking system for virtualisation support \033[0m";
    std::string res = shell_cmd("lscpu | grep Virtualization");

    if (res.empty())
        throw std::runtime_error("\n\033[1;31mVirtualisation is not enabled on your system!\033[0m");
    std::cout << "\033[1;32m\u2714\033[0m" << std::endl;
}

bool check_iommu()
{
    std::cout << "\033[1;36mIs IOMMU enabled? \033[0m";
    std::string res = shell_cmd("(dmesg | grep 'IOMMU enabled') 2>&1");

    if (res.find("IOMMU enabled") != std::string::npos)
        std::cout << "\033[1;32m\u2714\033[0m" << std::endl;
    else
    {
        std::cout << "\033[1;31m\u2718\033[0m" << std::endl;
        return false;
    }
    return true;
}

void enable_iommu()
{
    if (!check_iommu())
    {
        std::ifstream ifs("/etc/default/grub");
        std::ofstream ofs("./tmp/grub");

        if (ifs.fail())
            throw std::runtime_error("\033[1;31mGrubloader doesn't exist or I can't find it on your system.\033[0m");
        if (ofs.fail())
            throw std::runtime_error("\033[1;31mFailed to create temporary file.\033[0m");

        std::string line;
        while (getline(ifs, line))
        {
            std::istringstream iss(line);
            std::string key;
            if (std::getline(iss, key, '=') && key.rfind("#", 0) != 0)
            {
                std::string value;
                std::getline(iss, value);
                if (key == "GRUB_CMDLINE_LINUX_DEFAULT")
                {
                    if (get_cpu_vendor() == "intel")
                    {
                        if (value.find("intel_iommu=on") == std::string::npos)
                            value.insert(*value.cbegin(), " intel_iommu=on");
                    }
                    else if (value.find("amd_iommu=on") == std::string::npos)
                        value.insert(*value.cbegin(), " amd_iommu=on");
                    if (value.find("iommu=pt") == std::string::npos)
                        value.insert(*value.cbegin(), " iommu=pt");
                }
                ofs << key << "=" << value << '\n';
            }
            else
                ofs << line << '\n';
        }
        ifs.close();
        ofs.close();

        std::string res = shell_cmd("(mv -f ./tmp/grub /etc/default/grub) 2>&1");
        system("grub-mkconfig -o /boot/grub/grub.cfg");

        if (res.empty())
            throw std::runtime_error("\033[1;32mIOMMU enabled. Please reboot your system to continue.\033[0m");
        else
            throw std::runtime_error("\033[1;31mFailed to enable IOMMU.\n" + res + "\033[0m");
    }
}