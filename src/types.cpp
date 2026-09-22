#include "types.h"

// Table lifted from MrIkso/Nioh3FileLoaderLogger (MIT), see LICENSE.
static const TypeInfo kTypes[] = {
    // models, textures, animation
    { 0xAFBEC60C, "TexContext",                          ".g1t" },
    { 0xAD57EBBA, "StreamingTexContext",                 ".g1t" },
    { 0x563BDEF1, "ModelData",                           ".g1m" },
    { 0xBEF563DD, "StreamingMeshletModelData",           ".g1m" },
    { 0x786DCD84, "G1NFile",                             ".g1n" },
    { 0x17614AF5, "G1MXFile",                            ".g1mx" },
    { 0x6FA91671, "G1AFile",                             ".g1a" },
    { 0x7BCD279F, "G1SFile",                             ".g1s" },
    { 0x79C724C2, "G1PFile",                             ".g1p" },
    { 0x54738C76, "G1COFile",                            ".g1co" },
    { 0xA8D88566, "G1COXFile",                           ".g1cox" },
    { 0x7461C7CA, "G1HFile",                             ".g1h" },
    { 0xDB0AE0AA, "G1IIFile",                            ".gii" },
    { 0xB097D41F, "EffectData",                          ".g1e" },
    { 0x4D0102AC, "EffectMeshData",                      ".g1em" },
    { 0x1A6300FD, "EffectShapeMeshData",                 ".g1es" },
    { 0x2BCC0C02, "FRAnimationData",                     ".g1frani" },
    { 0x32AC9403, "FPoseData",                           ".g1fpose" },
    // system databases
    { 0x20A6A0BB, "ObjectDatabaseFile",                  ".kidsobjdb" },
    { 0xBF6B52C7, "NameDatabaseFile",                    ".name" },
    { 0x1FDCAA40, "TaskGraphFile",                       ".kidstask" },
    { 0xB1630F51, "RenderGraphFile",                     ".kidsrender" },
    { 0xBE144B78, "KTIDFile",                            ".ktid" },
    { 0x8E39AA37, "KTIDFileBinary",                      ".ktid" },
    { 0xB0A14534, "GlobalConfiguration",                 ".sgcbin" },
    { 0x8D735C52, "OBOROStaticResourceBinaryFile",       ".oboro" },
    // bind tables
    { 0x1AB40AE8, "OIDBindTableBinaryFile",              ".oid" },
    { 0xDBCB74A9, "OIDFile",                             ".oid" },
    { 0xE6A3C3BB, "OIDBindTableBinaryFileEx",            ".oidex" },
    { 0x9CB3A4B6, "OIDExFile",                           ".oidex" },
    { 0x753AA042, "OIDSQTBindTableBinaryFile",           ".oidsq" },
    { 0xF02F31AB, "OIDBindTable",                        "" },
    { 0xB340861A, "MaterialGroupBindTableBinaryFile",    ".mtl" },
    { 0x56EFE45C, "PartsModelGroupBindTableBinaryFile",  ".grp" },
    { 0xBBF9B49D, "GroupFile",                           ".grp" },
    { 0x27BC54B7, "RigBinFile",                          ".rigbin" },
    { 0x133D2C3B, "ShaderBindTableBinaryFile",           ".sid" },
    // scripts, collision
    { 0x5599AA51, "KSCLFile",                            ".kscl" },
    { 0x4F16D0EF, "KTSFile",                             ".kts" },
    { 0xED410290, "TexStageTableBinaryFile",             ".kts" },
    // ui, text
    { 0xA1BDB205, "G2NFile",                             ".g2n" },
    { 0x96C74B4F, "G2NGlyphSetFile",                     ".g2n" },
    { 0xC9D883C2, "ScreenLayoutColorTableBinaryFile",    ".colortable" },
    { 0xF13845EF, "ScreenLayoutShapeInfoFile",           ".sclshape" },
    { 0xF20DE437, "StaticScreenLayoutTexInfoFile",       ".texinfo" },
    // audio, video
    { 0xBBD39F2D, "AssetData",                           ".srsa" },
    { 0x0D34474D, "StreamAssetDataFile",                 ".srst" },
    { 0xA027E46B, "VideoStreamset",                      ".mov" },
    // misc
    { 0x5B2970FC, "KTF2File",                            ".ktf2" },
    { 0xD7F47FB1, "BinaryFile",                          ".efpl" },
    { 0x193D2E44, "RBFData",                             ".grbf" },
    { 0x4638B72D, "River2BakedGeometry",                 ".rbg" },
    { 0x5C3E543C, "SwingData",                           ".swg" },
    { 0x82945A44, "LandscapeQuadtree",                   ".lsqtree" },
    { 0xCBFD49B2, "MotionMatchingDatabase",              ".mmdb" },
    { 0x0BD05B27, "MITFile",                             ".mit" },
    { 0x6DBD6EA6, "CSVFile",                             ".mit" },
};

const TypeInfo* LookupType(uint32_t id) {
    for (const auto& t : kTypes) if (t.id == id) return &t;
    return nullptr;
}
