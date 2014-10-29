// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginApi.h"
#include "HttpPluginFwd.h"
#include "FrameworkFwd.h"

#include "HttpWorkQueue.h"

#include <Engine/Container/RefCounted.h>

namespace Tundra
{

/// HTTP client
class TUNDRA_HTTP_API HttpClient : public Urho3D::RefCounted
{   
public:
    HttpClient(Framework *framework);
    ~HttpClient();

    HttpRequestPtr Get(const String &url);

private:
    friend class HttpPlugin;
    void Update(float frametime);

    Framework *framework_;
    HttpWorkQueuePtr queue_;    
};

}
