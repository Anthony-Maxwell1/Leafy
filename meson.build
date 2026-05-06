project('leafy', 'cpp',
  version: 'v1.0.0',
  default_options: ['cpp_std=c++17'],
  meson_version: '>=1.1'
)

sources = files(
  './src/main.cpp',
  './src/renderer.cpp',
  './src/raster.cpp',
  './src/backend_kindle.cpp',
  './src/texture.cpp',
  './src/stb_image_impl.cpp',
  './src/texture_cache.cpp',
  './src/input_system.cpp',
  './src/app.cpp',
  './src/lua.cpp'
)

include_dirs = include_directories(
  './src/include/',
  './src/external'
)

executable(
  'leafy',
  sources,
  include_directories: include_dirs,
  cpp_args: '-static-libstdc++',
  link_args: '-static-libstdc++'
)