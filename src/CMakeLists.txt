set(HEADERS
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
    ${CMAKE_SOURCE_DIR}/include/ravel/ravel.h
    ${CMAKE_SOURCE_DIR}/include/ravel/simulator.h
)

set(SOURCES
    assembler/assembler.cpp
    assembler/parser.cpp
    assembler/preprocessor.cpp

    interpreter/cache.cpp
    interpreter/interpreter.cpp
    interpreter/libc_sim.cpp

    linker/linker.cpp

    simulator.cpp
  )

add_library(ravel-sim ${HEADERS} ${SOURCES})
target_compile_features(ravel-sim PUBLIC cxx_std_17)
target_include_directories(ravel-sim
    PUBLIC
      $<INSTALL_INTERFACE:include>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
if (UNIX)
  target_compile_options(ravel-sim PRIVATE -O2 -Wall)
endif ()
include(GNUInstallDirs)
install(TARGETS ravel-sim LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/ravel" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


add_executable(ravel main.cpp)
target_link_libraries(ravel PRIVATE ravel-sim)
target_compile_features(ravel PRIVATE cxx_std_17)
if (UNIX)
  target_compile_options(ravel PRIVATE -O2 -Wall)
endif ()

install(TARGETS ravel DESTINATION ${CMAKE_INSTALL_BINDIR})

