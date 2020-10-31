#include <iommu.h>
#include <tools.h>
#include <libvirt.h>

int main()
{
    try
    {
        std::string user = shell_cmd("echo $(whoami)");
        if (user != "root")
            throw std::runtime_error("\033[1;31mYou need to run the tool as root.\033[0m");

        char cnf;
        configure_iommu();

        std::cout << "Do you want to install required packages automatically? (y/n) ";
        std::cin >> cnf;
        if (tolower(cnf) == 'y')
            install_tools();
        enable_services();
        user_mod();
        libvirt_hook();
    }
    catch (const std::runtime_error &err)
    {
        std::cerr << err.what() << std::endl;
    }
}