set(C74_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../../externals")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-pretarget.cmake)

#############################################################
# MAX EXTERNAL
#############################################################

include_directories( 
	"${MAX_SDK_INCLUDES}"
	"${MAX_SDK_MSP_INCLUDES}"
	"${MAX_SDK_JIT_INCLUDES}"
)

file(GLOB PROJECT_SRC
     "*.h"
	 "*.c"
     "*.cpp"
)

message("Generating: taglib")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/taglib)

find_package(ZLIB REQUIRED)
find_package(Iconv REQUIRED)

add_library( 
	${PROJECT_NAME} 
	MODULE
	${PROJECT_SRC}
)

MACRO(HEADER_DIRECTORIES return_list)
    FILE(GLOB_RECURSE new_list *.h)
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()

HEADER_DIRECTORIES(header_dir_list)

list(LENGTH header_dir_list header_dir_list_count)
message(STATUS "[INFO] Found ${header_dir_list_count} header directories.")
message(STATUS "${CMAKE_BINARY_DIR}}/source/projects/metatag/taglib")

target_include_directories(${PROJECT_NAME}
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/taglib
	PRIVATE ${CMAKE_BINARY_DIR}/source/projects/metatag/taglib
	PRIVATE ${header_dir_list}
)

target_link_libraries(${PROJECT_NAME}
#	PRIVATE ZLIB::ZLIB
#	PRIVATE Iconv::Iconv
	PRIVATE tag
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-posttarget.cmake)
