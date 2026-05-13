cmake_minimum_required(VERSION 3.26)
cmake_policy(VERSION 3.26)
cmake_policy(SET CMP0207 NEW)

project(advisual LANGUAGES C CXX VERSION 1.0)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)

add_executable(advisual src/main.cpp)
file(GLOB_RECURSE src_files CONFIGURE_DEPENDS "src/*.cpp" "src/*.c")
#set(src_files "src/main.cpp")
target_sources(advisual PUBLIC ${src_files})
#target_sources(advisual PRIVATE windows_resource.rc)
target_include_directories(advisual PUBLIC "include")
target_include_directories(advisual PUBLIC "src")
target_link_directories(advisual PUBLIC ${CMAKE_SOURCE_DIR}/lib)
message(STATUS "Adding ${CMAKE_SOURCE_DIR}/lib to library link path.")

if (NOT TARGET libbinio)
	find_package(libbinio REQUIRED CONFIG)
endif()

if (NOT TARGET adplug)
	find_package(adplug REQUIRED CONFIG)
endif()

target_link_libraries(advisual PRIVATE adplug::adplug)
target_link_libraries(advisual PRIVATE libbinio::libbinio)

# Include gtkmm-4.0

link_directories(${GTKMM_LIBRARY_DIRS})
include_directories(${GTKMM_INCLUDE_DIRS})
add_definitions(${GTKMM_CFLAGS_OTHER})
target_link_libraries(advisual PRIVATE ${GTKMM_LIBRARIES})

target_compile_definitions(advisual PUBLIC ICONS_PATH="${CMAKE_INSTALL_PREFIX}/share/advisual/icons/")
target_compile_definitions(advisual PUBLIC THEMES_PATH="${CMAKE_INSTALL_PREFIX}/share/advisual/themes/")

# Install first party executable, icons and themes.
install(TARGETS advisual DESTINATION bin
	PERMISSIONS WORLD_READ OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_EXECUTE)

install(DIRECTORY icons DESTINATION share/advisual
	FILE_PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ GROUP_WRITE WORLD_READ
	DIRECTORY_PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(DIRECTORY themes DESTINATION share/advisual
	FILE_PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ GROUP_WRITE WORLD_READ WORLD_EXECUTE
	DIRECTORY_PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

set(COPY_GTKMM_LIBRARIES OFF CACHE BOOL "Meant to be used for MSYS2 for windows")
if (COPY_GTKMM_LIBRARIES)
	install( CODE [[
		file(GET_RUNTIME_DEPENDENCIES
			LIBRARIES
				$<TARGET_FILE:advisual>
			RESOLVED_DEPENDENCIES_VAR _r_deps
			UNRESOLVED_DEPENDENCIES_VAR _u_deps
			DIRECTORIES
				"C:/msys64/mingw64/bin"
		)
		foreach(_file ${_r_deps})
			string(TOLOWER ${_file} _file_lower)
			if(NOT ${_file_lower} MATCHES "c:[\\/]windows[\\/]system32.*")
				file(INSTALL
					DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
					TYPE SHARED_LIBRARY
					FOLLOW_SYMLINK_CHAIN
					FILES "${_file}"
				)
			endif()
		endforeach()
		message("UNRESOLVED_DEPENDENCIES_VAR: ${_u_deps}")
	]])
	set(PIXBUF_DEPENDS
		"librsvg-2-2.dll"
		"libxml2-16.dll"
		"libiconv-2.dll"
		"libcharset-1.dll"
		"zlib1.dll"
	)
	list(TRANSFORM PIXBUF_DEPENDS PREPEND "${GTKMM_LIBRARY_DIRS}/../bin/")
	install(FILES ${PIXBUF_DEPENDS} DESTINATION bin)

	install(DIRECTORY "${GTKMM_LIBRARY_DIRS}/gdk-pixbuf-2.0" DESTINATION lib
		FILE_PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ GROUP_WRITE WORLD_READ
		DIRECTORY_PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

include(CPack)