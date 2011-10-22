ADD_CUSTOM_COMMAND(OUTPUT shader.bin 
  COMMAND ../../as_r800 shader.asm
  DEPENDS shader.asm)

ADD_EXECUTABLE(test_01 ${test_files})

TARGET_LINK_LIBRARIES(test_01 r800_compute drm_radeon drm)