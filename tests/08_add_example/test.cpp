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

#include <r800_state.h>
#include <cs_image.h>
#include <evergreen_reg.h>
#include <iostream>
#include <stdexcept>

using namespace std;

void do_test(r800_state& state)
{
  compute_shader sh(&state, "shader.bin");
  state.set_kms_compute_mode(true);
  radeon_bo* write_buffer = state.bo_open(0, 1024*1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  radeon_bo_map(write_buffer, 1);
  
  uint32_t *ptr = (uint32_t*)write_buffer->ptr;

  for (int i = 0; i < 1024*256; i++)
  {
    ptr[i] = 0xFF; ///< Fill it with a default value
  } 
  
  radeon_bo_unmap(write_buffer);

  radeon_bo* read_buffer = state.bo_open(0, 1024*1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  radeon_bo_map(read_buffer, 1);
  
  ptr = (uint32_t*)read_buffer->ptr;

  for (int i = 0; i < 1024; i++)
  {
    ptr[i] = 0;
  } 
  
  cout << "give two int32 numbers: " << endl;
  
  cin >> ptr[0] >> ptr[1];
  
  radeon_bo_unmap(read_buffer);

  vtx_resource_t vtxr;

  memset(&vtxr, 0, sizeof(vtxr));

  vtxr.id = SQ_FETCH_RESOURCE_cs;
  vtxr.stride_in_dw = 1;
  vtxr.size_in_dw = 256;
  vtxr.vb_offset = 0;
  vtxr.bo = read_buffer;
  vtxr.dst_sel_x       = SQ_SEL_X;
  vtxr.dst_sel_y       = SQ_SEL_Y;
  vtxr.dst_sel_z       = SQ_SEL_Z;
  vtxr.dst_sel_w       = SQ_SEL_W;
  vtxr.endian = SQ_ENDIAN_NONE;
  vtxr.num_format_all = SQ_NUM_FORMAT_INT;
  vtxr.format = FMT_32_32_32_32;

  state.set_vtx_resource(&vtxr, RADEON_GEM_DOMAIN_VRAM); ///< for vertex read
  state.set_rat(11, write_buffer, 0, 1024*1024); ///< For RAT write
  
  state.set_gds(0, 0);
  state.set_tmp_ring(NULL, 0, 0);
  state.set_lds(0, 0, 0);
  state.load_shader(&sh);
  state.direct_dispatch({1}, {1});

  cerr << "start kernel" << endl;
  state.flush_cs();
  
  radeon_bo_wait(write_buffer);
  
  radeon_bo_map(write_buffer, 0);
  
  ptr = (uint32_t*)write_buffer->ptr;

  cout << "result:" << endl;
  
  cout << ptr[0] << endl;
  
  radeon_bo_unmap(write_buffer);  
  
  radeon_bo_unref(write_buffer);
  radeon_bo_unref(read_buffer);
}
