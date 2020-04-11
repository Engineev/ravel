set(SOURCE main.cpp
  assembler/assembler.cpp
  assembler/assembler.h
  assembler/object_file.h
  assembler/parser.cpp
  assembler/parser.h
  assembler/preprocessor.cpp
  assembler/preprocessor.h

  interpreter/cache.cpp
  interpreter/cache.h
  interpreter/interpreter.cpp
  interpreter/interpreter.h
  interpreter/libc_sim.cpp
  interpreter/libc_sim.h

  linker/interpretable.h
  linker/linker.cpp
  linker/linker.h

  container_utils.h
  error.h
  instructions.h
  simulator.cpp
  simulator.h
  )

add_executable(ravel ${SOURCE})

target_compile_features(ravel PRIVATE cxx_std_17)
if (UNIX)
  target_compile_options(ravel PRIVATE -O2 -Wall)
endif ()

target_include_directories(ravel PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(ravel
  PRIVATE -DCMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

install(TARGETS ravel DESTINATION bin)

