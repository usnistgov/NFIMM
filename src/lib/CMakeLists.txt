
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

#FILE(GLOB sources ${CMAKE_CURRENT_SOURCE_DIR}/**/*.cpp)
#add_library( ${PROJECT_NAME} ${sources} )
add_library( ${PROJECT_NAME}
   nfimm_lib.cpp
   metadata.cpp
   bmp/bmp.cpp
   bmp/file_header.cpp
   bmp/info_header.cpp
   png/crc_public_code.cpp
   png/ihdr.cpp
   png/phys.cpp
   png/png.cpp
   png/signature.cpp
   png/text.cpp
 )

# warning C4996: 'gmtime': This function or variable may be unsafe. Consider using gmtime_s instead.
# To disable deprecation, use _CRT_SECURE_NO_WARNINGS.
# warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead.
# To disable deprecation, use _CRT_SECURE_NO_WARNINGS.
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
