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

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include "r800_state.h"

#include "evergreen_reg.h"

using namespace std;

int main()
{
  char* buf;
  assert(drmAvailable());

  int fd = open("/dev/dri/card0", O_RDWR, 0);

  r800_state state(fd, false);
  state.set_default_state();
  
  compute_shader sh(&state, "first_cs.bin");
  radeon_bo* buffer = state.bo_open(0, 1024*1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  radeon_bo_map(buffer, 1);
  
  uint32_t *ptr = (uint32_t*)buffer->ptr;

  for (int i = 0; i < 1024*2; i++)
  {
    ptr[i] = 0xF;
  } 
  
  radeon_bo_unmap(buffer);

  radeon_bo* buffer2 = state.bo_open(0, 1024*1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  radeon_bo_map(buffer2, 1);
  ptr = (uint32_t*)buffer2->ptr;

  for (int i = 0; i < 1024; i++)
  {
    ptr[i] = i;
  } 
  
  radeon_bo_unmap(buffer2);
  uint32_t w = RADEON_GEM_DOMAIN_VRAM;

  {
    vtx_resource_t vtxr;
    
    memset(&vtxr, 0, sizeof(vtxr));
    
    vtxr.id = SQ_FETCH_RESOURCE_cs; //SQ_FETCH_RESOURCE_vs;
    vtxr.stride_in_dw = 1;
    vtxr.size_in_dw = 256;
    vtxr.vb_offset = 0;
    vtxr.bo = buffer2;
    vtxr.dst_sel_x       = SQ_SEL_X;
    vtxr.dst_sel_y       = SQ_SEL_Y;
    vtxr.dst_sel_z       = SQ_SEL_Z;
    vtxr.dst_sel_w       = SQ_SEL_W;
    vtxr.endian = SQ_ENDIAN_NONE;
    vtxr.num_format_all = SQ_NUM_FORMAT_INT;
    vtxr.format = FMT_32_32_32_32;
//     vtxr.uncached = true;
//     vtxr.format_comp_all = true;
    
    state.set_vtx_resource(&vtxr, RADEON_GEM_DOMAIN_VRAM);
  }
  
  state.set_kms_compute_mode(true);

  {
    state.set_rat(11, buffer, 0, 1024*1024);
    state.set_gds(0, 100);
//     state.setup_const_cache(0, buffer2, 0, 16*1024);
//     state.setup_const_cache(1, buffer2, 0, 16*1024);
    state.set_tmp_ring(NULL, 0, 0);
    state.set_lds(0, 0, 0);
    state.load_shader(&sh);
    state.direct_dispatch({1, 1}, {4, 4});

//     state.set_surface_sync(CB_ACTION_ENA_bit | CB11_DEST_BASE_ENA_bit, 1024*1024, 0, buffer, /*RADEON_GEM_DOMAIN_VRAM*/0, RADEON_GEM_DOMAIN_VRAM); 

    cout << "start kernel" << endl;
    state.flush_cs();
  }
  
  radeon_bo_wait(buffer);
  
//   radeon_bo_wait(buffer);

  while (radeon_bo_is_busy(buffer, &w))
  {
    sleep(1);
    printf(".\n");
  }
  
  state.set_kms_compute_mode(false);

  radeon_bo_map(buffer, 0);
  printf("mapped\n");

  ptr = (uint32_t*)buffer->ptr;

  for (int i = 0; i < 16; i++)
  {
    printf("%X ", ptr[i]);
    if (i%4 == 3)
    {
      printf("\n");
    }
  }

  radeon_bo_unmap(buffer);

  cout << endl;

//   radeon_bo_map(buffer2, 0);
// 
//   ptr = (uint32_t*)buffer2->ptr;
// 
//   for (int i = 0; i < 256; i++)
//   {
//     printf("%X ", ptr[i]);
//   }
// 
//   radeon_bo_unmap(buffer2);
// 
//   cout << endl;


  radeon_bo_unref(buffer);
  radeon_bo_unref(buffer2);
//   drmClose(fd); //r800_state will close it
}

