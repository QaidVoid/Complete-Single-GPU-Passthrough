#include <iommu.h>
#include <tools.h>
#include <libvirt.h>

int main()
{
    try
    {
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