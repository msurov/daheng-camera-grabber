if (NOT EXISTS "/usr/lib/libgxiapi.so")
    set(GXI_FOUND FALSE)
    set(GXI_NOTFOUND TRUE)
    return()
endif()

set(GXI_FOUND TRUE)
set(GXI_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include")
set(GXI_LIBRARIES "gxiapi")
