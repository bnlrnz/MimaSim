cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(Mimasim VERSION 1.0 LANGUAGES C)

add_executable(MimaSim src/main.c src/log.c src/mima.c src/mima_compiler.c)
target_include_directories(MimaSim PRIVATE include)
#target_link_libraries()