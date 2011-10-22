ADD_CUSTOM_COMMAND(OUTPUT shader.bin 
  COMMAND ../../as_r800 shader.asm
  DEPENDS shader.asm)


ADD_EXECUTABLE(test_05 ${test_files})
TARGET_LINK_LIBRARIES(test_05 r800_compute drm_radeon drm)