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
#include <signal.h>
#include <sys/time.h>
       
using namespace std;

#define matwid 1024
#define dividor 16

struct timeval gettime()
{
    struct timeval tv;
    struct timezone tz;
    
    gettimeofday(&tv, &tz);

    return tv;
}

void do_test(r800_state& state)
{
  //state.soft_reset(); return;

  compute_shader sh(&state, "shader.bin");
  state.set_kms_compute_mode(true);
  radeon_bo* write_buffer = state.bo_open(0, 4*matwid*matwid, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  radeon_bo_map(write_buffer, 1);

  float *ptr = (float*)write_buffer->ptr;

  // Ausgabe-Matrix
  for (int i = 0; i < matwid*matwid; i++)
  {
    ptr[i] = 0.0f; ///< Fill it with a default value
  }

  radeon_bo_unmap(write_buffer);

  radeon_bo* read_buffer_1 = state.bo_open(0, 4*matwid*matwid, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  radeon_bo_map(read_buffer_1, 1);

  ptr = (float*)read_buffer_1->ptr;

  // Matrix1
  for (int i = 0; i < matwid*matwid; i++)
  {
    ptr[i] = i % matwid == i / matwid ? 1 : 0.1;;(rand()%1024) / 1023.0;
  }

  radeon_bo_unmap(read_buffer_1);

  radeon_bo* read_buffer_2 = state.bo_open(0, 4*matwid*matwid, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  radeon_bo_map(read_buffer_2, 1);

  ptr = (float*)read_buffer_2->ptr;

  // Transponierte zweite Matrix
  for (int i = 0; i < matwid*matwid; i++)
  {
    ptr[i] = i % 3;//(rand()%1024) / 1023.0;
  }
  radeon_bo_unmap(read_buffer_2);

  vtx_resource_t vtxr1;

  memset(&vtxr1, 0, sizeof(vtxr1));

  vtxr1.stride_in_dw = 1;
  vtxr1.size_in_dw = matwid*matwid;
  vtxr1.vb_offset = 0;
  vtxr1.dst_sel_x       = SQ_SEL_X;
  vtxr1.dst_sel_y       = SQ_SEL_Y;
  vtxr1.dst_sel_z       = SQ_SEL_Z;
  vtxr1.dst_sel_w       = SQ_SEL_W;
  vtxr1.endian = SQ_ENDIAN_NONE;
  vtxr1.num_format_all = SQ_NUM_FORMAT_SCALED;
  vtxr1.format = FMT_32_32_32_32;


  vtxr1.id = SQ_FETCH_RESOURCE_cs+0;
  vtxr1.bo = read_buffer_1;
  state.set_vtx_resource(&vtxr1, RADEON_GEM_DOMAIN_VRAM); ///< for vertex read


  vtx_resource_t vtxr2;

  memset(&vtxr2, 0, sizeof(vtxr2));

  vtxr2.stride_in_dw = 1;
  vtxr2.size_in_dw = matwid*matwid;
  vtxr2.vb_offset = 0;
  vtxr2.dst_sel_x       = SQ_SEL_X;
  vtxr2.dst_sel_y       = SQ_SEL_Y;
  vtxr2.dst_sel_z       = SQ_SEL_Z;
  vtxr2.dst_sel_w       = SQ_SEL_W;
  vtxr2.endian = SQ_ENDIAN_NONE;
  vtxr2.num_format_all = SQ_NUM_FORMAT_SCALED;
  vtxr2.format = FMT_32_32_32_32;

  vtxr2.id = SQ_FETCH_RESOURCE_cs+1;
  vtxr2.bo = read_buffer_2;
  state.set_vtx_resource(&vtxr2, RADEON_GEM_DOMAIN_VRAM); ///< for vertex read

  state.set_rat(11, write_buffer, 0, 4*matwid*matwid); ///< For RAT write

  /// set up param buffers
  radeon_bo* param_buf = state.bo_open(0, 1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  radeon_bo_map(param_buf, 1);
  uint32_t* cptr = (uint32_t*)param_buf->ptr;

  cptr[0] = matwid;
  cptr[1] = dividor;

  radeon_bo_unmap(param_buf);
  state.setup_const_cache(0, param_buf, 256, 0); // Matrix width

  loop_const myloop;
  myloop.count = matwid/4;
  myloop.init = 0;
  myloop.inc =  4;
  state.set_loop_consts({myloop});

  state.set_gds(0, 0);
  state.set_tmp_ring(NULL, 0, 0);
  state.set_lds(0, 0, 0);
  state.load_shader(&sh);
  /// set matrix thread count
  if(matwid <= dividor)
    state.direct_dispatch({1}, {matwid, matwid});
  else 
    state.direct_dispatch({matwid/dividor, matwid/dividor}, {dividor, dividor});

  cerr << "start kernel" << endl;
  //clock_t time1 = clock();
  
  timeval time1 = gettime();
 
  state.flush_cs();

  radeon_bo_wait(write_buffer);

  radeon_bo_map(write_buffer, 0);
  
  timeval time2 = gettime();
  
  long long dsec = (((long long)time2.tv_sec) - ((long long)time1.tv_sec));
  long long dusec = dsec*1000000 + ((long long)time2.tv_usec) - ((long long)time1.tv_usec);
  
  
  cout << "Execution time: " << (dusec) << "us" << endl;
  double ft = dusec;
  double oper = double(matwid)*double(matwid)*double(matwid)*2;

  cout << oper / ft / 1000.0 << "GFlop/s" << endl;
  cout << (oper*4) / ft / 1000.0 << "Gbyte/s" << endl;
  
  ptr = (float*)write_buffer->ptr;
/*
  cout << "result:" << endl;

  for(int i = 0; i < matwid; i++) {
    for(int j = 0; j < matwid; j++) {
      cout << ptr[matwid*i+j] << " ";
    }
    cout << endl;
  }
*/
  radeon_bo_unmap(write_buffer);

  radeon_bo_unref(write_buffer);
  radeon_bo_unref(read_buffer_1);
  radeon_bo_unref(read_buffer_2);
}
