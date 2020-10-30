#include <libvirt.h>

void libvirt_hook()
{
    struct stat buf;
    if (stat(hooks_path, &buf) != 0)
        if (mkdir(hooks_path, 0755) == -1)
            throw std::runtime_error(strerror(errno));
    if (stat(qemu_hook, &buf) != 0)
    {
        std::ofstream file(qemu_hook);
        file << qemu_script;
        file.close();
        if (chmod(qemu_hook, 0755) == -1)
            throw std::runtime_error(strerror(errno));
    }
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
    if (stat(begin_sh, &buf) != 0)
    {
        std::ofstream file(begin_sh);
        file << "#BEGIN";
        file.close();
        if (chmod(begin_sh, 0755) == -1)
            throw std::runtime_error(strerror(errno));
    }
}

void hook_release()
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
    if (stat(end_sh, &buf) != 0)
    {
        std::ofstream file(end_sh);
        file << "#END";
        file.close();
        if (chmod(end_sh, 0755) == -1)
            throw std::runtime_error(strerror(errno));
    }
}

std::string begin_script()
{
    return "";
}

std::string end_script()
{
    return "";
}