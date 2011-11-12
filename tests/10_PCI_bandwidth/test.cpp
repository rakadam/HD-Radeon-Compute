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
#include <sys/time.h>


struct timeval gettime()
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);

    return tv;
}

using namespace std;

void do_test(r800_state& state)
{
  compute_shader sh(&state, "shader.bin");
  state.set_kms_compute_mode(true);
  
  int num = 4;
  size_t size = 64*1024*1024;
  radeon_bo* buffer = state.bo_open(0, 128*1024*1024, 4096, RADEON_GEM_DOMAIN_VRAM, 0);

  cout << "Write:" << endl;
  
  timeval time1 = gettime();

  radeon_bo_map(buffer, 1);
  
  uint32_t *ptr = (uint32_t*)buffer->ptr;
    
  for (int j = 0; j < num; j++)
  {
    for (size_t i = 0; i < size/4; i+=4)
    {
      ptr[i] = j;
      ptr[i+1] = j;
      ptr[i+2] = j;
      ptr[i+3] = j;
    }
  }

  timeval time2 = gettime();
  
  radeon_bo_unmap(buffer);
  
  long long dsec = (((long long)time2.tv_sec) - ((long long)time1.tv_sec));
  long long dusec = dsec*1000000 + ((long long)time2.tv_usec) - ((long long)time1.tv_usec);
  
  cout << "Time: " << dusec << " us" << endl;
  cout << "speed: " << double(size*num) / double(dusec) / 1024.0 << " GBps" << endl;
  
  {
    state.set_rat(11, buffer, 0, 1024*1024); ///< We use it for radeon_bo_wait, but not in the shader code!
    state.set_gds(0, 0);
    state.set_tmp_ring(NULL, 0, 0);
    state.set_lds(0, 0, 0);
    state.load_shader(&sh);
    state.direct_dispatch({1}, {1});

    cerr << "start kernel" << endl;
    state.flush_cs();
  }
  
  radeon_bo_wait(buffer);
  
  cout << "Read:" << endl;
  
  time1 = gettime();
  radeon_bo_map(buffer, 0);
  
  ptr = (uint32_t*)buffer->ptr;
  uint32_t *tt = new uint32_t[size/4];
  
  for (int j = 0; j < num; j++)
  {
    for (size_t i = 0; i < size/4; i+=4)
    {
      tt[i] = ptr[i];
      tt[i+1] = ptr[i+1];
      tt[i+2] = ptr[i+2];
      tt[i+3] = ptr[i+3];
    }
  }

  radeon_bo_unmap(buffer);
  
  time2 = gettime();
  
  for (int j = 0; j < num; j++)
  {
    for (size_t i = 0; i < size/4; i+=4)
    {
      tt[i] = j;
      tt[i+1] = j;
      tt[i+2] = j;
      tt[i+3] = j;
    }
  }
  
  timeval time3 = gettime();
  
  dsec = (((long long)time2.tv_sec) - ((long long)time1.tv_sec));
  dusec = dsec*1000000 + ((long long)time2.tv_usec) - ((long long)time1.tv_usec);

  long long dsec2 = (((long long)time3.tv_sec) - ((long long)time2.tv_sec));
  long long dusec2 = dsec2*1000000 + ((long long)time3.tv_usec) - ((long long)time2.tv_usec);

  dusec -= dusec2;
  
  cout << "Time: " << dusec << " us" << endl;
  cout << "speed: " << double(size*num) / double(dusec) / 1024.0 << " GBps" << endl;
  
  radeon_bo_unref(buffer);
  
  cerr << "OK" << endl;
}
