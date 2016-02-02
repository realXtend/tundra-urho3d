#include "BindingsHelpers.h"

namespace Bindings
{

void SetObject(duk_context* ctx, duk_idx_t stackIndex, void* obj, const char* typeName)
{
    if (stackIndex < 0)
        --stackIndex;
    duk_push_pointer(ctx, obj);
    duk_put_prop_string(ctx, stackIndex, "\xff""obj");
    duk_push_pointer(ctx, (void*)typeName);
    duk_put_prop_string(ctx, stackIndex, "\xff""type");
}

void DefineProperty(duk_context* ctx, const char* propertyName, duk_c_function getFunc, duk_c_function setFunc)
{
    duk_push_string(ctx, propertyName);
    if (setFunc)
    {
        duk_push_c_function(ctx, getFunc, 0);
        duk_push_c_function(ctx, setFunc, 1);
        duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER);
    }
    else
    {
        duk_push_c_function(ctx, getFunc, 0);
        duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER);
    }
}

}