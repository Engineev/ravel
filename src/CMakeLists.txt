set(SOURCE main.cpp
  ${CMAKE_SOURCE_DIR}/include/ravel/assembler/assembler.h
  ${CMAKE_SOURCE_DIR}/include/ravel/assembler/object_file.h
  ${CMAKE_SOURCE_DIR}/include/ravel/assembler/parser.h
  ${CMAKE_SOURCE_DIR}/include/ravel/assembler/preprocessor.h

  ${CMAKE_SOURCE_DIR}/include/ravel/interpreter/cache.h
  ${CMAKE_SOURCE_DIR}/include/ravel/interpreter/interpreter.h
  ${CMAKE_SOURCE_DIR}/include/ravel/interpreter/libc_sim.h

  ${CMAKE_SOURCE_DIR}/include/ravel/linker/interpretable.h
  ${CMAKE_SOURCE_DIR}/include/ravel/linker/linker.h

  ${CMAKE_SOURCE_DIR}/include/ravel/container_utils.h
  ${CMAKE_SOURCE_DIR}/include/ravel/error.h
  ${CMAKE_SOURCE_DIR}/include/ravel/instructions.h
  ${CMAKE_SOURCE_DIR}/include/ravel/simulator.h

  assembler/assembler.cpp
  assembler/parser.cpp
  assembler/preprocessor.cpp

  interpreter/cache.cpp
  interpreter/interpreter.cpp
  interpreter/libc_sim.cpp

  linker/linker.cpp

  simulator.cpp
  )

add_executable(ravel ${SOURCE})

target_compile_features(ravel PRIVATE cxx_std_17)
if (UNIX)
  target_compile_options(ravel PRIVATE -O2 -Wall)
endif ()

target_include_directories(ravel PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(ravel
  PRIVATE -DCMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

install(TARGETS ravel DESTINATION bin)

