ADD_CUSTOM_COMMAND(OUTPUT shader.bin
  COMMAND ../../as_r800 shader.asm
  DEPENDS shader.asm)


ADD_EXECUTABLE(mytest ${test_files})
TARGET_LINK_LIBRARIES(mytest r800_compute drm_radeon drm)

