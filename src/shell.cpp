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
    trim(resp);
    return resp;
}

void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}

void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(),
            s.end());
}

void trim(std::string &s)
{
    ltrim(s);
    rtrim(s);
}