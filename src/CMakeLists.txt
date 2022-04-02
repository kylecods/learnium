file(GLOB_RECURSE SOURCE_FILES "*.c")
file(GLOB_RECURSE HEADER_FILES "*.h")

add_library(ln_libs ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(ln_libs PUBLIC ../include)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Header files"
    FILES ${HEADERS}
)