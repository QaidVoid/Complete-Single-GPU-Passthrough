#include <shell.h>

std::string shell_cmd(char const *cmd)
{
    std::array<char, 128> buffer;
    std::string resp;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
        throw std::runtime_error("\n\033[1;31mThat was unexpected!\033[0m");

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        resp += buffer.data();
    }
    return resp;
}