SET(test_files
  ../main.cpp
  test.cpp
  shader.bin
)


add_subdirectory(01_basic_no_shader)
add_subdirectory(02_noop_shader)
add_subdirectory(03_single_copy)
add_subdirectory(04_multi_copy)
add_subdirectory(05_atomics)
add_subdirectory(06_loop)
add_subdirectory(07_work_2d)
add_subdirectory(08_add_example)
add_subdirectory(testpr)
