include(ExternalProject)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(WINDOWS_LINKER_OPTS ws2_32 wsock32 psapi iphlpapi userenv)
    set(CROSS_COMPILE_CFG --host=x86_64-w64-mingw32)
else()
    set(WINDOWS_LINKER_OPTS "")
    set(CROSS_COMPILE_CFG "")
endif()

set(LIBUV_ROOT ${CMAKE_BINARY_DIR}/third_party/libuv)
set(LIBUV_LIB_DIR ${LIBUV_ROOT}/src/uv/.libs)
set(LIBUV_INCLUDE_DIR ${LIBUV_ROOT}/src/uv/include)

ExternalProject_Add(uv_external
                PREFIX ${LIBUV_ROOT}
                GIT_REPOSITORY "https://github.com/libuv/libuv"
                GIT_TAG "1344d2bb82e195d0eafc0b40ba103f18dfd04cc5" #1.17.0
                UPDATE_COMMAND ""
                PATCH_COMMAND ""
                BINARY_DIR ${LIBUV_ROOT}/src/uv
                SOURCE_DIR ${LIBUV_ROOT}/src/uv
                INSTALL_DIR ${LIBUV_ROOT}/bin
                CONFIGURE_COMMAND ./configure --prefix=<INSTALL_DIR> ${CROSS_COMPILE_CFG}
                BUILD_COMMAND make
                BUILD_BYPRODUCTS ${LIBUV_LIB_DIR}/libuv.a
                INSTALL_COMMAND ""
                )

ExternalProject_Add_Step(uv_external
 autogen
 COMMAND sh autogen.sh
 DEPENDEES download
 DEPENDERS configure
 WORKING_DIRECTORY ${LIBUV_ROOT}/src/uv)

add_library(libuv STATIC IMPORTED)
set_target_properties(libuv PROPERTIES IMPORTED_LOCATION ${LIBUV_LIB_DIR}/libuv.a)
add_dependencies(libuv uv_external)
include_directories(${LIBUV_INCLUDE_DIR})