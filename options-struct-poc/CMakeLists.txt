cmake_minimum_required(VERSION 3.25)

project(
  OptionsPoc
  VERSION 0.1
  LANGUAGES CXX)

add_custom_command(
  COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/structparse.py
          ${CMAKE_CURRENT_SOURCE_DIR}/options.h "options_ns::*options"
          --out-json=options-struct.json
          --include ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/structparse.py
          ${CMAKE_CURRENT_SOURCE_DIR}/options.h
  OUTPUT options-struct.json
  COMMENT "Generating options struct json"
)

add_custom_command(
  COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/structio.py
          options-struct.json "options_ns::*options"
          ${CMAKE_CURRENT_SOURCE_DIR}/options-structio.h
          ${CMAKE_CURRENT_SOURCE_DIR}/options-structio.cpp
          --include="options.h"
          --include="options-io.h"
          --parse="std::string\;from_string\;options_ns::parse_%"
          --format="std::string\;to_string\;options_ns::format_%"
          --parse="json\;from_json\;options_ns::json_to_%"
          --key-sep="."
  DEPENDS options-struct.json
          ${CMAKE_CURRENT_SOURCE_DIR}/structio.py
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/options-structio.h
         ${CMAKE_CURRENT_SOURCE_DIR}/options-structio.cpp
  COMMENT "Generating structio code"
)

add_library(OptionsSkio options.h options-io.h options-io.cpp options-struct.json options-structio.h options-structio.cpp)
target_include_directories(OptionsSkio PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

add_executable(App app.cpp)
target_link_libraries(App PRIVATE OptionsSkio)
