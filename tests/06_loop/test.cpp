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
#include <iostream>
#include <stdexcept>

using namespace std;

void do_test(r800_state& state)
{
  compute_shader sh(&state, "shader.bin");
  state.set_kms_compute_mode(true);
  radeon_bo* buffer = state.bo_open(0, 1024*1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  radeon_bo_map(buffer, 1);
  
  uint64_t *ptr = (uint64_t*)buffer->ptr;

  ptr[0] = 0;
  ptr[1] = 0;
  
  radeon_bo_unmap(buffer);

  loop_const loop;
  
  loop.count = 100;
  loop.init = 0;
  loop.inc = 1;
  
  state.set_loop_consts({loop});

  state.set_rat(11, buffer, 0, 1024*1024); ///< We use it to return time measurements
  state.set_gds(0, 0);
  state.set_tmp_ring(NULL, 0, 0);
  state.set_lds(0, 0, 0);
  state.load_shader(&sh);
  state.direct_dispatch({1}, {1});

  cerr << "start kernel" << endl;
  state.flush_cs();
  
  radeon_bo_wait(buffer);
  
  radeon_bo_map(buffer, 0);
  
  ptr = (uint64_t*)buffer->ptr;
  
  if (ptr[0] == 0)
  {
    throw runtime_error("Shader failed to execute");
  }
  
  cout << "start time: " << ptr[0] << " stop time: " << ptr[1] << " in cycles" << endl;
  cout << "run time: " << ptr[1] - ptr[0] << " cycles" << endl;
  
  int cycles_per_iteration = (ptr[1] - ptr[0]) / uint64_t(loop.count);
  
  cout << "cycles per loop iteration: " << cycles_per_iteration << endl;
  
  if (cycles_per_iteration < 5)
  {
    throw runtime_error("Abnormally low cycle loop time");
  }
  
  if (cycles_per_iteration > 200)
  {
    throw runtime_error("Abnormally high cycle loop time");
  }
  
  radeon_bo_unmap(buffer);
  
  radeon_bo_unref(buffer);
  
  cerr << "OK" << endl;
}
