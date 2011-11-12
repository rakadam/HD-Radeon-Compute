ADD_CUSTOM_COMMAND(OUTPUT shader.bin 
  COMMAND ../../as_r800 shader.asm
  DEPENDS shader.asm)


ADD_EXECUTABLE(test_10 ${test_files})
TARGET_LINK_LIBRARIES(test_10 r800_compute drm_radeon drm)

add_test(test_10 test_10)

