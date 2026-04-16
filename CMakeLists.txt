cmake_minimum_required(VERSION 3.23)

project(AdVisual)

find_package(wxWidgets 3.2.9 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

add_executable(AdVisual src/main.cpp)
file(GLOB_RECURSE src_files CONFIGURE_DEPENDS "src/*.cpp" "src/*.c")
target_sources(AdVisual PUBLIC ${src_files})
target_sources(AdVisual PRIVATE windows_resource.rc)
target_include_directories(AdVisual PUBLIC "include")
target_include_directories(AdVisual PUBLIC "src")
target_link_directories(AdVisual PUBLIC ${CMAKE_SOURCE_DIR}/lib)
message(STATUS "Adding ${CMAKE_SOURCE_DIR}/lib to library link path.")

if (NOT TARGET libbinio)
	find_package(libbinio REQUIRED CONFIG)
endif()

if (NOT TARGET adplug)
	find_package(adplug REQUIRED CONFIG)
endif()

include(UsewxWidgets)
target_link_libraries(AdVisual PRIVATE ${wxWidgets_LIBRARIES})
target_link_libraries(AdVisual PRIVATE adplug::adplug)
target_link_libraries(AdVisual PRIVATE libbinio::libbinio)

# include gtk4.
include_directories(${GTK4_INCLUDE_DIRS})
link_directories(${GTK4_LIBRARY_DIRS})
add_definitions(${GTK4_CFLAGS_OTHER})

target_compile_definitions(AdVisual PUBLIC ASSETS_PATH="${CMAKE_INSTALL_PREFIX}/share/AdVisual/assets/")

install(TARGETS AdVisual DESTINATION bin
	PERMISSIONS WORLD_READ OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_EXECUTE)

install(DIRECTORY assets DESTINATION share/AdVisual
	FILE_PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ GROUP_WRITE WORLD_READ
	DIRECTORY_PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
