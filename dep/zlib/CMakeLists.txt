project(zlib)

set(ZLIB_HEADERS_LIST
    crc32.h
    deflate.h
    gzguts.h
    inffast.h
    inffixed.h
    inflate.h
    inftrees.h
    trees.h
    zutil.h)

set(ZLIB_SOURCES_LIST
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    inflate.c
    infback.c
    inftrees.c
    inffast.c
    trees.c
    uncompr.c
    zutil.c
    zip.c
    ioapi.c
)

add_library(${PROJECT_NAME} STATIC ${ZLIB_SOURCES_LIST})

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Expose public includes to other through cache variable.
set(ZLIB_INCLUDE_DIR ${PROJECT_SOURCE_DIR}
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

set(ZLIB_LIBRARY ${PROJECT_NAME}
    CACHE INTERNAL "${PROJECT_NAME}: Library Name" FORCE)
