cmake_minimum_required(VERSION 3.23)

project(AdVisual)

find_package(wxWidgets 3.2 REQUIRED)

add_executable(AdVisual src/main.cpp)
file(GLOB_RECURSE src_files CONFIGURE_DEPENDS "src/*.cpp" "src/*.c")
target_sources(AdVisual PUBLIC ${src_files})
target_include_directories(AdVisual PUBLIC "include")
target_include_directories(AdVisual PUBLIC "src")
link_directories(${CMAKE_SOURCE_DIR}/lib)

include(UsewxWidgets)
target_link_libraries(AdVisual PRIVATE ${wxWidgets_LIBRARIES})
target_link_libraries(AdVisual PRIVATE adplug)

target_compile_definitions(AdVisual PUBLIC ASSETS_PATH="${CMAKE_INSTALL_PREFIX}/share/AdVisual/assets/")

install(TARGETS AdVisual DESTINATION bin)

install(DIRECTORY assets DESTINATION share/AdVisual
	FILE_PERMISSIONS WORLD_READ OWNER_WRITE OWNER_READ GROUP_READ GROUP_WRITE
	DIRECTORY_PERMISSIONS WORLD_READ WORLD_EXECUTE OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE)

