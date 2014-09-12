// For conditions of distribution and use, see copyright notice in LICENSE

#include "TundraCoreApi.h"

#include <string>
#include <vector>

int TUNDRACORE_API run(int argc, char **argv);

#if defined(WIN32) // Windows application entry point.
#include <Windows.h>

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int /*nShowCmd*/){
    std::string cmdLine(lpCmdLine);

    // Parse the Windows command line.
    std::vector<std::string> arguments;
    unsigned i;
    unsigned cmdStart = 0;
    unsigned cmdEnd = 0;
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
    for(size_t i = 0; i < arguments.size(); ++i)
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
	return run(argc, argv);
}
#else
// Unix entrypoint
int main(int argc, char **argv)
{
    return run(argc, argv);
}
#endif

