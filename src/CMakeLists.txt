set(SOURCE main.cpp)

add_executable(ravel ${SOURCE})

target_compile_features(ravel PRIVATE cxx_std_14)
if (UNIX)
  target_compile_options(ravel PRIVATE -O2 -Wall)
endif ()

target_include_directories(ravel PRIVATE ${CMAKE_CURRENT_LIST_DIR})
