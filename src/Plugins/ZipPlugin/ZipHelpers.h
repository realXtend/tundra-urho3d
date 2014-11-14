// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "LoggingFunctions.h"

#include <zzip/zzip.h>

namespace Tundra
{

static bool CheckAndLogZzipError(zzip_error_t error)
{
    String errorMsg;
    switch (error)
    {
        case ZZIP_NO_ERROR:
            break;
        case ZZIP_OUTOFMEM:
            errorMsg = "Out of memory.";
            break;            
        case ZZIP_DIR_OPEN:
        case ZZIP_DIR_STAT: 
        case ZZIP_DIR_SEEK:
        case ZZIP_DIR_READ:
            errorMsg = "Unable to read zip file.";
            break;            
        case ZZIP_UNSUPP_COMPR:
            errorMsg = "Unsupported compression format.";
            break;            
        case ZZIP_CORRUPTED:
            errorMsg = "Corrupted archive.";
            break;            
        default:
            errorMsg = "Unknown error.";
            break;            
    };

    if (!errorMsg.Empty())
        LogError("[ZipAssetBundle] " + errorMsg);
    return !errorMsg.Empty();
}

static bool CheckAndLogArchiveError(ZZIP_DIR *archive)
{
    if (archive)
        return CheckAndLogZzipError((zzip_error_t)zzip_error(archive));
    return true;
}

}
