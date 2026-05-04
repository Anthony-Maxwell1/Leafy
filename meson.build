project('example_gtk_application', 'cpp', version: 'v1.0.0', default_options: ['cpp_std=c++17'], meson_version: '>=1.1')

# Define dependencies we want

###
# Project definition
###
sources = files(
    './src/main.cpp'
)

include_dirs = include_directories(
  './src/include/'
)

executable('example_gtk_application', sources, include_directories: include_dirs, dependencies: [], cpp_args: '-static-libstdc++', link_args: '-static-libstdc++')