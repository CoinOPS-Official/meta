cmake_minimum_required(VERSION 3.16)
project(xmlcheck LANGUAGES CXX)

add_executable(xmlcheck XmlGameDuplicateFinder.cpp)
target_compile_features(xmlcheck PRIVATE cxx_std_17)

# Put rapidxml.hpp here: tools/xmlcheck/rapidxml/rapidxml.hpp
target_include_directories(xmlcheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/rapidxml)
