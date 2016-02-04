// For conditions of distribution and use, see copyright notice in LICENSE

#include "TundraCoreApi.h"
#include "Framework.h"

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

using namespace Tundra;

int main(int argc, char **argv)
{
    #if defined(_MSC_VER) && defined(_DEBUG)
    // Memory leak debugging in MSVC debug mode
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif

    return run(argc, argv);
}


