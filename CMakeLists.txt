cmake_minimum_required(VERSION 2.8.7)

project(org.webosinternals.service.update C)

include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC2)
webos_component(0 1 0)

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS})

pkg_check_modules(GIO2 REQUIRED gio-2.0)
include_directories(${GIO2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GIO2_CFLAGS})

pkg_check_modules(GIO-UNIX REQUIRED gio-unix-2.0)
include_directories(${GIO-UNIX_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GIO-UNIX_CFLAGS_OTHER})

pkg_check_modules(GOBJECT2 REQUIRED gobject-2.0)
include_directories(${GOBJECT2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GOBJECT2_CFLAGS})

pkg_check_modules(LUNASERVICE2 REQUIRED luna-service2)
include_directories(${LUNASERVICE2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LUNASERVICE2_CFLAGS})

pkg_check_modules(PBNJSON_C REQUIRED pbnjson_c)
include_directories(${PBNJSON_C_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_C_CFLAGS_OTHER})

file(GLOB SOURCE_FILES src/*.c)

webos_add_compiler_flags(ALL -Wall)
webos_add_linker_options(ALL --no-undefined)

add_executable(org.webosinternals.service.update ${SOURCE_FILES})
target_link_libraries(org.webosinternals.service.update
    ${GLIB2_LDFLAGS} ${LUNASERVICE2_LDFLAGS} ${PBNJSON_C_LDFLAGS}
    ${GIO2_LDFLAGS} ${GIO-UNIX_LDFLAGS} ${GOBJECT2_LDFLAGS}
    rt pthread)

webos_build_daemon()
webos_build_system_bus_files()
