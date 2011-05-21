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
      compute_shader sh(&state, "/home/rakadam/Projects/drm_prb/dummy_vs.bin");
      radeon_bo* buffer = state.bo_open(0, 1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
      radeon_bo_map(buffer, 1);
      uint32_t *ptr = (uint32_t*)buffer->ptr;
      
      for (int i = 0; i < 256; i++)
      {
	ptr[i] = 0;
      }
      
      radeon_bo_unmap(buffer);
      state.set_rat(11, buffer);
      state.execute_shader(&sh);
//       state.set_surface_sync(CB_ACTION_ENA_bit | CB11_DEST_BASE_ENA_bit, 16*16*4, 0, buffer, 0, RADEON_GEM_DOMAIN_VRAM);
      state.flush_cs();
      
      radeon_bo_map(buffer, 0);
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

