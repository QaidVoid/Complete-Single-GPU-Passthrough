#include <sysinfo.h>

std::string get_cpu_vendor()
{
    std::string cpu = "intel";
    std::string res = shell_cmd("lspci | grep -i amd");

    if (!res.empty())
        cpu = "amd";
    return cpu;
}

std::string get_gpu_vendor()
{
    std::string gpu = "amd";
    std::string res = shell_cmd("lspci | grep -i nvidia");

    if (!res.empty())
        gpu = "nvidia";
    return gpu;
}

std::string get_vga_slot()
{
    std::string c = "lspci | grep -i vga.*" + get_gpu_vendor();
    std::string res = shell_cmd(c.c_str());

    if (res.empty())
        throw std::runtime_error("\n\033[1;31mCouldn't fetch VGA PCI slot!\033[0m");
    std::replace(res.begin(), res.end(), ':', '_');
    return "pci_0000_" + res.substr(0, res.find(" "));
}

std::string get_audio_slot()
{
    std::string c = "lspci | grep -i audio.*" + get_gpu_vendor();
    std::string res = shell_cmd(c.c_str());

    if (res.empty())
        throw std::runtime_error("\n\033[1;31mCouldn't fetch Audio PCI slot!\033[0m");
    std::replace(res.begin(), res.end(), ':', '_');
    return "pci_0000_" + res.substr(0, res.find(" "));
}