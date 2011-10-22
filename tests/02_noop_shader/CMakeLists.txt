ADD_CUSTOM_COMMAND(OUTPUT shader.bin 
  COMMAND ../../as_r800 shader.asm
  DEPENDS shader.asm)


ADD_EXECUTABLE(test_02 ${test_files})
TARGET_LINK_LIBRARIES(test_02 r800_compute drm_radeon drm)