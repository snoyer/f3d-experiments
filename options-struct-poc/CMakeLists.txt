cmake_minimum_required(VERSION 3.25)

project(
  OptionsPoc
  VERSION 0.1
  LANGUAGES CXX)

  
add_custom_command(
  COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/structio.py
          ${CMAKE_CURRENT_SOURCE_DIR}/options.h "options_ns::f3d_options"
          options-structio.h options-structio.cpp
          --namespace="options_ns::generated"
          --include="options.h"
          --include="options-io.h"
          --parse="std::string\;from_string\;options_ns::parse_%"
          --format="std::string\;to_string\;options_ns::format_%"
          --parse="json\;from_json\;options_ns::json_to_%"
          --key-sep="."
          --compiler=${CMAKE_CXX_COMPILER}
          --generate-json=options-structio.json
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/structio.py
          ${CMAKE_CURRENT_SOURCE_DIR}/options.h
  OUTPUT options-structio.h
         options-structio.cpp
         options-structio.json
  COMMENT "Generating structio code"
)

add_library(OptionsSkio options.h options-io.h options-io.cpp options-structio.h options-structio.cpp)
target_include_directories(OptionsSkio PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

add_executable(App app.cpp)
target_link_libraries(App PRIVATE OptionsSkio)
