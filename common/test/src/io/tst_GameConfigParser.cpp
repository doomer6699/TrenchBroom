/*
 Copyright (C) 2010 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "el/Expression.h"
#include "io/DiskIO.h"
#include "io/GameConfigParser.h"
#include "io/Reader.h"
#include "io/TraversalMode.h"
#include "mdl/GameConfig.h"
#include "mdl/Tag.h"
#include "mdl/TagMatcher.h"

#include <filesystem>
#include <string>

#include "Catch2.h"

namespace tb::io
{

TEST_CASE("GameConfigParser")
{
  SECTION("parseIncludedGameConfigs")
  {
    const auto basePath = std::filesystem::current_path() / "fixture/games/";
    const auto cfgFiles =
      Disk::find(basePath, TraversalMode::Recursive, makeExtensionPathMatcher({".cfg"}))
      | kdl::value();

    for (const auto& path : cfgFiles)
    {
      CAPTURE(path);

      auto file = Disk::openFile(path) | kdl::value();
      auto reader = file->reader().buffer();

      GameConfigParser parser(reader.stringView(), path);
      CHECK_NOTHROW(parser.parse());
    }
  }

  SECTION("parseBlankConfig")
  {
    const std::string config("   ");
    GameConfigParser parser(config);
    CHECK(parser.parse().is_error());
  }

  SECTION("parseEmptyConfig")
  {
    const std::string config("  {  } ");
    GameConfigParser parser(config);
    CHECK(parser.parse().is_error());
  }

  SECTION("parseQuakeConfig")
  {
    const std::string config(R"(
{
    "version": 9,
    "unexpectedKey": [],
    "name": "Quake",
    "icon": "Icon.png",
    "fileformats": [
        { "format": "Standard" },
        { "format": "Valve" }
    ],
    "filesystem": {
        "searchpath": "id1",
        "packageformat": { "extension": "pak", "format": "idpak" }
    },
    "materials": {
        "root": "textures",
        "extensions": ["D"],
        "palette": "gfx/palette.lmp",
        "attribute": "wad"
    },
    "entities": {
        "definitions": [ "Quake.fgd", "Quoth2.fgd", "Rubicon2.def", "Teamfortress.fgd" ],
        "defaultcolor": "0.6 0.6 0.6 1.0",
        "modelformats": [ "mdl", "bsp" ]
    },
    "tags": {
        "brush": [
            {
                "name": "Trigger",
                "attribs": [ "transparent" ],
                "match": "classname",
                "pattern": "trigger*"
            }
        ],
        "brushface": [
            {
                "name": "Clip",
                "attribs": [ "transparent" ],
                "match": "material",
                "pattern": "clip"
            },
            {
                "name": "Skip",
                "attribs": [ "transparent" ],
                "match": "material",
                "pattern": "skip"
            },
            {
                "name": "Hint",
                "attribs": [ "transparent" ],
                "match": "material",
                "pattern": "hint*"
            },
            {
                "name": "Liquid",
                "match": "material",
                "pattern": "\**"
            }
        ]
    }
}
)");

    CHECK(
      GameConfigParser(config).parse()
      == mdl::GameConfig{
        "Quake",
        {},
        {"Icon.png"},
        false,
        {// map formats
         mdl::MapFormatConfig{"Standard", {}},
         mdl::MapFormatConfig{"Valve", {}}},
        mdl::FileSystemConfig{{"id1"}, mdl::PackageFormatConfig{{".pak"}, "idpak"}},
        mdl::MaterialConfig{
          {"textures"},
          {".D"},
          {"gfx/palette.lmp"},
          "wad",
          {},
          {},
        },
        mdl::EntityConfig{
          {{"Quake.fgd"}, {"Quoth2.fgd"}, {"Rubicon2.def"}, {"Teamfortress.fgd"}},
          Color{0.6f, 0.6f, 0.6f, 1.0f},
          {},
          false},
        mdl::FaceAttribsConfig{},
        {
          mdl::SmartTag{
            "Trigger",
            {mdl::TagAttribute{1u, "transparent"}},
            std::make_unique<mdl::EntityClassNameTagMatcher>("trigger*", "")},
          mdl::SmartTag{
            "Clip",
            {mdl::TagAttribute{1u, "transparent"}},
            std::make_unique<mdl::MaterialNameTagMatcher>("clip")},
          mdl::SmartTag{
            "Skip",
            {mdl::TagAttribute{1u, "transparent"}},
            std::make_unique<mdl::MaterialNameTagMatcher>("skip")},
          mdl::SmartTag{
            "Hint",
            {mdl::TagAttribute{1u, "transparent"}},
            std::make_unique<mdl::MaterialNameTagMatcher>("hint*")},
          mdl::SmartTag{
            "Liquid", {}, std::make_unique<mdl::MaterialNameTagMatcher>("\\**")},
        },            // smart tags
        std::nullopt, // soft map bounds
        {}            // compilation tools
      });
  }

  SECTION("parseQuake2Config")
  {
    const std::string config(R"%(
{
    "version": 9,
    "name": "Quake 2",
    "icon": "Icon.png",
    "fileformats": [ { "format": "Quake2" } ],
    "filesystem": {
        "searchpath": "baseq2",
        "packageformat": { "extension": "pak", "format": "idpak" }
    },
    "materials": {
        "root": "textures",
        "extensions": ["wal"],
        "palette": "pics/colormap.pcx"
    },
    "entities": {
        "definitions": [ "Quake2.fgd" ],
        "defaultcolor": "0.6 0.6 0.6 1.0",
        "modelformats": [ "md2" ]
    },
    "tags": {
        "brush": [
            {
                "name": "Trigger",
                "attribs": [ "transparent" ],
                "match": "classname",
                "pattern": "trigger*",
                "material": "trigger"
            }
        ],
        "brushface": [
            {
                "name": "Clip",
                "attribs": [ "transparent" ],
                "match": "material",
                "pattern": "clip"
            },
            {
                "name": "Skip",
                "attribs": [ "transparent" ],
                "match": "material",
                "pattern": "skip"
            },
            {
                "name": "Hint",
                "attribs": [ "transparent" ],
                "match": "material",
                "pattern": "hint*"
            },
            {
                "name": "Detail",
                "match": "contentflag",
                "flags": [ "detail" ]
            },
            {
                "name": "Liquid",
                "match": "contentflag",
                "flags": [ "lava", "slime", "water" ]
            },
            {
                "name": "trans",
                "attribs": [ "transparent" ],
                "match": "surfaceflag",
                "flags": [ "trans33", "trans66" ]
            }
        ]
    },
    "faceattribs": {
        "surfaceflags": [
            {
                "name": "light",
                "description": "Emit light from the surface, brightness is specified in the 'value' field"
            },
            {
                "name": "slick",
                "description": "The surface is slippery"
            },
            {
                "name": "sky",
                "description": "The surface is sky, the texture will not be drawn, but the background sky box is used instead"
            },
            {
                "name": "warp",
                "description": "The surface warps (like water textures do)"
            },
            {
                "name": "trans33",
                "description": "The surface is 33% transparent"
            },
            {
                "name": "trans66",
                "description": "The surface is 66% transparent"
            },
            {
                "name": "flowing",
                "description": "The texture wraps in a downward 'flowing' pattern (warp must also be set)"
            },
            {
                "name": "nodraw",
                "description": "Used for non-fixed-size brush triggers and clip brushes"
            },
            {
                "name": "hint",
                "description": "Make a primary bsp splitter"
            },
            {
                "name": "skip",
                "description": "Completely ignore, allowing non-closed brushes"
            }
        ],
        "contentflags": [
            {
                "name": "solid",
                "description": "Default for all brushes"
            }, // 1 << 0
            {
                "name": "window",
                "description": "Brush is a window (not really used)"
            }, // 1 << 1
            {
                "name": "aux",
                "description": "Unused by the engine"
            }, // 1 << 2
            {
                "name": "lava",
                "description": "The brush is lava"
            }, // 1 << 3
            {
                "name": "slime",
                "description": "The brush is slime"
            }, // 1 << 4
            {
                "name": "water",
                "description": "The brush is water"
            }, // 1 << 5
            {
                "name": "mist",
                "description": "The brush is non-solid"
            }, // 1 << 6
            { "unused": true }, // 1 << 7
            { "unused": true }, // 1 << 8
            { "unused": true }, // 1 << 9
            { "unused": true }, // 1 << 10
            { "unused": true }, // 1 << 11
            { "unused": true }, // 1 << 12
            { "unused": true }, // 1 << 13
            { "unused": true }, // 1 << 14
            { "unused": true }, // 1 << 15
            {
                "name": "playerclip",
                "description": "Player cannot pass through the brush (other things can)"
            }, // 1 << 16
            {
                "name": "monsterclip",
                "description": "Monster cannot pass through the brush (player and other things can)"
            }, // 1 << 17
            {
                "name": "current_0",
                "description": "Brush has a current in direction of 0 degrees"
            }, // 1 << 18
            {
                "name": "current_90",
                "description": "Brush has a current in direction of 90 degrees"
            }, // 1 << 19
            {
                "name": "current_180",
                "description": "Brush has a current in direction of 180 degrees"
            }, // 1 << 20
            {
                "name": "current_270",
                "description": "Brush has a current in direction of 270 degrees"
            }, // 1 << 21
            {
                "name": "current_up",
                "description": "Brush has a current in the up direction"
            }, // 1 << 22
            {
                "name": "current_dn",
                "description": "Brush has a current in the down direction"
            }, // 1 << 23
            {
                "name": "origin",
                "description": "Special brush used for specifying origin of rotation for rotating brushes"
            }, // 1 << 24
            {
                "name": "monster",
                "description": "Purpose unknown"
            }, // 1 << 25
            {
                "name": "corpse",
                "description": "Purpose unknown"
            }, // 1 << 26
            {
                "name": "detail",
                "description": "Detail brush"
            }, // 1 << 27
            {
                "name": "translucent",
                "description": "Use for opaque water that does not block vis"
            }, // 1 << 28
            {
                "name": "ladder",
                "description": "Brushes with this flag allow a player to move up and down a vertical surface"
            } // 1 << 29
        ]
    }
}
)%");

    CHECK(
      GameConfigParser(config).parse()
      == mdl::GameConfig{
        "Quake 2",
        {},
        {"Icon.png"},
        false,
        {mdl::MapFormatConfig{"Quake2", {}}},
        mdl::FileSystemConfig{{"baseq2"}, mdl::PackageFormatConfig{{".pak"}, "idpak"}},
        mdl::MaterialConfig{
          {"textures"},
          {".wal"},
          {"pics/colormap.pcx"},
          std::nullopt,
          {},
          {},
        },
        mdl::EntityConfig{{{"Quake2.fgd"}}, Color{0.6f, 0.6f, 0.6f, 1.0f}, {}, false},
        mdl::FaceAttribsConfig{
          {{{"light",
             "Emit light from the surface, brightness is specified in the 'value' field",
             1 << 0},
            {"slick", "The surface is slippery", 1 << 1},
            {"sky",
             "The surface is sky, the texture will not be drawn, but the background sky "
             "box is used "
             "instead",
             1 << 2},
            {"warp", "The surface warps (like water textures do)", 1 << 3},
            {"trans33", "The surface is 33% transparent", 1 << 4},
            {"trans66", "The surface is 66% transparent", 1 << 5},
            {"flowing",
             "The texture wraps in a downward 'flowing' pattern (warp must also be set)",
             1 << 6},
            {"nodraw", "Used for non-fixed-size brush triggers and clip brushes", 1 << 7},
            {"hint", "Make a primary bsp splitter", 1 << 8},
            {"skip", "Completely ignore, allowing non-closed brushes", 1 << 9}}},
          {{{"solid", "Default for all brushes", 1 << 0},
            {"window", "Brush is a window (not really used)", 1 << 1},
            {"aux", "Unused by the engine", 1 << 2},
            {"lava", "The brush is lava", 1 << 3},
            {"slime", "The brush is slime", 1 << 4},
            {"water", "The brush is water", 1 << 5},
            {"mist", "The brush is non-solid", 1 << 6},
            {"playerclip",
             "Player cannot pass through the brush (other things can)",
             1 << 16},
            {"monsterclip",
             "Monster cannot pass through the brush (player and other things can)",
             1 << 17},
            {"current_0", "Brush has a current in direction of 0 degrees", 1 << 18},
            {"current_90", "Brush has a current in direction of 90 degrees", 1 << 19},
            {"current_180", "Brush has a current in direction of 180 degrees", 1 << 20},
            {"current_270", "Brush has a current in direction of 270 degrees", 1 << 21},
            {"current_up", "Brush has a current in the up direction", 1 << 22},
            {"current_dn", "Brush has a current in the down direction", 1 << 23},
            {"origin",
             "Special brush used for specifying origin of rotation for rotating brushes",
             1 << 24},
            {"monster", "Purpose unknown", 1 << 25},
            {"corpse", "Purpose unknown", 1 << 26},
            {"detail", "Detail brush", 1 << 27},
            {"translucent", "Use for opaque water that does not block vis", 1 << 28},
            {"ladder",
             "Brushes with this flag allow a player to move up and down a vertical "
             "surface",
             1 << 29}}},
          mdl::BrushFaceAttributes{mdl::BrushFaceAttributes::NoMaterialName}},
        {
          mdl::SmartTag{
            "Trigger",
            {mdl::TagAttribute{1u, "transparent"}},
            std::make_unique<mdl::EntityClassNameTagMatcher>("trigger*", "trigger")},
          mdl::SmartTag{
            "Clip",
            {mdl::TagAttribute{1u, "transparent"}},
            std::make_unique<mdl::MaterialNameTagMatcher>("clip")},
          mdl::SmartTag{
            "Skip",
            {mdl::TagAttribute{1u, "transparent"}},
            std::make_unique<mdl::MaterialNameTagMatcher>("skip")},
          mdl::SmartTag{
            "Hint",
            {mdl::TagAttribute{1u, "transparent"}},
            std::make_unique<mdl::MaterialNameTagMatcher>("hint*")},
          mdl::SmartTag{
            "Detail", {}, std::make_unique<mdl::ContentFlagsTagMatcher>(1 << 27)},
          mdl::SmartTag{
            "Liquid",
            {},
            std::make_unique<mdl::ContentFlagsTagMatcher>(
              (1 << 3) | (1 << 4) | (1 << 5))},
          mdl::SmartTag{
            "trans",
            {},
            std::make_unique<mdl::SurfaceFlagsTagMatcher>((1 << 4) | (1 << 5))},
        },            // smart tags
        std::nullopt, // soft map bounds
        {}            // compilation tools
      });
  }

  SECTION("parseExtrasConfig")
  {
    const std::string config(R"%(
{
    "version": 9,
    "name": "Extras",
    "fileformats": [ { "format": "Quake3" } ],
    "filesystem": {
        "searchpath": "baseq3",
        "packageformat": { "extension": "pk3", "format": "zip" }
    },
    "materials": {
        "root": "textures",
        "extensions": [ "" ],
        "shaderSearchPath": "scripts", // this will likely change when we get a material system
        "excludes": [
            "*_norm",
            "*_gloss"
        ]
    },
    "entities": {
        "definitions": [ "Extras.ent" ],
        "defaultcolor": "0.6 0.6 0.6 1.0",
        "modelformats": [ "md3" ],
        "scale": [ modelscale, modelscale_vec ]
    },
    "tags": {
        "brush": [
            {
                "name": "Trigger",
                "attribs": [ "transparent" ],
                "match": "classname",
                "pattern": "trigger*",
                "material": "trigger"
            }
        ],
        "brushface": [
            {
                "name": "Clip",
                "attribs": [ "transparent" ],
                "match": "surfaceparm",
                "pattern": "playerclip"
            },
            {
                "name": "Skip",
                "attribs": [ "transparent" ],
                "match": "material",
                "pattern": "skip"
            },
            {
                "name": "Hint",
                "attribs": [ "transparent" ],
                "match": "material",
                "pattern": "hint*"
            },
            {
                "name": "Detail",
                "match": "contentflag",
                "flags": [ "detail" ]
            },
            {
                "name": "Liquid",
                "match": "contentflag",
                "flags": [ "lava", "slime", "water" ]
            }
        ]
    },
    "faceattribs": {
        "defaults": {
            "materialName": "defaultMaterial",
            "offset": [0, 0],
            "scale": [0.5, 0.5],
            "rotation": 0,
            "surfaceFlags": [ "slick" ],
            "surfaceContents": [ "solid" ],
            "surfaceValue": 0,
            "color": "1.0 1.0 1.0 1.0"
        },
        "surfaceflags": [
            {
                "name": "light",
                "description": "Emit light from the surface, brightness is specified in the 'value' field"
            },
            {
                "name": "slick",
                "description": "The surface is slippery"
            },
            {
                "name": "sky",
                "description": "The surface is sky, the texture will not be drawn, but the background sky box is used instead"
            },
            {
                "name": "warp",
                "description": "The surface warps (like water textures do)"
            },
            {
                "name": "trans33",
                "description": "The surface is 33% transparent"
            },
            {
                "name": "trans66",
                "description": "The surface is 66% transparent"
            },
            {
                "name": "flowing",
                "description": "The texture wraps in a downward 'flowing' pattern (warp must also be set)"
            },
            {
                "name": "nodraw",
                "description": "Used for non-fixed-size brush triggers and clip brushes"
            },
            {
                "name": "hint",
                "description": "Make a primary bsp splitter"
            },
            {
                "name": "skip",
                "description": "Completely ignore, allowing non-closed brushes"
            }
        ],
        "contentflags": [
            {
                "name": "solid",
                "description": "Default for all brushes"
            }, // 1
            {
                "name": "window",
                "description": "Brush is a window (not really used)"
            }, // 2
            {
                "name": "aux",
                "description": "Unused by the engine"
            }, // 4
            {
                "name": "lava",
                "description": "The brush is lava"
            }, // 8
            {
                "name": "slime",
                "description": "The brush is slime"
            }, // 16
            {
                "name": "water",
                "description": "The brush is water"
            }, // 32
            {
                "name": "mist",
                "description": "The brush is non-solid"
            }, // 64
            { "unused": true }, // 128
            { "unused": true }, // 256
            { "unused": true }, // 512
            { "unused": true }, // 1024
            { "unused": true }, // 2048
            { "unused": true }, // 4096
            { "unused": true }, // 8192
            { "unused": true }, // 16384
            { "unused": true }, // 32768
            {
                "name": "playerclip",
                "description": "Player cannot pass through the brush (other things can)"
            }, // 65536
            {
                "name": "monsterclip",
                "description": "Monster cannot pass through the brush (player and other things can)"
            }, // 131072
            {
                "name": "current_0",
                "description": "Brush has a current in direction of 0 degrees"
            },
            {
                "name": "current_90",
                "description": "Brush has a current in direction of 90 degrees"
            },
            {
                "name": "current_180",
                "description": "Brush has a current in direction of 180 degrees"
            },
            {
                "name": "current_270",
                "description": "Brush has a current in direction of 270 degrees"
            },
            {
                "name": "current_up",
                "description": "Brush has a current in the up direction"
            },
            {
                "name": "current_dn",
                "description": "Brush has a current in the down direction"
            },
            {
                "name": "origin",
                "description": "Special brush used for specifying origin of rotation for rotating brushes"
            },
            {
                "name": "monster",
                "description": "Purpose unknown"
            },
            {
                "name": "corpse",
                "description": "Purpose unknown"
            },
            {
                "name": "detail",
                "description": "Detail brush"
            },
            {
                "name": "translucent",
                "description": "Use for opaque water that does not block vis"
            },
            {
                "name": "ladder",
                "description": "Brushes with this flag allow a player to move up and down a vertical surface"
            }
        ]
    }
}
)%");

    mdl::BrushFaceAttributes expectedBrushFaceAttributes("defaultMaterial");
    expectedBrushFaceAttributes.setOffset(vm::vec2f(0.0f, 0.0f));
    expectedBrushFaceAttributes.setScale(vm::vec2f(0.5f, 0.5f));
    expectedBrushFaceAttributes.setRotation(0.0f);
    expectedBrushFaceAttributes.setSurfaceContents(1 << 0);
    expectedBrushFaceAttributes.setSurfaceFlags(1 << 1);
    expectedBrushFaceAttributes.setSurfaceValue(0.0f);
    expectedBrushFaceAttributes.setColor(Color(255, 255, 255, 255));

    CHECK(
      GameConfigParser(config).parse()
      == mdl::GameConfig{
        "Extras",
        {},
        {},
        false,
        {mdl::MapFormatConfig{"Quake3", {}}},
        mdl::FileSystemConfig{{"baseq3"}, mdl::PackageFormatConfig{{".pk3"}, "zip"}},
        mdl::MaterialConfig{
          {"textures"},
          {""},
          {},
          std::nullopt,
          {"scripts"},
          {"*_norm", "*_gloss"},
        },
        mdl::EntityConfig{
          {{"Extras.ent"}},
          Color{0.6f, 0.6f, 0.6f, 1.0f},
          el::ExpressionNode{el::ArrayExpression{{
            // the line numbers are not checked
            el::ExpressionNode{el::VariableExpression{"modelscale"}},
            el::ExpressionNode{el::VariableExpression{"modelscale_vec"}},
          }}},
          false},
        mdl::FaceAttribsConfig{
          {{{"light",
             "Emit light from the surface, brightness is specified in the 'value' field",
             1 << 0},
            {"slick", "The surface is slippery", 1 << 1},
            {"sky",
             "The surface is sky, the texture will not be drawn, but the background sky "
             "box is used "
             "instead",
             1 << 2},
            {"warp", "The surface warps (like water textures do)", 1 << 3},
            {"trans33", "The surface is 33% transparent", 1 << 4},
            {"trans66", "The surface is 66% transparent", 1 << 5},
            {"flowing",
             "The texture wraps in a downward 'flowing' pattern (warp must also be set)",
             1 << 6},
            {"nodraw", "Used for non-fixed-size brush triggers and clip brushes", 1 << 7},
            {"hint", "Make a primary bsp splitter", 1 << 8},
            {"skip", "Completely ignore, allowing non-closed brushes", 1 << 9}}},
          {{{"solid", "Default for all brushes", 1 << 0},
            {"window", "Brush is a window (not really used)", 1 << 1},
            {"aux", "Unused by the engine", 1 << 2},
            {"lava", "The brush is lava", 1 << 3},
            {"slime", "The brush is slime", 1 << 4},
            {"water", "The brush is water", 1 << 5},
            {"mist", "The brush is non-solid", 1 << 6},
            {"playerclip",
             "Player cannot pass through the brush (other things can)",
             1 << 16},
            {"monsterclip",
             "Monster cannot pass through the brush (player and other things can)",
             1 << 17},
            {"current_0", "Brush has a current in direction of 0 degrees", 1 << 18},
            {"current_90", "Brush has a current in direction of 90 degrees", 1 << 19},
            {"current_180", "Brush has a current in direction of 180 degrees", 1 << 20},
            {"current_270", "Brush has a current in direction of 270 degrees", 1 << 21},
            {"current_up", "Brush has a current in the up direction", 1 << 22},
            {"current_dn", "Brush has a current in the down direction", 1 << 23},
            {"origin",
             "Special brush used for specifying origin of rotation for rotating brushes",
             1 << 24},
            {"monster", "Purpose unknown", 1 << 25},
            {"corpse", "Purpose unknown", 1 << 26},
            {"detail", "Detail brush", 1 << 27},
            {"translucent", "Use for opaque water that does not block vis", 1 << 28},
            {"ladder",
             "Brushes with this flag allow a player to move up and down a vertical "
             "surface",
             1 << 29}}},
          expectedBrushFaceAttributes},
        {
          mdl::SmartTag{
            "Trigger",
            {mdl::TagAttribute{1u, "transparent"}},
            std::make_unique<mdl::EntityClassNameTagMatcher>("trigger*", "trigger")},
          mdl::SmartTag{
            "Clip",
            {mdl::TagAttribute{1u, "transparent"}},
            std::make_unique<mdl::MaterialNameTagMatcher>("clip")},
          mdl::SmartTag{
            "Skip",
            {mdl::TagAttribute{1u, "transparent"}},
            std::make_unique<mdl::MaterialNameTagMatcher>("skip")},
          mdl::SmartTag{
            "Hint",
            {mdl::TagAttribute{1u, "transparent"}},
            std::make_unique<mdl::MaterialNameTagMatcher>("hint*")},
          mdl::SmartTag{
            "Detail", {}, std::make_unique<mdl::ContentFlagsTagMatcher>(1 << 27)},
          mdl::SmartTag{
            "Liquid",
            {},
            std::make_unique<mdl::ContentFlagsTagMatcher>(
              (1 << 3) | (1 << 4) | (1 << 5))},
        },            // smart tags
        std::nullopt, // soft map bounds
        {}            // compilation tools
      });
  }

  SECTION("parseDuplicateTags")
  {
    const std::string config(R"(
{
    "version": 9,
    "name": "Quake",
    "icon": "Icon.png",
    "fileformats": [
        { "format": "Standard" }
    ],
    "filesystem": {
        "searchpath": "id1",
        "packageformat": { "extension": "pak", "format": "idpak" }
    },
    "materials": {
        "root": "textures",
        "extensions": ["D"],
        "palette": "gfx/palette.lmp",
        "attribute": "wad"
    },
    "entities": {
        "definitions": [ "Quake.fgd", "Quoth2.fgd", "Rubicon2.def", "Teamfortress.fgd" ],
        "defaultcolor": "0.6 0.6 0.6 1.0",
        "modelformats": [ "mdl", "bsp" ]
    },
    "tags": {
        "brush": [
            {
                "name": "Trigger",
                "attribs": [ "transparent" ],
                "match": "classname",
                "pattern": "trigger*"
            }
        ],
        "brushface": [
            {
                "name": "Trigger",
                "attribs": [ "transparent" ],
                "match": "material",
                "pattern": "clip"
            }
        ]
    }
}
)");

    GameConfigParser parser(config);
    CHECK(parser.parse().is_error());
  }

  SECTION("parseSetDefaultProperties")
  {
    const std::string config(R"(
{
    "version": 9,
    "name": "Quake",
    "icon": "Icon.png",
    "fileformats": [
        { "format": "Standard" }
    ],
    "filesystem": {
        "searchpath": "id1",
        "packageformat": { "extension": "pak", "format": "idpak" }
    },
    "materials": {
        "root": "textures",
        "extensions": ["D"],
        "palette": "gfx/palette.lmp",
        "attribute": "wad"
    },
    "entities": {
        "definitions": [ "Quake.fgd", "Quoth2.fgd", "Rubicon2.def", "Teamfortress.fgd" ],
        "defaultcolor": "0.6 0.6 0.6 1.0",
        "modelformats": [ "mdl", "bsp" ],
        "setDefaultProperties": true
    }
}
)");

    CHECK(
      GameConfigParser(config).parse()
      == mdl::GameConfig{
        "Quake",
        {},
        {"Icon.png"},
        false,
        {mdl::MapFormatConfig{"Standard", {}}},
        mdl::FileSystemConfig{{"id1"}, mdl::PackageFormatConfig{{".pak"}, "idpak"}},
        mdl::MaterialConfig{
          {"textures"},
          {".D"},
          {"gfx/palette.lmp"},
          "wad",
          {},
          {},
        },
        mdl::EntityConfig{
          {{"Quake.fgd"}, {"Quoth2.fgd"}, {"Rubicon2.def"}, {"Teamfortress.fgd"}},
          Color{0.6f, 0.6f, 0.6f, 1.0f},
          {},
          true}, // setDefaultProperties
        mdl::FaceAttribsConfig{},
        {},
        std::nullopt, // soft map bounds
        {}            // compilation tools
      });
  }
}

} // namespace tb::io
