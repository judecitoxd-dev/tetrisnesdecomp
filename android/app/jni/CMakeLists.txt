cmake_minimum_required(VERSION 3.22.1)
project(TetrisNESAndroid LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# The workflow copies the official SDL 2.30.11 source tree here.
add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_subdirectory(src)
