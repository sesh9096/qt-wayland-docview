cmake_minimum_required (VERSION 3.29)

set (PROJECT_NAME "QtDocview")

project (${PROJECT_NAME})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_executable (${PROJECT_NAME} main.cpp)

target_link_libraries (${PROJECT_NAME} Qt6::Widgets)

# set_target_properties (${TEMPLATE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TEMPLATE_DIR})
set_target_properties (${TEMPLATE_NAME} PROPERTIES CXX_STANDARD 17)
# target_link_libraries (${TEMPLATE_NAME} Vulkan::cppm glfw)
# target_include_directories (${TEMPLATE_NAME} PRIVATE ${STB_INCLUDEDIR})

# Local Variables:
# compile-command: "cmake -B build -G Ninja"
# End:
