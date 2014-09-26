// For conditions of distribution and use, see copyright notice in LICENSE

#include "TundraCoreApi.h"
#include "Framework.h"

#if defined(WIN32) // Windows application entry point.
#include <Windows.h>
#include <string>
#include <vector>

using namespace Tundra;

int WINAPI WinMain(_In_ HINSTANCE /*hInstance*/, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR lpCmdLine, _In_ int /*nShowCmd*/){
    std::string cmdLine(lpCmdLine);

    // Parse the Windows command line.
    std::vector<std::string> arguments;
    size_t i;
    size_t cmdStart = 0;
    size_t cmdEnd = 0;
    bool cmd = false;
    bool quote = false;

    // Inject executable name as Framework will expect it to be there.
    // Otherwise the first param will be ignored (it assumes its the executable name).
    // In WinMain() its not included in the 'lpCmdLine' param.
    arguments.push_back("Tundra.exe");

    for(i = 0; i < cmdLine.length(); ++i)
    {
        if (cmdLine[i] == '\"')
            quote = !quote;
        if ((cmdLine[i] == ' ') && (!quote))
        {
            if (cmd)
            {
                cmd = false;
                cmdEnd = i;
                arguments.push_back(cmdLine.substr(cmdStart, cmdEnd-cmdStart));
            }
        }
        else
        {
            if (!cmd)
            {
               cmd = true;
               cmdStart = i;
            }
        }
    }
    if (cmd)
        arguments.push_back(cmdLine.substr(cmdStart, i-cmdStart));

    std::vector<const char*> argv;
    for(i = 0; i < arguments.size(); ++i)
        argv.push_back(arguments[i].c_str());

    if (argv.size())
        return run((int)argv.size(), (char**)&argv[0]);
    else
        return run(0, 0);
}
#elif defined(ANDROID)
// Android entrypoint (use SDL_Main, which the Urho3D library will call)
extern "C" int SDL_main(int argc, char** argv);
int SDL_main(int argc, char** argv)
{
    return Tundra::run(argc, argv);
}
#else
// Unix entrypoint
int main(int argc, char **argv)
{
    return Tundra::run(argc, argv);
}
#endif
