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
//     int size = 500*1024*1024;
    assert(drmAvailable());

     int fd = open("/dev/dri/card0", O_RDWR, 0);
     
//      int fd = drmOpen(NULL, "pci:0000:00:01.0");
     
//     struct drm_radeon_gem_info mminfo;

//     if (!drmCommandWriteRead(fd, DRM_RADEON_GEM_INFO, &mminfo, sizeof(mminfo)))
//     {
// 	printf("mem size init: gart size :%llx vram size: s:%llx visible:%llx\n",
// 		    (unsigned long long)mminfo.gart_size,
// 		    (unsigned long long)mminfo.vram_size,
// 		    (unsigned long long)mminfo.vram_visible);
//     }
//      
//      if (0)
     {
      r800_state state(fd);
      compute_shader sh(&state, "first_cs.bin");
      radeon_bo* buffer = state.bo_open(0, 1024*1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
      radeon_bo_map(buffer, 1);
      uint32_t *ptr = (uint32_t*)buffer->ptr;
      
      for (int i = 0; i < 256; i++)
      {
	ptr[i] = 0xF;
      } 
      radeon_bo_unmap(buffer);
      
      radeon_bo* buffer2 = state.bo_open(0, 1024*1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
      radeon_bo_map(buffer2, 1);
      ptr = (uint32_t*)buffer2->ptr;
      
      for (int i = 0; i < 256; i++)
      {
	ptr[i] = 0x2;
      } 
      radeon_bo_unmap(buffer2);
      
      state.set_rat(2, buffer, 0, 1024);
      state.set_rat(3, buffer2, 0, 1024);
      
      state.execute_shader(&sh);
      state.flush_cs();
      printf("emitted\n");
      
      uint32_t w = RADEON_GEM_DOMAIN_VRAM;
      
      while (radeon_bo_is_busy(buffer, &w))
      {
	sleep(1);
	printf(".\n");
      }
      
      radeon_bo_map(buffer, 0);
      printf("mapped\n");
      
      ptr = (uint32_t*)buffer->ptr;
      
      for (int i = 0; i < 256; i++)
      {
	printf("%X ", ptr[i]);
      }
      
      radeon_bo_unmap(buffer);
      
      cout << endl;
//       sleep(1);
     }
//    int fd = drmOpen("/dev/dri/card0", "pci:0000:01:00.0");
//     cout << fd << endl;
//     assert(fd > 0);
//     cout << drmGetBusid(fd) << endl;
//     struct radeon_cs_manager * gem = radeon_cs_manager_gem_ctor(fd);
//     struct radeon_bo_manager * bom = radeon_bo_manager_gem_ctor(fd);
//     struct radeon_cs *cs = radeon_cs_create(gem, 0);
//     cout << "cs:" << cs << endl;
//     cout << gem << " " << bom << endl;
//     struct radeon_bo * bo = radeon_bo_open(bom, 0, size, 0, RADEON_GEM_DOMAIN_VRAM, 0);
//     cout << bo << endl;
// //     cout << radeon_cs_space_check(cs) << endl;
//     radeon_cs_print(cs, stdout);
//     
//     cout << "map:" << radeon_bo_map(bo, 0) << endl;
//     
//     buf = (char*)bo->ptr;
//     cout << (void*)buf << endl;
//     
//     cout << "fill" << endl;
//     memset(buf, 0, size);
//     cout << "unmap" << endl;
//     radeon_bo_unmap(bo);
//     radeon_bo_wait(bo);
//     cout << "done" << endl;
//     
//     
//     radeon_cs_set_limit(cs, RADEON_GEM_DOMAIN_VRAM, 510*1024*1024);
//     
//     radeon_cs_space_add_persistent_bo(cs, bo, RADEON_GEM_DOMAIN_VRAM, RADEON_GEM_DOMAIN_VRAM);
//     cout << "space_check: " << radeon_cs_space_check(cs) << endl;
//     
//     radeon_cs_write_reloc(cs, bo, RADEON_GEM_DOMAIN_VRAM, RADEON_GEM_DOMAIN_VRAM, 0);
//     
//     cout << "done" << endl;
//     radeon_bo_unref(bo);
//     radeon_cs_destroy(cs);
//     radeon_bo_manager_gem_dtor(bom);
//     radeon_cs_manager_gem_dtor(gem);


    drmClose(fd);
}

