cmake_minimum_required(VERSION 3.13)
project(FigmaShadow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(SHARED_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cpp")

file(GLOB CORE_SOURCES CONFIGURE_DEPENDS "${SHARED_CORE_DIR}/figmashadow/*.cpp")

add_library(react-native-figma-shadow
  SHARED
  ${CORE_SOURCES}
  "${CMAKE_CURRENT_SOURCE_DIR}/src/main/jni/cpp-adapter.cpp"
)

target_include_directories(react-native-figma-shadow
  PUBLIC
  "${SHARED_CORE_DIR}"
  "${SHARED_CORE_DIR}/figmashadow"
)

find_library(log-lib log)
find_library(jnigraphics-lib jnigraphics)

target_link_libraries(react-native-figma-shadow
  ${log-lib}
  ${jnigraphics-lib}
)
