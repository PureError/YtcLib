file(GLOB HEADER_FILES ${ROOT_DIR}/include/*)
aux_source_directory(${ROOT_DIR}/src SOURCE_FILES_DIR)
add_library(YtcLib ${HEADER_FILES} ${SOURCE_FILES_DIR})