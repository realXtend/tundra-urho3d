// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"
#include "UrhoModuleApi.h"

#include <Urho3D/Container/HashMap.h>
#include <Urho3D/Math/StringHash.h>
#include <Urho3D/Math/Color.h>

/// @cond PRIVATE

namespace Tundra
{

namespace Ogre
{

namespace Material
{
    namespace Block
    {
        const StringHash Material                   = "material";
        const StringHash Technique                  = "technique";
        const StringHash Pass                       = "pass";
        const StringHash VertexProgram              = "vertex_program_ref";
        const StringHash FragmentProgram            = "fragment_program_ref";
        const StringHash DefaultParameters          = "default_params";
        const StringHash TextureUnit                = "texture_unit";
    }

    namespace Pass
    {
        const StringHash Ambient                    = "ambient";
        const StringHash Diffuse                    = "diffuse";
        const StringHash Specular                   = "specular";
        const StringHash Emissive                   = "emissive";
        const StringHash SceneBlend                 = "scene_blend";
        const StringHash SeparateSceneBlend         = "separate_scene_blend";
        const StringHash SceneBlendOp               = "scene_blend_op";
        const StringHash SeparateSceneBlendOp       = "separate_scene_blend_op";
        const StringHash DepthCheck                 = "depth_check";
        const StringHash DepthWrite                 = "depth_write";
        const StringHash DepthFunc                  = "depth_func";
        const StringHash DepthBias                  = "depth_bias";
        const StringHash IterationDepthBias         = "iteration_depth_bias";
        const StringHash AlphaRejection             = "alpha_rejection";
        const StringHash AlphaToCoverage            = "alpha_to_coverage";
        const StringHash LightScissor               = "light_scissor";
        const StringHash LightClip_planes           = "light_clip_planes";
        const StringHash IlluminationStage          = "illumination_stage";
        const StringHash TransparentSorting         = "transparent_sorting";
        const StringHash NormaliseNormals           = "normalise_normals";
        const StringHash CullHardware               = "cull_hardware";
        const StringHash CullSoftware               = "cull_software";
        const StringHash Lighting                   = "lighting";
        const StringHash Shading                    = "shading";
        const StringHash PolygonMode                = "polygon_mode";
        const StringHash PolygonModeOverrideable    = "polygon_mode_overrideable";
        const StringHash FogOverride                = "fog_override";
        const StringHash ColourWrite                = "colour_write";
        const StringHash MaxLights                  = "max_lights";
        const StringHash StartLight                 = "start_light";
        const StringHash Iteration                  = "iteration";
        const StringHash PointSize                  = "point_size";
        const StringHash PointSprites               = "point_sprites";
        const StringHash PointSizeAttenuation       = "point_size_attenuation";
        const StringHash PointSizeMin               = "point_size_min";
        const StringHash PointSizeMax               = "point_size_max";
    }

    namespace TextureUnit
    {
        const StringHash TextureAlias               = "texture_alias";
        const StringHash Texture                    = "texture";
        const StringHash AnimTexture                = "anim_texture";
        const StringHash CubicTexture               = "cubic_texture";
        const StringHash TexCoordSet                = "tex_coord_set";
        const StringHash TexAddressMode             = "tex_address_mode";
        const StringHash TexBorderColour            = "tex_border_colour";
        const StringHash Filtering                  = "filtering";
        const StringHash MaxAnisotropy              = "max_anisotropy";
        const StringHash MipmapBias                 = "mipmap_bias";
        const StringHash ColourOp                   = "colour_op";
        const StringHash ColourOpEx                 = "colour_op_ex";
        const StringHash ColourOpMultipassFallback  = "colour_op_multipass_fallback";
        const StringHash AlphaOpEx                  = "alpha_op_ex";
        const StringHash EnvMap                     = "env_map";
        const StringHash Scroll                     = "scroll";
        const StringHash ScrollAnim                 = "scroll_anim";
        const StringHash Rotate                     = "rotate";
        const StringHash RotateAnim                 = "rotate_anim";
        const StringHash Scale                      = "scale";
        const StringHash WaveXform                  = "wave_xform";
        const StringHash Transform                  = "transform";
        const StringHash BindingType                = "binding_type";
        const StringHash ContentType                = "content_type";
    }

    namespace Program
    {
        const StringHash Source                     = "source";
        const StringHash Syntax                     = "syntax";
        const StringHash Profiles                   = "profiles";
        const StringHash EntryPoint                 = "entry_point";
        const StringHash ParamIndexedAuto           = "param_indexed_auto";
        const StringHash ParamIndexed               = "param_indexed";
        const StringHash ParamNamedAuto             = "param_named_auto";
        const StringHash ParamNamed                 = "param_named";
        const StringHash SharedParamNamed           = "shared_param_named";
    }
}

/// Material section
enum MaterialPart
{
    MP_Unsupported,
    MP_Material,
    MP_Technique,
    MP_Pass,
    MP_VertexProgram,
    MP_FragmentProgram,
    MP_DefaultParameters,
    MP_TextureUnit
};

static inline String MaterialPartToString(MaterialPart part)
{
    switch(part)
    {
        case MP_Material:           return "material";
        case MP_Technique:          return "technique";
        case MP_Pass:               return "pass";
        case MP_VertexProgram:      return "vertex_program_ref";
        case MP_FragmentProgram:    return "fragment_program_ref";
        case MP_DefaultParameters:  return "default_params";
        case MP_TextureUnit:        return "texture_unit";
    }
    return "Invalid MaterialPart enum " + String(static_cast<int>(part));
}

/// Material properties
typedef HashMap<StringHash, Vector<String> > MaterialProperties;
typedef HashMap<StringHash, String> MaterialPropertyNames;

/// Material block
struct URHO_MODULE_API MaterialBlock
{
    /** Block name, ref or identifier eg,
        material <name>
        texture_unit <name>
        vertex_program_ref <ref> */
    String id;

    /// Blocks part identifier
    MaterialPart part;

    /// Properties
    MaterialProperties properties;
    MaterialPropertyNames propertyNames;

    /// Block indexes
    int technique;
    int pass;
    int textureUnit;

    /// Child blocks
    Vector<MaterialBlock*> blocks;

    /// Parent block
    MaterialBlock *parent;

    MaterialBlock(MaterialBlock *parent_ = 0, MaterialPart part_ = MP_Unsupported, int t = -1, int p = -1, int tu = -1);
    ~MaterialBlock();

    /// Available in MP_Material (MaterialParser::root)
    uint NumTechniques() const;
    MaterialBlock *Technique(uint index = 0) const;
    /// Available in MP_Technique
    uint NumPasses() const;
    MaterialBlock *Pass(uint index = 0) const;
    /// Available in MP_Pass
    uint NumTextureUnits() const;
    MaterialBlock *TextureUnit(uint index = 0) const;

    /// Available in MP_Pass
    MaterialBlock *VertexProgram() const;
    /// Available in MP_Pass
    MaterialBlock *FragmentProgram() const;

    /// Number of children of type @c part.
    uint NumChildren(MaterialPart part) const;

    /// Number of @c name properties
    uint Num(const StringHash &name) const;

    /// If @c name property exists
    bool Has(const StringHash &name) const;

    /// Return @c name property as a string for @c index. If not defined @c defaultValue is returned.
    String StringValue(const StringHash &name, const String &defaultValue, uint index = 0) const;

    /// Return @c name property as a string for @c index. If not defined a empty vector is returned.
    /** All values are trimmed separately and whitespace is removed between values. */
    StringVector StringVectorValue(const StringHash &name, uint index = 0) const;

    /// Return @c name property as a VectorN for @c index. If not defined or not enough components @c defaultValue is returned.
    Urho3D::Vector2 Vector2Value(const StringHash &name, const Urho3D::Vector2 &defaultValue, uint index = 0) const;
    Urho3D::Vector3 Vector3Value(const StringHash &name, const Urho3D::Vector3 &defaultValue, uint index = 0) const;
    Urho3D::Vector4 Vector4Value(const StringHash &name, const Urho3D::Vector4 &defaultValue, uint index = 0) const;

    /// Return @c name property as a Color for @c index. If not defined or valid color @c defaultValue is returned.
    Urho3D::Color ColorValue(const StringHash &name, const Urho3D::Color &defaultValue, uint index = 0) const;

    /// Return @c name property as a boolean for @c index. If not defined or valid boolean @c defaultValue is returned.
    bool BooleanValue(const StringHash &name, bool defaultValue, uint index = 0) const;

    /** @todo Specialized value getters for param_named for "param_named <param_name> float4 1 2 3 4" etc.
        where <param_name> can be provided and 'float4' etc. indentifiers are chopped from the beginning. */

    /// Returns if this block is supported.
    /** If false no properties have been parsed for the block scope. */
    bool IsSupported();

    /// Dump material to stdout.
    void Dump(bool recursive = true, uint indentation = 0);
};

/// Material parser
/** This parser is intentionally generic and does not consern itself
    with property types or values. Its meant to store all data
    while preserving Ogre material hierarchy. This data can then
    be selectively then processed into a Urho Material. */
class URHO_MODULE_API MaterialParser
{
public:
    MaterialParser();
    ~MaterialParser();

    /// Root block
    /** Can be accessed after Parse to query material data. */
    MaterialBlock *root;

    /// Parses script from input data.
    /** @param Returns success. If false you can query Error() for a printable message. */
    bool Parse(const char *data_, uint lenght_);

    /// Returns error that occurred while parsing,
    String Error() const;

private:
    bool ProcessLine();
    
    void Advance();
    void SkipBlock();

    /// Data
    String data;
    String line;

    uint pos;
    uint lineNum;
    
    /// State
    struct State
    {
        MaterialBlock *block;

        int technique;
        int pass;
        int textureUnit;

        String error;

        State() : block(0), technique(-1), pass(-1), textureUnit(-1) {}
    };
    State state;
};

}

}

/// @endcond
