/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   UiAPI.h
    @brief  Ui core API. */

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"
#include "FrameworkFwd.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/Ptr.h>

namespace Urho3D
{
    class UIElement;
    class Menu;
}

namespace Tundra
{

class MenuBar;

class TUNDRACORE_API UiAPI : public Object
{
    URHO3D_OBJECT(UiAPI, Object);

public:
    MenuBar* GetMenuBar() const;

private:
    friend class Framework;

    void Initialize(float time);

    void CreateMenuBar();
    void RelaseMenuBar();

    /// Constructor. Framework takes ownership of this object.
    /** @param fw Framework */
    explicit UiAPI(Framework *framework);
    ~UiAPI();

    Framework *framework_;
    SharedPtr<MenuBar> menuBar_;
};

}
