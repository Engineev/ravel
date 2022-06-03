add_executable(ravel-sim-test test-ravel-sim.cpp)
target_link_libraries(ravel-sim-test PRIVATE ravel-sim)
target_compile_features(ravel-sim-test PRIVATE cxx_std_17)
if (UNIX)
#  target_compile_options(ravel-sim-test PRIVATE -O2 -Wall)
endif ()

