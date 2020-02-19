set(SOURCE main.cpp
  argparse.cpp
  argparse.h
  assembler.cpp
  assembler.h
  container_utils.h
  instructions.h
  interpretable.h
  interpreter.cpp
  interpreter.h
  libc_sim.cpp
  libc_sim.h
  linker.cpp
  linker.h
  object_file.h
  parser.cpp
  parser.h
  )

add_executable(ravel ${SOURCE})

target_compile_features(ravel PRIVATE cxx_std_17)
if (UNIX)
  target_compile_options(ravel PRIVATE -O2 -Wall)
endif ()

target_include_directories(ravel PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(ravel
  PRIVATE -DCMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
