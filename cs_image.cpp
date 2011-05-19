#include <cstdio>
extern "C" {
#include <drm.h>
#include <xf86drm.h>
#include <radeon_cs_gem.h>
#include <radeon_bo_gem.h>
};
#include <assert.h>
#include "cs_image.h"
#include "r800_state.h"

compute_shader::compute_shader(r800_state* state, const std::vector<char>& binary)
{
  const cs_image_header* header = (const cs_image_header*)&binary[0];
  
  assert(binary.size()%4 == 0);
  assert(header->magic == 0x42424242);

  uint32_t sum = 0;
  
  for (int i = 8; i < binary.size(); i++)
  {
    sum = sum + binary[i];
  }
  
  assert(header->chksum == sum);
  
  lds_alloc = header->lds_alloc;
  num_gprs = header->num_gprs;
  temp_gprs = header->temp_gprs;
  global_gprs = header->global_gprs;
  stack_size = header->stack_size;
  thread_num = header->thread_num;
  dyn_gpr_limit = header->dyn_gpr_limit;

  int shader_start = sizeof(cs_image_header);
  
  alloc_size = binary.size() - shader_start;
  
  if (alloc_size % 16)
  {
    alloc_size += (16 - alloc_size % 16);
  }
  
  binary_code_bo = state->bo_open(0, alloc_size, 0, RADEON_GEM_DOMAIN_VRAM, 0);
  
  assert(binary_code_bo != NULL);
  
  assert(radeon_bo_map(binary_code_bo, 1) == 0);
  
  memcpy(binary_code_bo->ptr, &binary[0] + shader_start, binary.size() - shader_start);
  
  radeon_bo_unmap(binary_code_bo);
}

compute_shader::compute_shader(r800_state* state, std::string fname)
{
  FILE *f = fopen(fname.c_str(), "r");
  
  assert(f);
  
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  
  char* buf = new char[size];
  fread(buf, 1, size, f);
  
  fclose(f); 
  
  lds_alloc = 0;
  num_gprs = 4;
  temp_gprs = 4;
  global_gprs = 0;
  stack_size = 4;
  thread_num = 16;
  dyn_gpr_limit = 0;
  
  alloc_size = size;
  
  binary_code_bo = state->bo_open(0, size, 0, RADEON_GEM_DOMAIN_VRAM, 0);
  
  assert(binary_code_bo != NULL);
  
  assert(radeon_bo_map(binary_code_bo, 1) == 0);
  
  memcpy(binary_code_bo->ptr, buf, size);
  
  radeon_bo_unmap(binary_code_bo);
}

// compute_shader cs_read_from_file(std::string fname)
// {
//   FILE *f = fopen(fname.c_str(), "r");
//   
//   fread()
//   
//   fclose(f);
// }
