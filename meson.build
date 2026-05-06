project('leafy', 'cpp', 'c',
  version: 'v1.0.0',
  default_options: ['cpp_std=c++17'],
  meson_version: '>=1.1'
)

# =========================================================
# PATH ROOTS
# =========================================================

src_root = meson.project_source_root()

lua_dir = 'src/external/lua'
freetype_dir = 'src/external/freetype'

freetype_include = include_directories(
  join_paths(freetype_dir, 'build/include/freetype2')
)

# =========================================================
# LUA (BUILD FROM SOURCE)
# =========================================================

lua_sources = files(
  'src/external/lua/lapi.c',
  'src/external/lua/lauxlib.c',
  'src/external/lua/lbaselib.c',
  'src/external/lua/lcode.c',
  'src/external/lua/lcorolib.c',
  'src/external/lua/lctype.c',
  'src/external/lua/ldblib.c',
  'src/external/lua/ldebug.c',
  'src/external/lua/ldo.c',
  'src/external/lua/ldump.c',
  'src/external/lua/lfunc.c',
  'src/external/lua/lgc.c',
  'src/external/lua/linit.c',
  'src/external/lua/liolib.c',
  'src/external/lua/llex.c',
  'src/external/lua/lmathlib.c',
  'src/external/lua/lmem.c',
  'src/external/lua/loadlib.c',
  'src/external/lua/lobject.c',
  'src/external/lua/lopcodes.c',
  'src/external/lua/loslib.c',
  'src/external/lua/lparser.c',
  'src/external/lua/lstate.c',
  'src/external/lua/lstring.c',
  'src/external/lua/lstrlib.c',
  'src/external/lua/ltable.c',
  'src/external/lua/ltablib.c',
  'src/external/lua/ltm.c',
  'src/external/lua/lundump.c',
  'src/external/lua/lutf8lib.c',
  'src/external/lua/lvm.c',
  'src/external/lua/lzio.c',
)

lua_lib = static_library(
  'lua',
  lua_sources,
  include_directories: include_directories(lua_dir),
  c_args: ['-Wno-implicit-function-declaration'],
  pic: true
)

# =========================================================
# FREETYPE (PREBUILT STATIC LIB)
# =========================================================

freetype_lib_path = join_paths(
  meson.current_source_dir(),
  freetype_dir,
  'build/lib/libfreetype.a'
)

freetype_dep = declare_dependency(
  include_directories: freetype_include,
  link_args: [
    freetype_lib_path
  ]
)

# =========================================================
# SOURCES
# =========================================================

sources = files(
  'src/main.cpp',
  'src/renderer.cpp',
  'src/raster.cpp',
  'src/backend_kindle.cpp',
  'src/texture.cpp',
  'src/stb_image_impl.cpp',
  'src/texture_cache.cpp',
  'src/input_system.cpp',
  'src/app.cpp',
  'src/lua.cpp',
  'src/ui.cpp'
)

include_dirs = include_directories(
  'src/include',
  'src/external',
  'src/external/lua'
)

# =========================================================
# EXECUTABLE
# =========================================================

executable(
  'leafy',
  sources,
  include_directories: include_dirs,
  link_with: lua_lib,
  dependencies: [freetype_dep],
  cpp_args: ['-static-libstdc++'],
  link_args: ['-static-libstdc++']
)