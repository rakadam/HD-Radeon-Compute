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
  
  int mx = 15;
  int my = 14;
  
  uint32_t *ptr = (uint32_t*)buffer->ptr;

  for (int i = 0; i < 256; i++)
  {
    ptr[i] = 255;
  }
  
  radeon_bo_unmap(buffer);
  
  radeon_bo* param_buf = state.bo_open(0, 1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  radeon_bo_map(param_buf, 1);
  ptr = (uint32_t*)param_buf->ptr;
  
  ptr[0] = mx;
  
  radeon_bo_unmap(param_buf);

  state.setup_const_cache(0, param_buf, 256, 0); ///< we pass mx as a constant parameter to the shader
  state.set_rat(11, buffer, 0, 1024*1024);
  state.set_gds(0, 0);
  state.set_tmp_ring(NULL, 0, 0);
  state.set_lds(0, 0, 0);
  state.load_shader(&sh);
  state.direct_dispatch({1}, {mx, my});

  cerr << "start kernel" << endl;
  state.flush_cs();
  
  radeon_bo_wait(buffer);
  
  radeon_bo_map(buffer, 0);
  
  ptr = (uint32_t*)buffer->ptr;

  int x = 0;
  int y = 0;
  
  for (int i = 0; i < mx*my; i++)
  {
    printf("%4i", ptr[i]);
    
    if (ptr[i] != (x+y))
    {
      throw runtime_error("GPU output does not match reference pattern");
    }
    
    if (i%mx == mx-1)
    {
      cout << endl;
      y++;
      x = 0;
    }
    else
    {
      x++;
    }
  }
  
  cout << endl;
  
  radeon_bo_unmap(buffer);
  
  radeon_bo_unref(buffer);
  
  cerr << "OK" << endl;
}
