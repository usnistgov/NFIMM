project(NFIMM_bin)

if(_WIN32_64)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_64")

  # message(STATUS "BIN: CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
  message(STATUS "BIN: CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable( ${PROJECT_NAME}
    nfimm_bin.cpp
  )
else()
  # message(STATUS "BIN: CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
  message(STATUS "BIN: CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(${PROJECT_NAME}
    nfimm_bin.cpp
  )
endif()

message(STATUS "BIN: CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "BIN: CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(${PROJECT_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(NFIMM_bin NFIMM_ITL)

target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

get_property(inc_dirs TARGET ${PROJECT_NAME} PROPERTY INCLUDE_DIRECTORIES)
message(STATUS "${PROJECT_NAME} include dirs =>> ${inc_dirs}")
