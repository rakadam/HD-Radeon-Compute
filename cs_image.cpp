/*
 * Copyright 2011 StreamNovation Ltd. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY StreamNovation Ltd. ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL StreamNovation Ltd. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of StreamNovation Ltd.
 *
 *
 * Author(s):
 *          Adam Rak <adam.rak@streamnovation.com>
 *    
 *    
 *    
 */

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
  num_gprs = 16;
  temp_gprs = 4;
  global_gprs = 0;
  stack_size = 3;
  thread_num = 8;
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
