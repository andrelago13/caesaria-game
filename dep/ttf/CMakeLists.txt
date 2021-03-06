project(ttf)

#find_package(Freetype REQUIRED)
message("FREETYPE_DIR is" ${FREETYPELIB_HOME})

file(GLOB TTF_HEADERS_LIST "*.h")
file(GLOB TTF_SOURCES_LIST "*.c")

include_directories(
  ${SDL2MINI_INCLUDE_DIR}
  ${LIBPNG_INCLUDE_DIR}
  ${FREETYPE_INCLUDE_DIR}
)

if (${FREETYPE_INCLUDE_DIR_ft2build})
    include_directories(${FREETYPE_INCLUDE_DIR_ft2build})
endif()

add_library(${PROJECT_NAME} STATIC ${TTF_SOURCES_LIST})

target_link_libraries(${PROJECT_NAME} ${FREETYPE_LIBRARY})
#target_link_libraries(${PROJECT_NAME} ${PNG_LIBRARY})

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Expose public includes to other through cache variable.
set(SDL2_TTF_INCLUDE_DIR ${PROJECT_SOURCE_DIR}
                                 ${LIBPNG_INCLUDE_DIR}
                                 ${FREETYPE_INCLUDE_DIR}
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

set(SDL2_TTF_LIBRARY ${PROJECT_NAME}
    CACHE INTERNAL "${PROJECT_NAME}: Library Name" FORCE)
