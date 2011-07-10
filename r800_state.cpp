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
#include <assert.h>
#include <sys/ioctl.h>
#include <string.h>

#include "r800_state.h"
#include "radeon_cs.h"
#include "evergreen_reg.h"
#include "dummy_ps_ec.h"

#define SPI_COMPUTE_INPUT_CNTL 0x000286E8
#define SQ_LDS_RESOURCE_MGMT 0x8e2c
#define NUM_LS_LDS_shift 16

#define SQ_LDS_ALLOC 0x288e8
#define SQ_LDS_ALLOC_SIZE_SHIFT 0
#define SQ_LDS_ALLOC_HS_NUM_WAVES_SHIFT 14

#define GDS_ORDERED_WAVE_PER_SE 0x28728
// #define GDS_ORDERED_WAVE_PER_SE_COUNT_SHIFT 0

#define GDS_ADDR_BASE 0x28720
#define GDS_ADDR_SIZE 0x28724

//for VGT_GS_MODE
#define COMPUTE_MODE_bit (1 << 14)
#define PARTIAL_THD_AT_EOI_bit (1 << 17)

#define SX_MEMORY_EXPORT_BASE 0x9010
#define SX_MEMORY_EXPORT_SIZE 0x9014


#define GRBM_GFX_INDEX                                  0x802C
#define         INSTANCE_INDEX(x)                       ((x) << 0)
#define         SE_INDEX(x)                             ((x) << 16)
#define         INSTANCE_BROADCAST_WRITES               (1 << 30)
#define         SE_BROADCAST_WRITES                     (1 << 31)


#define VGT_COMPUTE_START_X 0x899c
// 31:0 START

#define VGT_COMPUTE_START_Y 0x89a0
// 31:0 START

#define VGT_COMPUTE_START_Z 0x89a4
// 31:0 START

#define SPI_COMPUTE_NUM_THREAD_X 0x286ec
// 15:0 VALUE

#define SPI_COMPUTE_NUM_THREAD_Y 0x286f0
// 15:0 VALUE

#define SPI_COMPUTE_NUM_THREAD_Z 0x286f4
// 15:0 VALUE

#define VGT_COMPUTE_THREAD_GROUP_SIZE 0x89ac
// 11:0 SIZE

#define VGT_DISPATCH_INITIATOR 0x28b74
// 0 COMPUTE_SHADER_EN
// 
#define PACKET3_DISPATCH_DIRECT                         0x15
#define PACKET3_DISPATCH_INDIRECT                       0x16

using namespace std;

r800_state::r800_state(int fd, bool exclusive) : fd(fd), cs(fd), exclusive(exclusive)
{
  drmSetVersion sv;

  sv.drm_di_major = 1;
  sv.drm_di_minor = 1;
  sv.drm_dd_major = -1;
  sv.drm_dd_minor = -1;
  
  if (exclusive)
  {
    int ret = drmSetInterfaceVersion(fd, &sv);
    
    if (ret != 0)
    {
      fprintf(stderr, "Cannot set interface version\n");
      close(fd);
      exit(1);
    }
    
    get_master();
  }
  
  if (drmCommandWriteRead(fd, DRM_RADEON_GEM_INFO, &mminfo, sizeof(mminfo)))
  {
    cerr << "Cannot get GEM info" << endl;
    exit(1);
  }
   
  
  bom = radeon_bo_manager_gem_ctor(fd);
  assert(bom);

  
  ChipFamily = CHIP_FAMILY_PALM; //TODO: fix it
  
  printf("mem size init: gart size :%llx vram size: s:%llx visible:%llx\n",
    (unsigned long long)mminfo.gart_size,
    (unsigned long long)mminfo.vram_size,
    (unsigned long long)mminfo.vram_visible);
    
  printf("mem size init: gart size :%iMbytes vram size: s:%iMbytes visible:%iMbytes\n",
    (unsigned long long)mminfo.gart_size/1024/1024,
    (unsigned long long)mminfo.vram_size/1024/1024,
    (unsigned long long)mminfo.vram_visible/1024/1024);

  cs.set_limit(RADEON_GEM_DOMAIN_GTT, mminfo.gart_size);
  cs.set_limit(RADEON_GEM_DOMAIN_VRAM, mminfo.vram_size);

/*  dummy_bo = bo_open(1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  dummy_bo_ps = bo_open(sizeof(dummy_ps_shader_binary), 0, RADEON_GEM_DOMAIN_VRAM, 0);
  dummy_bo_cb = bo_open(8*1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  dummy_vbo = bo_open(1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  
  cout << "dummy_bo:" << dummy_bo_ps << " " << dummy_bo_ps->handle << endl;*/
  
  set_default_state();
}

r800_state::~r800_state()
{
  radeon_bo_manager_gem_dtor(bom);
  if (exclusive)
  {
    drop_master();
  }
}

struct radeon_bo* r800_state::bo_open(uint32_t size,
  uint32_t alignment,
  uint32_t domains,
  uint32_t flags)
{
  return radeon_bo_open(bom, 0, size, alignment, domains, flags);
}
  

struct radeon_bo *r800_state::bo_open(uint32_t handle, uint32_t size, uint32_t alignment, uint32_t domains, uint32_t flags)
{
    return radeon_bo_open(bom, handle, size, alignment, domains, flags);
}

int r800_state::bo_is_referenced_by_cs(struct radeon_bo *bo)
{
  throw 0;
//   return radeon_bo_is_referenced_by_cs(bo, cs);
}


void r800_state::get_master()
{
  int ret = ioctl(fd, DRM_IOCTL_SET_MASTER, 0);
  if (ret < 0)
  {
    cerr << "Cannot get master" << endl;
  }
  
  drop_master();
  
  ret = ioctl(fd, DRM_IOCTL_SET_MASTER, 0);
  if (ret < 0)
  {
    cerr << "Cannot get master" << endl;
  }  
}

void r800_state::drop_master()
{
  int ret = ioctl(fd, DRM_IOCTL_DROP_MASTER, 0);
  if (ret < 0)
  {
    cerr << "Cannot drop master" << endl;
    throw 0; //shouldn't happen!
  }
}

void r800_state::start_3d()
{
    cs.packet3(IT_CONTEXT_CONTROL, {0x80000000, 0x80000000});
}

void r800_state::sq_setup()
{
  uint32_t sq_config, sq_gpr_resource_mgmt_1, sq_gpr_resource_mgmt_2, sq_gpr_resource_mgmt_3;
  uint32_t sq_thread_resource_mgmt, sq_thread_resource_mgmt_2;
  uint32_t sq_stack_resource_mgmt_1, sq_stack_resource_mgmt_2, sq_stack_resource_mgmt_3;

  if ((ChipFamily == CHIP_FAMILY_CEDAR) ||
      (ChipFamily == CHIP_FAMILY_PALM) ||
      (ChipFamily == CHIP_FAMILY_CAICOS))
      sq_config = 0;
  else
      sq_config = VC_ENABLE_bit;

  sq_config |= (EXPORT_SRC_C_bit |
		(sq_conf.cs_prio << CS_PRIO_shift) |
		(sq_conf.ls_prio << LS_PRIO_shift) |
		(sq_conf.hs_prio << HS_PRIO_shift) |
		(sq_conf.ps_prio << PS_PRIO_shift) |
		(sq_conf.vs_prio << VS_PRIO_shift) |
		(sq_conf.gs_prio << GS_PRIO_shift) |
		(sq_conf.es_prio << ES_PRIO_shift));

  sq_gpr_resource_mgmt_1 = ((sq_conf.num_ps_gprs << NUM_PS_GPRS_shift) |
			    (sq_conf.num_vs_gprs << NUM_VS_GPRS_shift) |
			    (sq_conf.num_temp_gprs << NUM_CLAUSE_TEMP_GPRS_shift));
  sq_gpr_resource_mgmt_2 = ((sq_conf.num_gs_gprs << NUM_GS_GPRS_shift) |
			    (sq_conf.num_es_gprs << NUM_ES_GPRS_shift));
  sq_gpr_resource_mgmt_3 = ((sq_conf.num_hs_gprs << NUM_HS_GPRS_shift) |
			    (sq_conf.num_ls_gprs << NUM_LS_GPRS_shift));

  sq_thread_resource_mgmt = ((sq_conf.num_ps_threads << NUM_PS_THREADS_shift) |
			      (sq_conf.num_vs_threads << NUM_VS_THREADS_shift) |
			      (sq_conf.num_gs_threads << NUM_GS_THREADS_shift) |
			      (sq_conf.num_es_threads << NUM_ES_THREADS_shift));
  sq_thread_resource_mgmt_2 = ((sq_conf.num_hs_threads << NUM_HS_THREADS_shift) |
				(sq_conf.num_ls_threads << NUM_LS_THREADS_shift));

  sq_stack_resource_mgmt_1 = ((sq_conf.num_ps_stack_entries << NUM_PS_STACK_ENTRIES_shift) |
			      (sq_conf.num_vs_stack_entries << NUM_VS_STACK_ENTRIES_shift));

  sq_stack_resource_mgmt_2 = ((sq_conf.num_gs_stack_entries << NUM_GS_STACK_ENTRIES_shift) |
			      (sq_conf.num_es_stack_entries << NUM_ES_STACK_ENTRIES_shift));

  sq_stack_resource_mgmt_3 = ((sq_conf.num_hs_stack_entries << NUM_HS_STACK_ENTRIES_shift) |
			      (sq_conf.num_ls_stack_entries << NUM_LS_STACK_ENTRIES_shift));

    
  cs[SQ_DYN_GPR_CNTL_PS_FLUSH_REQ] = 0;
    
  cs[SQ_CONFIG] = {
    sq_config,
    sq_gpr_resource_mgmt_1,
    sq_gpr_resource_mgmt_2,
    sq_gpr_resource_mgmt_3
  };
    
  cs[SQ_THREAD_RESOURCE_MGMT] = {
    sq_thread_resource_mgmt,
    sq_thread_resource_mgmt_2,
    sq_stack_resource_mgmt_1,
    sq_stack_resource_mgmt_2,
    sq_stack_resource_mgmt_3      
  };
    
  cs[SQ_ESGS_RING_ITEMSIZE] = 0;
  cs[SQ_GSVS_RING_ITEMSIZE] = 0;
  cs[SQ_ESTMP_RING_ITEMSIZE] = 0;
  cs[SQ_GSTMP_RING_ITEMSIZE] = 0;
  cs[SQ_VSTMP_RING_ITEMSIZE] = 0;
  cs[SQ_PSTMP_RING_ITEMSIZE] = 0;
  cs[SQ_GS_VERT_ITEMSIZE] = {0, 0, 0, 0};
  cs[SQ_VTX_BASE_VTX_LOC] = {0, 0};

  cs[SQ_VTX_SEMANTIC] = vector<uint32_t>(SQ_VTX_SEMANTIC_num);
  
  
  cs[SQ_PGM_RESOURCES_PS] = {0, 0};
  
  cs[CB_BLEND0_CONTROL] = {0, 0, 0, 0, 0, 0, 0, 0};
  
/*  cs[SQ_PGM_START_FS] = 0;
  cs.reloc(dummy_bo, RADEON_GEM_DOMAIN_VRAM, 0);*/
  
  cs[SQ_PGM_RESOURCES_FS] = 0; //we won't use fetch shaders
  
  cs[SQ_LDS_ALLOC_PS] = 0;
  cs[SQ_DYN_GPR_RESOURCE_LIMIT_1] = 0;

  
}

void r800_state::set_default_sq()
{
    /* SQ */
    sq_conf.ps_prio = 0;
    sq_conf.vs_prio = 1;
    sq_conf.gs_prio = 2;
    sq_conf.es_prio = 3;
    sq_conf.hs_prio = 0;
    sq_conf.ls_prio = 0;
    sq_conf.cs_prio = 0;

    switch (ChipFamily) {
    case CHIP_FAMILY_CEDAR:
    default:
	sq_conf.num_ps_gprs = 93;
	sq_conf.num_vs_gprs = 46;
	sq_conf.num_temp_gprs = 4;
	sq_conf.num_gs_gprs = 31;
	sq_conf.num_es_gprs = 31;
	sq_conf.num_hs_gprs = 23;
	sq_conf.num_ls_gprs = 23;
	sq_conf.num_ps_threads = 96;
	sq_conf.num_vs_threads = 16;
	sq_conf.num_gs_threads = 16;
	sq_conf.num_es_threads = 16;
	sq_conf.num_hs_threads = 16;
	sq_conf.num_ls_threads = 16;
	sq_conf.num_ps_stack_entries = 42;
	sq_conf.num_vs_stack_entries = 42;
	sq_conf.num_gs_stack_entries = 42;
	sq_conf.num_es_stack_entries = 42;
	sq_conf.num_hs_stack_entries = 42;
	sq_conf.num_ls_stack_entries = 42;
	break;
    case CHIP_FAMILY_REDWOOD:
	sq_conf.num_ps_gprs = 93;
	sq_conf.num_vs_gprs = 46;
	sq_conf.num_temp_gprs = 4;
	sq_conf.num_gs_gprs = 31;
	sq_conf.num_es_gprs = 31;
	sq_conf.num_hs_gprs = 23;
	sq_conf.num_ls_gprs = 23;
	sq_conf.num_ps_threads = 128;
	sq_conf.num_vs_threads = 20;
	sq_conf.num_gs_threads = 20;
	sq_conf.num_es_threads = 20;
	sq_conf.num_hs_threads = 20;
	sq_conf.num_ls_threads = 20;
	sq_conf.num_ps_stack_entries = 42;
	sq_conf.num_vs_stack_entries = 42;
	sq_conf.num_gs_stack_entries = 42;
	sq_conf.num_es_stack_entries = 42;
	sq_conf.num_hs_stack_entries = 42;
	sq_conf.num_ls_stack_entries = 42;
	break;
    case CHIP_FAMILY_JUNIPER:
	sq_conf.num_ps_gprs = 93;
	sq_conf.num_vs_gprs = 46;
	sq_conf.num_temp_gprs = 4;
	sq_conf.num_gs_gprs = 31;
	sq_conf.num_es_gprs = 31;
	sq_conf.num_hs_gprs = 23;
	sq_conf.num_ls_gprs = 23;
	sq_conf.num_ps_threads = 128;
	sq_conf.num_vs_threads = 20;
	sq_conf.num_gs_threads = 20;
	sq_conf.num_es_threads = 20;
	sq_conf.num_hs_threads = 20;
	sq_conf.num_ls_threads = 20;
	sq_conf.num_ps_stack_entries = 85;
	sq_conf.num_vs_stack_entries = 85;
	sq_conf.num_gs_stack_entries = 85;
	sq_conf.num_es_stack_entries = 85;
	sq_conf.num_hs_stack_entries = 85;
	sq_conf.num_ls_stack_entries = 85;
	break;
    case CHIP_FAMILY_CYPRESS:
    case CHIP_FAMILY_HEMLOCK:
	sq_conf.num_ps_gprs = 93;
	sq_conf.num_vs_gprs = 46;
	sq_conf.num_temp_gprs = 4;
	sq_conf.num_gs_gprs = 31;
	sq_conf.num_es_gprs = 31;
	sq_conf.num_hs_gprs = 23;
	sq_conf.num_ls_gprs = 23;
	sq_conf.num_ps_threads = 128;
	sq_conf.num_vs_threads = 20;
	sq_conf.num_gs_threads = 20;
	sq_conf.num_es_threads = 20;
	sq_conf.num_hs_threads = 20;
	sq_conf.num_ls_threads = 20;
	sq_conf.num_ps_stack_entries = 85;
	sq_conf.num_vs_stack_entries = 85;
	sq_conf.num_gs_stack_entries = 85;
	sq_conf.num_es_stack_entries = 85;
	sq_conf.num_hs_stack_entries = 85;
	sq_conf.num_ls_stack_entries = 85;
	break;
    case CHIP_FAMILY_PALM:
	sq_conf.num_ps_gprs = 93;
	sq_conf.num_vs_gprs = 46;
	sq_conf.num_temp_gprs = 4;
	sq_conf.num_gs_gprs = 31;
	sq_conf.num_es_gprs = 31;
	sq_conf.num_hs_gprs = 23;
	sq_conf.num_ls_gprs = 23;
	sq_conf.num_ps_threads = 96;
	sq_conf.num_vs_threads = 16;
	sq_conf.num_gs_threads = 16;
	sq_conf.num_es_threads = 16;
	sq_conf.num_hs_threads = 16;
	sq_conf.num_ls_threads = 16;
	sq_conf.num_ps_stack_entries = 42;
	sq_conf.num_vs_stack_entries = 42;
	sq_conf.num_gs_stack_entries = 42;
	sq_conf.num_es_stack_entries = 42;
	sq_conf.num_hs_stack_entries = 42;
	sq_conf.num_ls_stack_entries = 42;
	break;
    case CHIP_FAMILY_BARTS:
	sq_conf.num_ps_gprs = 93;
	sq_conf.num_vs_gprs = 46;
	sq_conf.num_temp_gprs = 4;
	sq_conf.num_gs_gprs = 31;
	sq_conf.num_es_gprs = 31;
	sq_conf.num_hs_gprs = 23;
	sq_conf.num_ls_gprs = 23;
	sq_conf.num_ps_threads = 128;
	sq_conf.num_vs_threads = 20;
	sq_conf.num_gs_threads = 20;
	sq_conf.num_es_threads = 20;
	sq_conf.num_hs_threads = 20;
	sq_conf.num_ls_threads = 20;
	sq_conf.num_ps_stack_entries = 85;
	sq_conf.num_vs_stack_entries = 85;
	sq_conf.num_gs_stack_entries = 85;
	sq_conf.num_es_stack_entries = 85;
	sq_conf.num_hs_stack_entries = 85;
	sq_conf.num_ls_stack_entries = 85;
	break;
    case CHIP_FAMILY_TURKS:
	sq_conf.num_ps_gprs = 93;
	sq_conf.num_vs_gprs = 46;
	sq_conf.num_temp_gprs = 4;
	sq_conf.num_gs_gprs = 31;
	sq_conf.num_es_gprs = 31;
	sq_conf.num_hs_gprs = 23;
	sq_conf.num_ls_gprs = 23;
	sq_conf.num_ps_threads = 128;
	sq_conf.num_vs_threads = 20;
	sq_conf.num_gs_threads = 20;
	sq_conf.num_es_threads = 20;
	sq_conf.num_hs_threads = 20;
	sq_conf.num_ls_threads = 20;
	sq_conf.num_ps_stack_entries = 42;
	sq_conf.num_vs_stack_entries = 42;
	sq_conf.num_gs_stack_entries = 42;
	sq_conf.num_es_stack_entries = 42;
	sq_conf.num_hs_stack_entries = 42;
	sq_conf.num_ls_stack_entries = 42;
	break;
    case CHIP_FAMILY_CAICOS:
	sq_conf.num_ps_gprs = 93;
	sq_conf.num_vs_gprs = 46;
	sq_conf.num_temp_gprs = 4;
	sq_conf.num_gs_gprs = 31;
	sq_conf.num_es_gprs = 31;
	sq_conf.num_hs_gprs = 23;
	sq_conf.num_ls_gprs = 23;
	sq_conf.num_ps_threads = 128;
	sq_conf.num_vs_threads = 10;
	sq_conf.num_gs_threads = 10;
	sq_conf.num_es_threads = 10;
	sq_conf.num_hs_threads = 10;
	sq_conf.num_ls_threads = 10;
	sq_conf.num_ps_stack_entries = 42;
	sq_conf.num_vs_stack_entries = 42;
	sq_conf.num_gs_stack_entries = 42;
	sq_conf.num_es_stack_entries = 42;
	sq_conf.num_hs_stack_entries = 42;
	sq_conf.num_ls_stack_entries = 42;
	break;
    }
}

void r800_state::set_default_state()
{
  start_3d();
  set_default_sq();
  
  set_sx_defaults();
  set_export(NULL, 0, 0);
  set_db_defaults();
  sq_setup();

  set_vgt_defaults();
  set_pa_defaults();
  set_spi_defaults();
  
  for (int i = 0; i < 12; i++)
  {
    set_rat_defaults(i);
  }
}

void r800_state::set_spi_defaults()
{
  cs[SPI_CONFIG_CNTL] = 0;
  cs[SPI_CONFIG_CNTL_1] = X_DELAY_22_CLKS;

  cs[SPI_PS_INPUT_CNTL_0] = {0, 0};
  cs[SPI_INPUT_Z] = 0;
  cs[SPI_FOG_CNTL] = 0;
  cs[SPI_BARYC_CNTL] = LINEAR_CENTROID_ENA__X_ON_AT_CENTROID << LINEAR_CENTROID_ENA_shift;
  cs[SPI_PS_IN_CONTROL_2] = vector<uint32_t>{0, 0, 0};

  cs[SPI_VS_OUT_CONFIG] = 0;
  cs[SPI_VS_OUT_ID_0] = 0;
  cs[SPI_INTERP_CONTROL_0] = 0;
  cs[SPI_PS_IN_CONTROL_0] = {LINEAR_GRADIENT_ENA_bit, 0};
  cs[SPI_COMPUTE_INPUT_CNTL] = 0;
}

void r800_state::set_pa_defaults()
{
  cs[PA_CL_ENHANCE] = 3 << 1 | 1;
  cs[PA_CL_CLIP_CNTL] = 0;
  
  cs[PA_SC_VPORT_ZMIN_0] = 0.0f;
  cs[PA_SC_VPORT_ZMAX_0] = 1.0f;
  cs[PA_SC_WINDOW_OFFSET] = 0;
  cs[PA_SC_CLIPRECT_RULE] = CLIP_RULE_mask;
  cs[PA_SC_EDGERULE] = 0xAAAAAAAA;
  cs[PA_SU_HARDWARE_SCREEN_OFFSET] = 0;
  
  for (int i = 0; i < PA_SC_CLIPRECT_0_TL_num; i++)
  {
    cs[PA_SC_CLIPRECT_0_TL + i * PA_SC_CLIPRECT_0_TL_offset] = {
    ((0 << PA_SC_CLIPRECT_0_TL__TL_X_shift) |
	(0 << PA_SC_CLIPRECT_0_TL__TL_Y_shift)), 
    ((8192 << PA_SC_CLIPRECT_0_BR__BR_X_shift) |
	(8192 << PA_SC_CLIPRECT_0_BR__BR_Y_shift))
    };
  }
  
  for (int i = 0; i < PA_SC_VPORT_SCISSOR_0_TL_num; i++)
  {
    cs[PA_SC_VPORT_SCISSOR_0_TL + i * PA_SC_VPORT_SCISSOR_0_TL_offset] = {
    ((0 << PA_SC_VPORT_SCISSOR_0_TL__TL_X_shift) |
	(0 << PA_SC_VPORT_SCISSOR_0_TL__TL_Y_shift) |
	WINDOW_OFFSET_DISABLE_bit),
    ((8192 << PA_SC_VPORT_SCISSOR_0_BR__BR_X_shift) |
	(8192 << PA_SC_VPORT_SCISSOR_0_BR__BR_Y_shift))
    };
  }
  
  cs[PA_SC_MODE_CNTL] = {0, 0};
  cs[PA_SC_LINE_CNTL] = vector<uint32_t>{0, 0, (X_ROUND_TO_EVEN << PA_SU_VTX_CNTL__ROUND_MODE_shift) |
      PIX_CENTER_bit};
  cs[PA_CL_GB_VERT_CLIP_ADJ] = 1.0f;
  cs[PA_CL_GB_VERT_DISC_ADJ] = 1.0f;
  cs[PA_CL_GB_HORZ_CLIP_ADJ] = 1.0f;
  cs[PA_CL_GB_HORZ_DISC_ADJ] = 1.0f;
  cs[PA_SC_AA_SAMPLE_LOCS_0] = {0, 0, 0, 0, 0, 0, 0, 0};
  cs[PA_SC_AA_MASK] = 0xFFFFFFFF;
  cs[PA_CL_CLIP_CNTL] = CLIP_DISABLE_bit;
  cs[PA_CL_VTE_CNTL] = VTX_XY_FMT_bit;
  cs[PA_CL_VS_OUT_CNTL] = 0;
  cs[PA_CL_NANINF_CNTL] = 0;
  cs[PA_SU_LINE_STIPPLE_CNTL] = 0;
  cs[PA_SU_LINE_STIPPLE_SCALE] = 0;
  cs[PA_SU_PRIM_FILTER_CNTL] = 0;
  cs[PA_SU_POLY_OFFSET_DB_FMT_CNTL] = {0, 0, 0, 0, 0, 0};
  cs[PA_SC_LINE_STIPPLE] = 0;
  cs[PA_SC_MODE_CNTL] = {0, 0};
  
  set_dummy_scissors(); //that's PA too
  
  cs[PA_SU_LINE_CNTL] = 0; //(8 << PA_SU_LINE_CNTL__WIDTH_shift);
  cs[PA_SU_SC_MODE_CNTL] = CULL_FRONT_bit | CULL_BACK_bit | FACE_bit  | ( 2 << POLYMODE_FRONT_PTYPE_shift) | ( 2 << POLYMODE_BACK_PTYPE_shift);
  cs[PA_SU_POINT_SIZE] = 0;
  cs[PA_SU_POINT_MINMAX] = 0;
}

void r800_state::set_vgt_defaults()
{
  cs[VGT_OUTPUT_PATH_CNTL] = 0;
  cs[VGT_HOS_CNTL] = 0;
  cs[VGT_HOS_MAX_TESS_LEVEL] = 0;
  cs[VGT_HOS_MIN_TESS_LEVEL] = 0;
  cs[VGT_HOS_REUSE_DEPTH] = 0;
  cs[VGT_GROUP_PRIM_TYPE] = 0;
  cs[VGT_GROUP_FIRST_DECR] = 0;
  cs[VGT_GROUP_DECR] = 0;
  cs[VGT_GROUP_VECT_0_CNTL] = 0;
  cs[VGT_GROUP_VECT_1_CNTL] = 0;
  cs[VGT_GROUP_VECT_0_FMT_CNTL] = 0;
  cs[VGT_GROUP_VECT_1_FMT_CNTL] = 0;
  cs[VGT_STRMOUT_CONFIG] = 0;
  cs[VGT_STRMOUT_BUFFER_CONFIG] = 0;
  cs[VGT_REUSE_OFF] = 0;
  cs[VGT_VTX_CNT_EN] = 0;

  cs[VGT_MAX_VTX_INDX] = 0xFFFFFFFF;
  cs[VGT_MIN_VTX_INDX] = 0;
  cs[VGT_INDX_OFFSET] = 0;
  cs[VGT_MULTI_PRIM_IB_RESET_INDX] = 0;
  
  cs[VGT_INSTANCE_STEP_RATE_0] = {0, 0};
  cs[VGT_REUSE_OFF] = 0;
  cs[VGT_VTX_CNT_EN] = 0;
 
  
  cs[VGT_OUTPUT_PATH_CNTL] = 0;
  cs[VGT_HOS_CNTL] = 0;
  cs[VGT_HOS_MAX_TESS_LEVEL] = 0;
  cs[VGT_HOS_MIN_TESS_LEVEL] = 0;
  cs[VGT_HOS_REUSE_DEPTH] = 0;
  cs[VGT_GROUP_PRIM_TYPE] = 0;
  cs[VGT_GROUP_FIRST_DECR] = 0;
  cs[VGT_GROUP_DECR] = 0;
  cs[VGT_GROUP_VECT_0_CNTL] = {0, 0};
  cs[VGT_GROUP_VECT_0_FMT_CNTL] = {0, 0};
  cs[VGT_PRIMITIVEID_EN] = 0;
  cs[VGT_MULTI_PRIM_IB_RESET_EN] = 0;
  
  cs[VGT_GS_MODE] = GS_OFF | COMPUTE_MODE_bit | PARTIAL_THD_AT_EOI_bit;
  cs[VGT_SHADER_STAGES_EN] = CS_STAGE_ON << LS_EN_shift; // HS VS GS ES is in default 0, which is OFF, or for VS  its VS_STAGE_REAL
  
  cs[VGT_STRMOUT_CONFIG] = {0, 0};

}

void r800_state::set_db_defaults()
{
//   cs[DB_Z_INFO] = 0;
//   cs.reloc(dummy_bo, RADEON_GEM_DOMAIN_VRAM, 0);
//   
//   cs[DB_STENCIL_INFO] = 0;
//   cs.reloc(dummy_bo, RADEON_GEM_DOMAIN_VRAM, 0);
//   
//   cs[DB_HTILE_DATA_BASE] = 0;
//   cs.reloc(dummy_bo, RADEON_GEM_DOMAIN_VRAM, 0);
  
  cs[DB_DEPTH_CONTROL] = 0;

  cs[DB_SHADER_CONTROL] = 0; 
  cs[DB_RENDER_CONTROL] = STENCIL_COMPRESS_DISABLE_bit | DEPTH_COMPRESS_DISABLE_bit | COLOR_DISABLE_bit;
  cs[DB_COUNT_CONTROL] = 0;
  cs[DB_DEPTH_VIEW] = 0;
  cs[DB_RENDER_OVERRIDE] = {0, 0};
  cs[DB_PRELOAD_CONTROL] = 0;
  cs[DB_SRESULTS_COMPARE_STATE] = {0, 0};
  
  cs[DB_STENCIL_CLEAR] = 0;
  cs[DB_DEPTH_CLEAR] = 0;
  cs[DB_ALPHA_TO_MASK] = 0;

}

void r800_state::set_sx_defaults()
{
  cs[SX_MISC] = 0;  
  cs[SX_ALPHA_TEST_CONTROL] = {0, 0, 0, 0, 0};
}


void r800_state::set_lds(int num_lds, int size, int num_waves)
{
  cs[SQ_LDS_RESOURCE_MGMT] = num_lds << NUM_LS_LDS_shift | 0x1000 /*pixel shader setup workaround for Xorg bug*/;
  cs[SQ_LDS_ALLOC] = (size << SQ_LDS_ALLOC_SIZE_SHIFT) | (num_waves << SQ_LDS_ALLOC_HS_NUM_WAVES_SHIFT);
}

void r800_state::set_gds(uint32_t addr, uint32_t size)
{
  cs[GDS_ORDERED_WAVE_PER_SE] = 1;
  cs[GDS_ADDR_BASE] = addr;
  cs[GDS_ADDR_SIZE] = size;
}

void r800_state::set_export(radeon_bo* bo, int offset, int size)
{
  if (size)
  {
    cs[SX_MEMORY_EXPORT_BASE] = offset;
    cs.reloc(bo, 0, RADEON_GEM_DOMAIN_VRAM);
  }
  
  cs[SX_MEMORY_EXPORT_SIZE] = size;
  
/** for cayman
  SX_SCATTER_EXPORT_BASE
  SX_SCATTER_EXPORT_SIZE
*/

  if (bo)
  {
    cs.add_persistent_bo(bo, 0, RADEON_GEM_DOMAIN_VRAM);
  }
}

void r800_state::set_tmp_ring(radeon_bo* bo, int offset, int size)
{
  if (size)
  {
    cs[SQ_LSTMP_RING_BASE] = offset;
    cs.reloc(bo, RADEON_GEM_DOMAIN_VRAM, RADEON_GEM_DOMAIN_VRAM);
  }
  
  cs[SQ_LSTMP_RING_SIZE] = size;
}

void r800_state::select_se(int se_index, bool broadcast_writes)
{
  cs[GRBM_GFX_INDEX] = INSTANCE_INDEX(0) | SE_INDEX(se_index) | INSTANCE_BROADCAST_WRITES | (broadcast_writes ? SE_BROADCAST_WRITES : 0);
}

void r800_state::direct_dispatch(int groupnum, int local_size)
{
  cs[VGT_PRIMITIVE_TYPE] = DI_PT_POINTLIST;
  
  cs[VGT_COMPUTE_START_X] = 0;
  cs[VGT_COMPUTE_START_Y] = 0;
  cs[VGT_COMPUTE_START_Z] = 0;
  
  cs[SPI_COMPUTE_NUM_THREAD_X] = local_size;
  cs[SPI_COMPUTE_NUM_THREAD_Y] = 1;
  cs[SPI_COMPUTE_NUM_THREAD_Z] = 1;
 
  cs[VGT_NUM_INDICES] = local_size;
  
  cs[VGT_COMPUTE_THREAD_GROUP_SIZE] = local_size;
  
  cs.packet3(PACKET3_DISPATCH_DIRECT,
	     { groupnum, 1, 1, 1}
  );
}

void r800_state::set_draw_auto(int num_indices)
{   
  cs[VGT_PRIMITIVE_TYPE] = DI_PT_RECTLIST;
  cs.packet3(IT_INDEX_TYPE, {DI_INDEX_SIZE_32_BIT});
  cs.packet3(IT_NUM_INSTANCES, {1});
  cs.packet3(IT_DRAW_INDEX_AUTO, {num_indices, DI_SRC_SEL_AUTO_INDEX});
}

void r800_state::set_surface_sync(uint32_t sync_type, uint32_t size, uint64_t mc_addr, struct radeon_bo *bo, uint32_t rdomains, uint32_t wdomain)
{
    uint32_t cp_coher_size;
    if (size == 0xffffffff)
	cp_coher_size = 0xffffffff;
    else
	cp_coher_size = ((size + 255) >> 8);

    uint32_t poll_interval = 10;
    
    cs.packet3(IT_SURFACE_SYNC, {sync_type, cp_coher_size, uint32_t(mc_addr >> 8), poll_interval});
    cs.reloc(bo, rdomains, wdomain);
}

void r800_state::set_vtx_resource(vtx_resource_t *res, uint32_t domain)
{
    uint32_t sq_vtx_constant_word2, sq_vtx_constant_word3, sq_vtx_constant_word4;

    sq_vtx_constant_word2 = ((((res->vb_addr) >> 32) & BASE_ADDRESS_HI_mask) |
			     ((res->vtx_size_dw << 2) << SQ_VTX_CONSTANT_WORD2_0__STRIDE_shift) |
			     (res->format << SQ_VTX_CONSTANT_WORD2_0__DATA_FORMAT_shift) |
			     (res->num_format_all << SQ_VTX_CONSTANT_WORD2_0__NUM_FORMAT_ALL_shift) |
			     (res->endian << SQ_VTX_CONSTANT_WORD2_0__ENDIAN_SWAP_shift));
    if (res->clamp_x)
	    sq_vtx_constant_word2 |= SQ_VTX_CONSTANT_WORD2_0__CLAMP_X_bit;

    if (res->format_comp_all)
	    sq_vtx_constant_word2 |= SQ_VTX_CONSTANT_WORD2_0__FORMAT_COMP_ALL_bit;

    if (res->srf_mode_all)
	    sq_vtx_constant_word2 |= SQ_VTX_CONSTANT_WORD2_0__SRF_MODE_ALL_bit;

    sq_vtx_constant_word3 = ((res->dst_sel_x << SQ_VTX_CONSTANT_WORD3_0__DST_SEL_X_shift) |
			     (res->dst_sel_y << SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Y_shift) |
			     (res->dst_sel_z << SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Z_shift) |
			     (res->dst_sel_w << SQ_VTX_CONSTANT_WORD3_0__DST_SEL_W_shift));

    if (res->uncached)
	sq_vtx_constant_word3 |= SQ_VTX_CONSTANT_WORD3_0__UNCACHED_bit;

    /* XXX ??? */
    sq_vtx_constant_word4 = 0;

    set_surface_sync(TC_ACTION_ENA_bit,
		    res->offset, 0,
		    res->bo,
		    domain, 0);

//     BEGIN_BATCH(10 + 2);
//     PACK0(SQ_FETCH_RESOURCE + res->id * SQ_FETCH_RESOURCE_offset, 8);
//     E32(res->vb_addr & 0xffffffff);				// 0: BASE_ADDRESS
//     E32((res->vtx_num_entries << 2) - 1);			// 1: SIZE
//     E32(sq_vtx_constant_word2);	// 2: BASE_HI, STRIDE, CLAMP, FORMAT, ENDIAN
//     E32(sq_vtx_constant_word3);		// 3: swizzles
//     E32(sq_vtx_constant_word4);		// 4: num elements
//     E32(0);							// 5: n/a
//     E32(0);							// 6: n/a
//     E32(SQ_TEX_VTX_VALID_BUFFER << SQ_VTX_CONSTANT_WORD7_0__TYPE_shift);	// 7: TYPE
//     RELOC_BATCH(res->bo, domain, 0);
//     END_BATCH();

  cs[SQ_FETCH_RESOURCE + res->id * SQ_FETCH_RESOURCE_offset] = {
      uint32_t(res->vb_addr & 0xffffffff),
      (res->vtx_num_entries << 2) - 1,
      sq_vtx_constant_word2,
      sq_vtx_constant_word3,
      sq_vtx_constant_word4,
      0,
      0,
      SQ_TEX_VTX_VALID_BUFFER << SQ_VTX_CONSTANT_WORD7_0__TYPE_shift
  };
  
  cs.reloc(res->bo, domain, 0);
}

void r800_state::set_dummy_render_target()
{
  cs[CB_COLOR0_BASE] = 0;
  cs.reloc(dummy_bo_cb, 0, RADEON_GEM_DOMAIN_VRAM);

  /* Set CMASK & FMASK buffer to the offset of color buffer as
  * we don't use those this shouldn't cause any issue and we
  * then have a valid cmd stream
  */
  cs[CB_COLOR0_CMASK] = 0;
  cs.reloc(dummy_bo_cb, 0, RADEON_GEM_DOMAIN_VRAM);

  cs[CB_COLOR0_FMASK] = 0;
  cs.reloc(dummy_bo_cb, 0, RADEON_GEM_DOMAIN_VRAM);

  cs[CB_COLOR0_ATTRIB] = CB_COLOR0_ATTRIB__NON_DISP_TILING_ORDER_bit;
  cs.reloc(dummy_bo_cb, 0, RADEON_GEM_DOMAIN_VRAM);

  cs[CB_COLOR0_INFO] = 
    (COLOR_8 << CB_COLOR0_INFO__FORMAT_shift) |
    (3 << COMP_SWAP_shift) |
    (EXPORT_4C_16BPC << SOURCE_FORMAT_shift) |
    (BLEND_CLAMP_bit);
    
  cs.reloc(dummy_bo_cb, 0, RADEON_GEM_DOMAIN_VRAM);

  cs[CB_COLOR0_PITCH] = (16 / 8) - 1;
  cs[CB_COLOR0_SLICE] = ((16*16) / 64) - 1;
  cs[CB_COLOR0_VIEW] = 0;
  cs[CB_COLOR0_DIM] =  (15 << WIDTH_MAX_shift) | ( 15 << HEIGHT_MAX_shift);
  cs[CB_COLOR0_CMASK_SLICE] = 0;
  cs[CB_COLOR0_FMASK_SLICE] = 0;
  cs[CB_COLOR0_CLEAR_WORD0] = {0, 0, 0, 0};
  cs[CB_TARGET_MASK] = 15 << TARGET0_ENABLE_shift | TARGET2_ENABLE_mask;
  cs[CB_SHADER_MASK] = OUTPUT0_ENABLE_mask | OUTPUT2_ENABLE_mask;
  cs[CB_COLOR_CONTROL] = 0x00cc0000 | (CB_NORMAL << CB_COLOR_CONTROL__MODE_shift);
  cs[CB_BLEND0_CONTROL] = 0;
}

void r800_state::set_rat(int id, radeon_bo* bo, int start, int size)
{
  int offset;
  
  int rd = 0;
  int wd = RADEON_GEM_DOMAIN_VRAM;
  
  assert(id < 12);
  
  cs.add_persistent_bo(bo, rd, wd);
    
  if (id < 8)
  {
    offset = id*0x3c;
  }
  else
  {
    offset = 8*0x3c + (id-8)*0x1c;
  }
  
  assert((size & 3) == 0);
  assert((start & 0xFF) == 0);
  
  cs[CB_COLOR0_BASE + offset] = start >> 8;
  cs.reloc(bo, rd, wd);

  cs[CB_IMMED0_BASE + id*8] = start >> 8;
  cs.reloc(bo, rd, wd);
  
  cs[CB_COLOR0_INFO + offset] = RAT_bit;
  cs.reloc(bo, rd, wd);
  
  cs[CB_COLOR0_ATTRIB + offset] = CB_COLOR0_ATTRIB__NON_DISP_TILING_ORDER_bit;
  cs.reloc(bo, rd, wd);
  
  cs[CB_COLOR0_DIM + offset] = size >> 2; //for RATs its a linear buffer size!
      
  if (id < 8)
  {
    cs[CB_COLOR0_CMASK + offset] = 0;
    cs.reloc(bo, rd, wd);

    cs[CB_COLOR0_FMASK + offset] = 0;
    cs.reloc(bo, rd, wd);
  }
  
  set_surface_sync(CB_ACTION_ENA_bit /*| CB2_DEST_BASE_ENA_bit*/ | FULL_CACHE_ENA_bit, 1024*1024, 0, bo,/* RADEON_GEM_DOMAIN_VRAM*/0, RADEON_GEM_DOMAIN_VRAM); //FIXME CBn_DEST_BASE_ENA_bit
}

void r800_state::set_rat_defaults(int id)
{
  int offset;
  
  assert(id < 12);
  
  if (id < 8)
  {
    offset = id*0x3c;
  }
  else
  {
    offset = 8*0x3c + (id-8)*0x1c;
  }
    
  cs[CB_COLOR0_PITCH + offset] = 0;
  cs[CB_COLOR0_SLICE + offset] = 0;
  cs[CB_COLOR0_VIEW + offset] = 0;
  
  cs[CB_COLOR0_DIM + offset] = 0;
      
  if (id < 8)
  {
    cs[CB_COLOR0_CMASK_SLICE + offset] = 0;
    cs[CB_COLOR0_FMASK_SLICE + offset] = 0;
    cs[CB_COLOR0_CLEAR_WORD0 + offset] = {0, 0, 0, 0};
  }
}

void r800_state::flush_cs()
{
  //should unmap and unref bo-s first?
  cs.cs_emit();
  cs.cs_erase();
}

void r800_state::upload_dummy_ps()
{
  assert(radeon_bo_map(dummy_bo_ps, 1) == 0);
  memcpy(dummy_bo_ps->ptr, dummy_ps_shader_binary, sizeof(dummy_ps_shader_binary));
  radeon_bo_unmap(dummy_bo_ps);
}

void r800_state::set_dummy_scissors()
{
  cs[PA_SC_GENERIC_SCISSOR_TL] = {
    ((0 << PA_SC_GENERIC_SCISSOR_TL__TL_X_shift) |
      (0 << PA_SC_GENERIC_SCISSOR_TL__TL_Y_shift) |
      WINDOW_OFFSET_DISABLE_bit),
    ((16 << PA_SC_GENERIC_SCISSOR_BR__BR_X_shift) |
      (16 << PA_SC_GENERIC_SCISSOR_TL__TL_Y_shift))
  };

  cs[PA_SC_SCREEN_SCISSOR_TL] = {
    ((0 << PA_SC_SCREEN_SCISSOR_TL__TL_X_shift) |
      (0 << PA_SC_SCREEN_SCISSOR_TL__TL_Y_shift)),
    ((16 << PA_SC_SCREEN_SCISSOR_BR__BR_X_shift) |
      (16 << PA_SC_SCREEN_SCISSOR_BR__BR_Y_shift))
  }; 
  cs[PA_SC_WINDOW_SCISSOR_TL] = {
    ((0 << PA_SC_WINDOW_SCISSOR_TL__TL_X_shift) |
      (0 << PA_SC_WINDOW_SCISSOR_TL__TL_Y_shift) |
      WINDOW_OFFSET_DISABLE_bit),
    ((16 << PA_SC_WINDOW_SCISSOR_BR__BR_X_shift) |
      (16 << PA_SC_WINDOW_SCISSOR_BR__BR_Y_shift))
  };
}

void r800_state::setup_const_cache(int cache_id, struct radeon_bo* cbo, int size, int offset)
{
  assert(cache_id < SQ_ALU_CONST_BUFFER_SIZE_VS_0_num);

  set_surface_sync(SH_ACTION_ENA_bit,
		size, offset,
		cbo, RADEON_GEM_DOMAIN_VRAM, 0);
		
  cs[SQ_ALU_CONST_BUFFER_SIZE_LS_0+cache_id] = size;

  assert(size < SQ_ALU_CONST_BUFFER_SIZE_LS_0__DATA_mask);
  
  assert((offset & 0xFF) == 0);
  cs[SQ_ALU_CONST_CACHE_LS_0+cache_id] = offset >> 8;
  cs.reloc(cbo, RADEON_GEM_DOMAIN_VRAM, 0);

  assert(cache_id < SQ_ALU_CONST_BUFFER_SIZE_LS_0_num);
  
  cs.add_persistent_bo(cbo, RADEON_GEM_DOMAIN_VRAM, 0);
}

void r800_state::prepare_compute_shader(compute_shader* sh)
{
  set_surface_sync(SH_ACTION_ENA_bit,
		  sh->alloc_size, 0,
		  sh->binary_code_bo, RADEON_GEM_DOMAIN_VRAM, 0);
		  
//   set_surface_sync(SH_ACTION_ENA_bit,
// 		  sizeof(dummy_ps_shader_binary), 0,
// 		  dummy_bo_ps, RADEON_GEM_DOMAIN_VRAM, 0);
// 
//     cs[SQ_PGM_START_PS] = 0;
//     cs.reloc(dummy_bo_ps, RADEON_GEM_DOMAIN_VRAM, 0);
//     cs[SQ_PGM_RESOURCES_PS] = {
//       (/*num_gprs*/1 | (0 << STACK_SIZE_shift)),
//       SQ_ROUND_NEAREST_EVEN | ALLOW_DOUBLE_DENORM_IN_bit | ALLOW_DOUBLE_DENORM_OUT_bit
//     };
//     cs[SQ_PGM_START_VS] = 0;
//     cs.reloc(sh->binary_code_bo, RADEON_GEM_DOMAIN_VRAM, 0);
//     cs[SQ_PGM_RESOURCES_VS] = {
//       (sh->num_gprs | (sh->stack_size << STACK_SIZE_shift)) | PRIME_CACHE_ENABLE,
//       SQ_ROUND_NEAREST_EVEN | ALLOW_DOUBLE_DENORM_IN_bit | ALLOW_DOUBLE_DENORM_OUT_bit
//     };
//     
//     cs[SQ_LDS_ALLOC_PS] = sh->lds_alloc; //in 32 bit words
//     cs[SQ_GLOBAL_GPR_RESOURCE_MGMT_1] = (0 << PS_GGPR_BASE_shift) | (sh->global_gprs << VS_GGPR_BASE_shift);
//     cs[SQ_GLOBAL_GPR_RESOURCE_MGMT_2] = 0;
//     cs[SQ_THREAD_RESOURCE_MGMT] = (sq_conf.num_ps_threads << NUM_PS_THREADS_shift) | (sh->thread_num << NUM_VS_THREADS_shift);
//     cs[SQ_STACK_RESOURCE_MGMT_1] = (1 << NUM_PS_STACK_ENTRIES_shift) | (sh->stack_size << NUM_VS_STACK_ENTRIES_shift);
//     cs[SQ_PGM_EXPORTS_PS] = 2; 

  cs[SQ_PGM_START_LS] = 0;
  cs.reloc(sh->binary_code_bo, RADEON_GEM_DOMAIN_VRAM, 0);
  
  cs[SQ_PGM_RESOURCES_LS] = {
    (sh->num_gprs << NUM_GPRS_shift) | (sh->stack_size << STACK_SIZE_shift) | PRIME_CACHE_ENABLE,
    SQ_ROUND_NEAREST_EVEN | ALLOW_DOUBLE_DENORM_IN_bit | ALLOW_DOUBLE_DENORM_OUT_bit
  };
  
  uint32_t tt = (sh->num_gprs << NUM_GPRS_shift) | (sh->stack_size << STACK_SIZE_shift) | PRIME_CACHE_ENABLE;
  
  
//   printf("%.8X\n", tt);
  
  cs[SQ_GLOBAL_GPR_RESOURCE_MGMT_1] = 0;
  cs[SQ_GLOBAL_GPR_RESOURCE_MGMT_2] = (sh->global_gprs << LS_GGPR_BASE_shift) | (sh->global_gprs << CS_GGPR_BASE_shift);
  
  cs[SQ_STACK_RESOURCE_MGMT_3] = sh->stack_size << NUM_LS_STACK_ENTRIES_shift;
  cs[SQ_THREAD_RESOURCE_MGMT_2] = sh->thread_num << NUM_LS_THREADS_shift;
  
  set_tmp_ring(NULL, 0, 0);
  
  set_lds(0, 0, 0);
}

void r800_state::execute_shader(compute_shader* sh)
{
  cs.add_persistent_bo(sh->binary_code_bo, RADEON_GEM_DOMAIN_VRAM, 0);

  cs.space_check();
  
  prepare_compute_shader(sh);
  
//   for (int i = 0; i < SQ_LOOP_CONST_cs_num*4; i++)
  {
    cs[SQ_LOOP_CONST + SQ_LOOP_CONST_cs*4 /*+ i*/] = (1 << SQ_LOOP_CONST_0__COUNT_shift) | (0 << INIT_shift) | (1 << INC_shift);
  }
  
  direct_dispatch(1, 128);
  
//   setup_const_cache(0, dummy_bo, 256, 0);

//   vtx_resource_t vtxr;
//   
//   memset(&vtxr, 0, sizeof(vtxr));
//   
//   vtxr.id = SQ_FETCH_RESOURCE_vs;
//   vtxr.vtx_size_dw = 4;
//   vtxr.vtx_num_entries = 1;
//   vtxr.vb_addr = 0;
//   vtxr.bo = dummy_vbo;
//   vtxr.dst_sel_x       = SQ_SEL_X;
//   vtxr.dst_sel_y       = SQ_SEL_Y;
//   vtxr.dst_sel_z       = SQ_SEL_Z;
//   vtxr.dst_sel_w       = SQ_SEL_W;
//   set_vtx_resource(&vtxr, RADEON_GEM_DOMAIN_VRAM);

//   set_draw_auto(3);
}

