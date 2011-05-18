#include <iostream>
#include <assert.h>
#include <sys/ioctl.h>
#include <string.h>

#include "r800_state.h"
#include "radeon_reg.h"
#include "evergreen_reg.h"
#include "dummy_ps_ec.h"

using namespace std;

compute_shader::compute_shader(r800_state* state, const std::vector<char>& binary)
{
  const uint32_t* header = (const uint32_t*)&binary[0];
  
  assert(binary.size()%4 == 0);
  assert(header[0] == 0x42424242);

  uint32_t sum = 0;
  
  for (int i = 2; i < binary.size()/4; i++)
  {
    sum = sum + header[i];
  }
  
  assert(header[1] == sum);
  
  lds_alloc = header[3];
  num_gprs = header[4];
  temp_gprs = header[5];
  global_gprs = header[6];
  stack_size = header[7];
  thread_num = header[8];
  dyn_gpr_limit = header[9];

  /* 10-15 reserved*/
  
  int shader_start = 16*4;
  
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


#define CP_PACKET0(reg, n)                                              \
        (RADEON_CP_PACKET0 | ((n) << 16) | ((reg) >> 2))
#define CP_PACKET1(reg0, reg1)                                          \
        (RADEON_CP_PACKET1 | (((reg1) >> 2) << 11) | ((reg0) >> 2))
#define CP_PACKET2()                                                    \
        (RADEON_CP_PACKET2)
#define CP_PACKET3(pkt, n)                                              \
        (RADEON_CP_PACKET3 | (pkt) | ((n) << 16))

void r800_state::radeon_cs_flush_indirect(r800_state* state)
{
  radeon_cs_emit(state->cs);
  radeon_cs_erase(state->cs);
}

r800_state::r800_state(int fd) : fd(fd)
{
  get_master();
  
  gem = radeon_cs_manager_gem_ctor(fd);
  assert(gem);
  bom = radeon_bo_manager_gem_ctor(fd);
  assert(bom);
  cs = radeon_cs_create(gem, RADEON_BUFFER_SIZE/4);
  
  if (!cs)
  {
    cerr << "Failed to open cs" << endl;
    exit(1);
  }
  
  ChipFamily = CHIP_FAMILY_PALM; //TODO: fix it
  

  if (drmCommandWriteRead(fd, DRM_RADEON_GEM_INFO, &mminfo, sizeof(mminfo)))
  {
    cerr << "Cannot get GEM info" << endl;
    exit(1);
  }
  
  printf("mem size init: gart size :%llx vram size: s:%llx visible:%llx\n",
    (unsigned long long)mminfo.gart_size,
    (unsigned long long)mminfo.vram_size,
    (unsigned long long)mminfo.vram_visible);
    
  printf("mem size init: gart size :%iMbytes vram size: s:%iMbytes visible:%iMbytes\n",
    (unsigned long long)mminfo.gart_size/1024/1024,
    (unsigned long long)mminfo.vram_size/1024/1024,
    (unsigned long long)mminfo.vram_visible/1024/1024);

  radeon_cs_set_limit(cs, RADEON_GEM_DOMAIN_GTT, mminfo.gart_size);
  radeon_cs_set_limit(cs, RADEON_GEM_DOMAIN_VRAM, mminfo.vram_size);
  radeon_cs_space_set_flush(cs, (void (*)(void*))radeon_cs_flush_indirect, this);

  dummy_bo = bo_open(1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  dummy_bo_ps = bo_open(sizeof(dummy_ps_shader_binary), 1024, RADEON_GEM_DOMAIN_VRAM, 0);
  dummy_bo_cb = bo_open(8*1024, 1024, RADEON_GEM_DOMAIN_VRAM, 0);
}

r800_state::~r800_state()
{
  radeon_cs_destroy(cs);
  radeon_bo_manager_gem_dtor(bom);
  radeon_cs_manager_gem_dtor(gem);
  drop_master();
}

struct radeon_bo* r800_state::bo_open(uint32_t size,
  uint32_t alignment,
  uint32_t domains,
  uint32_t flags)
{
  return radeon_bo_open(bom, 0, size, alignment, domains, flags);
}
  

void r800_state::cs_begin(int ndw)
{
  radeon_cs_begin(cs, ndw, __FILE__, __func__, __LINE__);
}

void r800_state::cs_end()
{
  radeon_cs_end(cs, __FILE__, __func__, __LINE__);
}

void r800_state::reloc(struct radeon_bo *bo, uint32_t rd, uint32_t wd)
{
  radeon_cs_write_reloc(cs, bo, rd, wd, 0);
}

void r800_state::write_dword(uint32_t dword)
{
  radeon_cs_write_dword(cs, dword);
}

void r800_state::write_float(float f)
{
  union uni_ {
    float f;
    uint32_t u32;
  } uni;
  
  uni.f = f;
  write_dword(uni.u32);
}

void r800_state::packet3(uint32_t cmd, uint32_t num)
{
  assert(num);
  write_dword(RADEON_CP_PACKET3 | ((cmd) << 8) | ((((num) - 1) & 0x3fff) << 16));
}

void r800_state::packet0(uint32_t reg, uint32_t num)
{
     if ((reg) >= SET_CONFIG_REG_offset && (reg) < SET_CONFIG_REG_end) {	
	packet3(IT_SET_CONFIG_REG, (num) + 1);			
	write_dword(((reg) - SET_CONFIG_REG_offset) >> 2);                  
    } else if ((reg) >= SET_CONTEXT_REG_offset && (reg) < SET_CONTEXT_REG_end) { 
	packet3(IT_SET_CONTEXT_REG, (num) + 1);			
	write_dword(((reg) - SET_CONTEXT_REG_offset) >> 2);			
    } else if ((reg) >= SET_RESOURCE_offset && (reg) < SET_RESOURCE_end) { 
	packet3(IT_SET_RESOURCE, num + 1);				
	write_dword(((reg) - SET_RESOURCE_offset) >> 2);			
    } else if ((reg) >= SET_SAMPLER_offset && (reg) < SET_SAMPLER_end) { 
	packet3(IT_SET_SAMPLER, (num) + 1);				
	write_dword((reg - SET_SAMPLER_offset) >> 2);			
    } else if ((reg) >= SET_CTL_CONST_offset && (reg) < SET_CTL_CONST_end) { 
	packet3(IT_SET_CTL_CONST, (num) + 1);			
	write_dword(((reg) - SET_CTL_CONST_offset) >> 2);		
    } else if ((reg) >= SET_LOOP_CONST_offset && (reg) < SET_LOOP_CONST_end) { 
	packet3(IT_SET_LOOP_CONST, (num) + 1);			
	write_dword(((reg) - SET_LOOP_CONST_offset) >> 2);
    } else if ((reg) >= SET_BOOL_CONST_offset && (reg) < SET_BOOL_CONST_end) { 
	packet3(IT_SET_BOOL_CONST, (num) + 1);
	write_dword(((reg) - SET_BOOL_CONST_offset) >> 2);
    } else {
	write_dword(CP_PACKET0 ((reg), (num) - 1));
    }
}

void r800_state::set_reg(uint32_t reg, uint32_t val)
{
    packet0(reg, 1);
    write_dword(val);
}

void r800_state::set_reg(uint32_t reg, float val)
{
    packet0(reg, 1);
    write_dword(val);
}


void r800_state::add_persistent_bo(struct radeon_bo *bo,
  uint32_t read_domains,
  uint32_t write_domain)
{
  radeon_cs_space_add_persistent_bo(cs, bo, read_domains, write_domain);
}

struct radeon_bo *r800_state::bo_open(uint32_t handle, uint32_t size, uint32_t alignment, uint32_t domains, uint32_t flags)
{
    return radeon_bo_open(bom, handle, size, alignment, domains, flags);
}

int r800_state::bo_is_referenced_by_cs(struct radeon_bo *bo)
{
  return radeon_bo_is_referenced_by_cs(bo, cs);
}

void r800_state::set_regs(uint32_t reg, std::vector<uint32_t> vals)
{
  packet0(reg, vals.size());
  
  for (int i = 0; i < int(vals.size()); i++)
  {
    write_dword(vals[i]);
  }
}

void r800_state::send_packet3(uint32_t cmd, std::vector<uint32_t> vals)
{
  assert(vals.size());
  packet3(cmd, vals.size());
  
  for (int i = 0; i < int(vals.size()); i++)
  {
    write_dword(vals[i]);
  }
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
    cs_begin(3);
    send_packet3(IT_CONTEXT_CONTROL, {0x80000000, 0x80000000});
    cs_end();
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
    
    {
      asic_cmd reg(this);
      
      reg[SQ_DYN_GPR_CNTL_PS_FLUSH_REQ] = 0; // disable dyn gprs
      
      reg[SQ_CONFIG] = {
	sq_config,
	sq_gpr_resource_mgmt_1,
	sq_gpr_resource_mgmt_2,
	sq_gpr_resource_mgmt_3
      };
      
      reg[SQ_THREAD_RESOURCE_MGMT] = {
	sq_thread_resource_mgmt,
	sq_thread_resource_mgmt_2,
	sq_stack_resource_mgmt_1,
	sq_stack_resource_mgmt_2,
	sq_stack_resource_mgmt_3      
      };
      
      reg[SPI_CONFIG_CNTL] = 0;
      reg[SPI_CONFIG_CNTL_1] = X_DELAY_22_CLKS;
      reg[PA_SC_MODE_CNTL] = {0, 0};
      reg[SQ_ESGS_RING_ITEMSIZE] = 0;
      reg[SQ_GSVS_RING_ITEMSIZE] = 0;
      reg[SQ_ESTMP_RING_ITEMSIZE] = 0;
      reg[SQ_GSTMP_RING_ITEMSIZE] = 0;
      reg[SQ_VSTMP_RING_ITEMSIZE] = 0;
      reg[SQ_PSTMP_RING_ITEMSIZE] = 0;
      reg[SQ_GS_VERT_ITEMSIZE] = {0, 0, 0, 0};
      reg[SQ_VTX_BASE_VTX_LOC] = {0, 0};
    }


    {
      asic_cmd reg(this);
      reg[DB_Z_INFO] = 0;
      reg.reloc(dummy_bo, RADEON_GEM_DOMAIN_VRAM, 0);
    }
    
    {
      asic_cmd reg(this);
      reg[DB_STENCIL_INFO] = 0;
      reg.reloc(dummy_bo, RADEON_GEM_DOMAIN_VRAM, 0);
    }
    
    {
      asic_cmd reg(this);
      reg[DB_HTILE_DATA_BASE] = 0;
      reg.reloc(dummy_bo, RADEON_GEM_DOMAIN_VRAM, 0);
    }
    
    {
      asic_cmd reg(this);
      
      reg[DB_DEPTH_CONTROL] = 0;
      
      reg[VGT_OUTPUT_PATH_CNTL] = 0;
      reg[VGT_HOS_CNTL] = 0;
      reg[VGT_HOS_MAX_TESS_LEVEL] = 0;
      reg[VGT_HOS_MIN_TESS_LEVEL] = 0;
      reg[VGT_HOS_REUSE_DEPTH] = 0;
      reg[VGT_GROUP_PRIM_TYPE] = 0;
      reg[VGT_GROUP_FIRST_DECR] = 0;
      reg[VGT_GROUP_DECR] = 0;
      reg[VGT_GROUP_VECT_0_CNTL] = 0;
      reg[VGT_GROUP_VECT_1_CNTL] = 0;
      reg[VGT_GROUP_VECT_0_FMT_CNTL] = 0;
      reg[VGT_GROUP_VECT_1_FMT_CNTL] = 0;
      reg[VGT_GS_MODE] = 0;
      reg[VGT_STRMOUT_CONFIG] = 0;
      reg[VGT_STRMOUT_BUFFER_CONFIG] = 0;
      reg[VGT_REUSE_OFF] = 0;
      reg[VGT_VTX_CNT_EN] = 0;
      reg[PA_CL_ENHANCE] = 3 << 1 | 1;
      reg[SQ_VTX_SEMANTIC] = vector<uint32_t>(SQ_VTX_SEMANTIC_num);
      reg[PA_CL_CLIP_CNTL] = 0;
      reg[PA_SC_VPORT_ZMIN_0] = 0.0f;
      reg[PA_SC_VPORT_ZMAX_0] = 1.0f;
      reg[DB_SHADER_CONTROL] = 0;
      reg[DB_RENDER_CONTROL] = STENCIL_COMPRESS_DISABLE_bit | DEPTH_COMPRESS_DISABLE_bit;
      reg[DB_COUNT_CONTROL] = 0;
      reg[DB_DEPTH_VIEW] = 0;
      reg[DB_RENDER_OVERRIDE] = {0, 0};
      reg[DB_PRELOAD_CONTROL] = 0;
      reg[DB_SRESULTS_COMPARE_STATE] = {0, 0};
      
      reg[DB_STENCIL_CLEAR] = 0;
      reg[DB_DEPTH_CLEAR] = 0;
      reg[DB_ALPHA_TO_MASK] = 0;
      
      reg[SX_MISC] = 0;
      
      reg[SX_ALPHA_TEST_CONTROL] = {0, 0, 0, 0, 0};
      reg[CB_SHADER_MASK] = 0;
      reg[PA_SC_WINDOW_OFFSET] = 0;
      reg[PA_SC_CLIPRECT_RULE] = CLIP_RULE_mask;
      reg[PA_SC_EDGERULE] = 0xAAAAAAAA;
      reg[PA_SU_HARDWARE_SCREEN_OFFSET] = 0;
      
      reg[SQ_PGM_RESOURCES_PS] = {0, 0};
      
      for (int i = 0; i < PA_SC_CLIPRECT_0_TL_num; i++)
      {
	reg[PA_SC_CLIPRECT_0_TL + i * PA_SC_CLIPRECT_0_TL_offset] = {
	((0 << PA_SC_CLIPRECT_0_TL__TL_X_shift) |
	    (0 << PA_SC_CLIPRECT_0_TL__TL_Y_shift)), 
	((8192 << PA_SC_CLIPRECT_0_BR__BR_X_shift) |
	    (8192 << PA_SC_CLIPRECT_0_BR__BR_Y_shift))
	};
      }
      
      for (int i = 0; i < PA_SC_VPORT_SCISSOR_0_TL_num; i++)
      {
	reg[PA_SC_VPORT_SCISSOR_0_TL + i * PA_SC_VPORT_SCISSOR_0_TL_offset] = {
	((0 << PA_SC_VPORT_SCISSOR_0_TL__TL_X_shift) |
	    (0 << PA_SC_VPORT_SCISSOR_0_TL__TL_Y_shift) |
	    WINDOW_OFFSET_DISABLE_bit),
	((8192 << PA_SC_VPORT_SCISSOR_0_BR__BR_X_shift) |
	    (8192 << PA_SC_VPORT_SCISSOR_0_BR__BR_Y_shift))
	};
      }
      
      reg[PA_SC_MODE_CNTL] = {0, 0};
      reg[PA_SC_LINE_CNTL] = vector<uint32_t>{0, 0, (X_ROUND_TO_EVEN << PA_SU_VTX_CNTL__ROUND_MODE_shift) |
	  PIX_CENTER_bit};
      reg[PA_CL_GB_VERT_CLIP_ADJ] = 1.0f;
      reg[PA_CL_GB_VERT_DISC_ADJ] = 1.0f;
      reg[PA_CL_GB_HORZ_CLIP_ADJ] = 1.0f;
      reg[PA_CL_GB_HORZ_DISC_ADJ] = 1.0f;
      
      reg[PA_SC_AA_SAMPLE_LOCS_0] = {0, 0, 0, 0, 0, 0, 0, 0};
      reg[PA_SC_AA_MASK] = 0xFFFFFFFF;
      reg[PA_CL_CLIP_CNTL] = CLIP_DISABLE_bit;
      reg[PA_SU_SC_MODE_CNTL] = FACE_bit;
      reg[PA_CL_VTE_CNTL] = VTX_XY_FMT_bit;
      reg[PA_CL_VS_OUT_CNTL] = 0;
      reg[PA_CL_NANINF_CNTL] = 0;
      reg[PA_SU_LINE_STIPPLE_CNTL] = 0;
      reg[PA_SU_LINE_STIPPLE_SCALE] = 0;
      reg[PA_SU_PRIM_FILTER_CNTL] = 0;
  
      reg[PA_SU_POLY_OFFSET_DB_FMT_CNTL] = {0, 0, 0, 0, 0, 0};
      reg[CB_BLEND0_CONTROL] = {0, 0, 0, 0, 0, 0, 0, 0};
    }
    
    {
      asic_cmd reg(this);
      
      reg[SQ_PGM_START_FS] = 0;
      reg.reloc(dummy_bo, RADEON_GEM_DOMAIN_VRAM, 0);
    }
    
    {
      asic_cmd reg(this);
      reg[SQ_PGM_RESOURCES_FS] = 0; //we won't use fetch shaders
      
      reg[VGT_MAX_VTX_INDX] = 0xFFFFFFFF;
      reg[VGT_MIN_VTX_INDX] = 0;
      reg[VGT_INDX_OFFSET] = 0;
      reg[VGT_MULTI_PRIM_IB_RESET_INDX] = 0;
      
      reg[VGT_INSTANCE_STEP_RATE_0] = {0, 0};
      reg[VGT_REUSE_OFF] = 0;
      reg[VGT_VTX_CNT_EN] = 0;
      
      reg[PA_SU_POINT_SIZE] = 0;
      reg[PA_SU_POINT_MINMAX] = 0;
      reg[PA_SU_LINE_CNTL] = (8 << PA_SU_LINE_CNTL__WIDTH_shift);
      reg[PA_SC_LINE_STIPPLE] = 0;
      reg[VGT_OUTPUT_PATH_CNTL] = 0;
      reg[VGT_HOS_CNTL] = 0;
      reg[VGT_HOS_MAX_TESS_LEVEL] = 0;
      reg[VGT_HOS_MIN_TESS_LEVEL] = 0;
      reg[VGT_HOS_REUSE_DEPTH] = 0;
      reg[VGT_GROUP_PRIM_TYPE] = 0;
      reg[VGT_GROUP_FIRST_DECR] = 0;
      reg[VGT_GROUP_DECR] = 0;
      reg[VGT_GROUP_VECT_0_CNTL] = {0, 0};
      reg[VGT_GROUP_VECT_0_FMT_CNTL] = {0, 0};
      reg[VGT_GS_MODE] = 0;
      reg[VGT_PRIMITIVEID_EN] = 0;
      reg[VGT_MULTI_PRIM_IB_RESET_EN] = 0;
      reg[VGT_SHADER_STAGES_EN] = 0;
      reg[VGT_STRMOUT_CONFIG] = {0, 0};
    }
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


asic_cmd::asic_cmd(r800_state* state) : state(state), reloc_num(0)
{
}

asic_cmd::asic_cmd(asic_cmd&& b) : queue(b.queue), state(b.state), reloc_num(b.reloc_num)
{
  b.state = NULL;
  b.queue.clear();
  b.reloc_num = 0;
}

void asic_cmd::reloc(struct radeon_bo * bo, uint32_t rd, uint32_t wd)
{
  queue.push_back(token(bo, rd, wd));
  reloc_num++;
}

void asic_cmd::write_dword(uint32_t dword)
{
  queue.push_back(token(dword));
}

void asic_cmd::write_float(float f)
{
  union uni_ {
    float f;
    uint32_t u32;
  } uni;
  
  uni.f = f;
  write_dword(uni.u32);
}

void asic_cmd::packet3(uint32_t cmd, uint32_t num)
{
  assert(num);
  write_dword(RADEON_CP_PACKET3 | ((cmd) << 8) | ((((num) - 1) & 0x3fff) << 16));
}

void asic_cmd::send_packet3(uint32_t cmd, std::vector<uint32_t> vals)
{
  assert(vals.size());
  packet3(cmd, vals.size());
  
  for (int i = 0; i < int(vals.size()); i++)
  {
    write_dword(vals[i]);
  }
}

void asic_cmd::packet0(uint32_t reg, uint32_t num)
{
    if ((reg) >= SET_CONFIG_REG_offset && (reg) < SET_CONFIG_REG_end) {	
	packet3(IT_SET_CONFIG_REG, (num) + 1);			
	write_dword(((reg) - SET_CONFIG_REG_offset) >> 2);                  
    } else if ((reg) >= SET_CONTEXT_REG_offset && (reg) < SET_CONTEXT_REG_end) { 
	packet3(IT_SET_CONTEXT_REG, (num) + 1);			
	write_dword(((reg) - SET_CONTEXT_REG_offset) >> 2);			
    } else if ((reg) >= SET_RESOURCE_offset && (reg) < SET_RESOURCE_end) { 
	packet3(IT_SET_RESOURCE, num + 1);				
	write_dword(((reg) - SET_RESOURCE_offset) >> 2);			
    } else if ((reg) >= SET_SAMPLER_offset && (reg) < SET_SAMPLER_end) { 
	packet3(IT_SET_SAMPLER, (num) + 1);				
	write_dword((reg - SET_SAMPLER_offset) >> 2);			
    } else if ((reg) >= SET_CTL_CONST_offset && (reg) < SET_CTL_CONST_end) { 
	packet3(IT_SET_CTL_CONST, (num) + 1);			
	write_dword(((reg) - SET_CTL_CONST_offset) >> 2);		
    } else if ((reg) >= SET_LOOP_CONST_offset && (reg) < SET_LOOP_CONST_end) { 
	packet3(IT_SET_LOOP_CONST, (num) + 1);			
	write_dword(((reg) - SET_LOOP_CONST_offset) >> 2);
    } else if ((reg) >= SET_BOOL_CONST_offset && (reg) < SET_BOOL_CONST_end) { 
	packet3(IT_SET_BOOL_CONST, (num) + 1);
	write_dword(((reg) - SET_BOOL_CONST_offset) >> 2);
    } else {
	write_dword(CP_PACKET0 ((reg), (num) - 1));
    }
}

void asic_cmd::set_reg(uint32_t reg, int val)
{
    packet0(reg, 1);
    write_dword((uint32_t)val);
}

void asic_cmd::set_reg(uint32_t reg, uint32_t val)
{
    packet0(reg, 1);
    write_dword(val);
}

void asic_cmd::set_reg(uint32_t reg, float val)
{
    packet0(reg, 1);
    write_dword(val);
}

void asic_cmd::set_regs(uint32_t reg, std::vector<uint32_t> vals)
{
  packet0(reg, vals.size());
  
  for (int i = 0; i < int(vals.size()); i++)
  {
    write_dword(vals[i]);
  }
}


asic_cmd::~asic_cmd()
{
  if (!state)
  {
    return;
  }
  
  int ndw = queue.size() + reloc_num*2;
  
  radeon_cs_begin(state->cs, ndw, __FILE__, __func__, __LINE__);

  for (int i = 0; i < int(queue.size()); i++)
  {
    if (queue[i].bo_reloc)
    {
      radeon_cs_write_reloc(state->cs, queue[i].bo, queue[i].rd, queue[i].wd, 0);
    }
    else
    {
      radeon_cs_write_dword(state->cs, queue[i].dw); 
    }
  }
  
  radeon_cs_end(state->cs, __FILE__, __func__, __LINE__);
}

asic_cmd::asic_cmd_regsetter asic_cmd::operator[](uint32_t index)
{
  return asic_cmd::asic_cmd_regsetter(this, index, false);
}

void r800_state::set_default_state()
{
  start_3d();
  set_default_sq();
  sq_setup();
  
  {
    asic_cmd cmd(this);
    
    cmd[SQ_LDS_ALLOC_PS] = 0;
    cmd[SQ_DYN_GPR_RESOURCE_LIMIT_1] = 0;
  }
}

void r800_state::set_spi_defaults()
{
  asic_cmd reg(this);
 
  reg[SPI_VS_OUT_CONFIG] = 0;
  reg[SPI_VS_OUT_ID_0] = 0;
  reg[SPI_INTERP_CONTROL_0] = 0;
  reg[SPI_PS_INPUT_CNTL_0] = {0, 0};
  reg[SPI_PS_IN_CONTROL_0] = {LINEAR_GRADIENT_ENA_bit, 0};
  reg[SPI_INPUT_Z] = 0;
  reg[SPI_FOG_CNTL] = 0;
  reg[SPI_BARYC_CNTL] = LINEAR_CENTROID_ENA__X_ON_AT_CENTROID << LINEAR_CENTROID_ENA_shift;
  reg[SPI_PS_IN_CONTROL_2] = vector<uint32_t>{0, 0, 0};
}

void r800_state::set_draw_auto(int num_indices)
{   
  asic_cmd reg(this);
  reg[VGT_PRIMITIVE_TYPE] = DI_PT_NONE; //or POINTLIST
  reg.packet3(IT_INDEX_TYPE, 1);
  reg.write_dword(DI_INDEX_SIZE_16_BIT); //or 32 bits
  reg.packet3(IT_NUM_INSTANCES, 1);
  reg.write_dword(1);
  reg.packet3(IT_DRAW_INDEX_AUTO, 2);
  reg.write_dword(num_indices);
  reg.write_dword(DI_SRC_SEL_AUTO_INDEX);
}

void r800_state::set_surface_sync(uint32_t sync_type, uint32_t size, uint64_t mc_addr, struct radeon_bo *bo, uint32_t rdomains, uint32_t wdomain)
{
    uint32_t cp_coher_size;
    if (size == 0xffffffff)
	cp_coher_size = 0xffffffff;
    else
	cp_coher_size = ((size + 255) >> 8);

    uint32_t poll_interval = 10;
    
    asic_cmd reg(this);
    
    reg.packet3(IT_SURFACE_SYNC, 4);
    reg.write_dword(sync_type);
    reg.write_dword(cp_coher_size);
    reg.write_dword((mc_addr >> 8));
    reg.write_dword(poll_interval);
    reg.reloc(bo, rdomains, wdomain);
}

void r800_state::set_dummy_render_target()
{
/*   BEGIN_BATCH(3 + 2);
    EREG(CB_COLOR0_BASE + (0x3c * cb_conf->id), (cb_conf->base >> 8));
    RELOC_BATCH(cb_conf->bo, 0, domain);
    END_BATCH();*/
    
    {
      asic_cmd reg(this);
      
      reg[CB_COLOR0_BASE] = 0;
      reg.reloc(dummy_bo_cb, 0, RADEON_GEM_DOMAIN_VRAM);
    }

    /* Set CMASK & FMASK buffer to the offset of color buffer as
     * we don't use those this shouldn't cause any issue and we
     * then have a valid cmd stream
     */
/*    BEGIN_BATCH(3 + 2);
    EREG(CB_COLOR0_CMASK + (0x3c * cb_conf->id), (0     >> 8));
    RELOC_BATCH(cb_conf->bo, 0, domain);
    END_BATCH();*/
    
    {
      asic_cmd reg(this);
      
      reg[CB_COLOR0_CMASK] = 0;
      reg.reloc(dummy_bo_cb, 0, RADEON_GEM_DOMAIN_VRAM);
    }
    
//     BEGIN_BATCH(3 + 2);
//     EREG(CB_COLOR0_FMASK + (0x3c * cb_conf->id), (0     >> 8));
//     RELOC_BATCH(cb_conf->bo, 0, domain);
//     END_BATCH();

    {
      asic_cmd reg(this);
      
      reg[CB_COLOR0_FMASK] = 0;
      reg.reloc(dummy_bo_cb, 0, RADEON_GEM_DOMAIN_VRAM);
    }
    
    /* tiling config */
/*    BEGIN_BATCH(3 + 2);
    EREG(CB_COLOR0_ATTRIB + (0x3c * cb_conf->id), cb_color_attrib);
    RELOC_BATCH(cb_conf->bo, 0, domain);
    END_BATCH();*/
    
   {
      asic_cmd reg(this);
      
      reg[CB_COLOR0_ATTRIB] = CB_COLOR0_ATTRIB__NON_DISP_TILING_ORDER_bit;
      reg.reloc(dummy_bo_cb, 0, RADEON_GEM_DOMAIN_VRAM);
    }
 
/*    BEGIN_BATCH(3 + 2);
    EREG(CB_COLOR0_INFO + (0x3c * cb_conf->id), cb_color_info);
    RELOC_BATCH(cb_conf->bo, 0, domain);
    END_BATCH();*/
    
    {
      asic_cmd reg(this);
      
      reg[CB_COLOR0_INFO] = 
	(COLOR_8 << CB_COLOR0_INFO__FORMAT_shift) |
	(3 << COMP_SWAP_shift) |
	(EXPORT_4C_16BPC << SOURCE_FORMAT_shift) |
	(BLEND_CLAMP_bit);
	
      reg.reloc(dummy_bo_cb, 0, RADEON_GEM_DOMAIN_VRAM);
    }


//     BEGIN_BATCH(33);
//     EREG(CB_COLOR0_PITCH + (0x3c * cb_conf->id), pitch);
//     EREG(CB_COLOR0_SLICE + (0x3c * cb_conf->id), slice);
//     EREG(CB_COLOR0_VIEW + (0x3c * cb_conf->id), 0);
//     EREG(CB_COLOR0_DIM + (0x3c * cb_conf->id), cb_color_dim);
//     EREG(CB_COLOR0_CMASK_SLICE + (0x3c * cb_conf->id), 0);
//     EREG(CB_COLOR0_FMASK_SLICE + (0x3c * cb_conf->id), 0);
//     PACK0(CB_COLOR0_CLEAR_WORD0 + (0x3c * cb_conf->id), 4);
//     E32(0);
//     E32(0);
//     E32(0);
//     E32(0);
//     EREG(CB_TARGET_MASK,                      (cb_conf->pmask << TARGET0_ENABLE_shift));
//     EREG(CB_COLOR_CONTROL,                    (EVERGREEN_ROP[cb_conf->rop] |
// 					       (CB_NORMAL << CB_COLOR_CONTROL__MODE_shift)));
//     EREG(CB_BLEND0_CONTROL,                   cb_conf->blendcntl);
//     END_BATCH();

    {
      asic_cmd reg(this);
      reg[CB_COLOR0_PITCH] = (16 / 8) - 1;
      reg[CB_COLOR0_SLICE] = ((16*16) / 64) - 1;
      reg[CB_COLOR0_VIEW] = 0;
      reg[CB_COLOR0_DIM] =  (15 << WIDTH_MAX_shift) | ( 15 << HEIGHT_MAX_shift);
      reg[CB_COLOR0_CMASK_SLICE] = 0;
      reg[CB_COLOR0_FMASK_SLICE] = 0;
      reg[CB_COLOR0_CLEAR_WORD0] = {0, 0, 0, 0};
      reg[CB_TARGET_MASK] = 8 << TARGET0_ENABLE_shift;
      reg[CB_COLOR_CONTROL] = CB_NORMAL << CB_COLOR_CONTROL__MODE_shift;
      reg[CB_BLEND0_CONTROL] = 0;
    }
}

void r800_state::flush_cs()
{
  //should unmap and unref bo-s first?
  radeon_cs_emit(cs);
  radeon_cs_erase(cs);
}

void r800_state::upload_dummy_ps()
{
  assert(radeon_bo_map(dummy_bo_ps, 1) == 0);
  memcpy(dummy_bo_ps->ptr, dummy_ps_shader_binary, sizeof(dummy_ps_shader_binary));
  radeon_bo_unmap(dummy_bo_ps);
}

void r800_state::set_dummy_scissors()
{
  {
    asic_cmd reg(this);
    
    reg[PA_SC_GENERIC_SCISSOR_TL] = {
      ((0 << PA_SC_GENERIC_SCISSOR_TL__TL_X_shift) |
	(0 << PA_SC_GENERIC_SCISSOR_TL__TL_Y_shift) |
	WINDOW_OFFSET_DISABLE_bit),
      ((16 << PA_SC_GENERIC_SCISSOR_BR__BR_X_shift) |
	(16 << PA_SC_GENERIC_SCISSOR_TL__TL_Y_shift))
    };
  }
  {
    asic_cmd reg(this);
    
    reg[PA_SC_SCREEN_SCISSOR_TL] = {
      ((0 << PA_SC_SCREEN_SCISSOR_TL__TL_X_shift) |
	(0 << PA_SC_SCREEN_SCISSOR_TL__TL_Y_shift)),
      ((16 << PA_SC_SCREEN_SCISSOR_BR__BR_X_shift) |
	(16 << PA_SC_SCREEN_SCISSOR_BR__BR_Y_shift))
    };
  }
  {
    asic_cmd reg(this);
    
    reg[PA_SC_WINDOW_SCISSOR_TL] = {
      ((0 << PA_SC_WINDOW_SCISSOR_TL__TL_X_shift) |
	(0 << PA_SC_WINDOW_SCISSOR_TL__TL_Y_shift) |
	WINDOW_OFFSET_DISABLE_bit),
      ((16 << PA_SC_WINDOW_SCISSOR_BR__BR_X_shift) |
	(16 << PA_SC_WINDOW_SCISSOR_BR__BR_Y_shift))
    };
  }
}

void r800_state::setup_const_cache(int cache_id, struct radeon_bo* cbo, int size, int offset)
{
  assert(cache_id < SQ_ALU_CONST_BUFFER_SIZE_VS_0_num);
  
  {
    asic_cmd reg(this);
    
    reg[SQ_ALU_CONST_BUFFER_SIZE_VS_0+cache_id] = size;
  }
  {
    asic_cmd reg(this);
    assert(offset & 0xFF == 0);
    reg[SQ_ALU_CONST_CACHE_VS_0+cache_id] = offset >> 8;
    reg.reloc(cbo, radeon_bo_get_src_domain(cbo), 0);
  }
  
  assert(cache_id < SQ_ALU_CONST_BUFFER_SIZE_PS_0_num);
  
  {
    asic_cmd reg(this);
    
    reg[SQ_ALU_CONST_BUFFER_SIZE_PS_0+cache_id] = size;
  }
  {
    asic_cmd reg(this);
    assert(offset & 0xFF == 0);
    reg[SQ_ALU_CONST_CACHE_PS_0+cache_id] = offset >> 8;
    reg.reloc(cbo, radeon_bo_get_src_domain(cbo), 0);
  }
}

void r800_state::prepare_compute_shader(compute_shader* sh)
{
  //we actually prepare a PS VS pair, and use the VS

  {
    asic_cmd reg(this);
    reg[SQ_PGM_START_PS] = 0;
    reg.reloc(dummy_bo_ps, RADEON_GEM_DOMAIN_VRAM, RADEON_GEM_DOMAIN_VRAM);
  }
  {
    asic_cmd reg(this);
    reg[SQ_PGM_RESOURCES_PS] = {
      (/*num_gprs*/1 | (0 << STACK_SIZE_shift)),
      SQ_ROUND_NEAREST_EVEN | ALLOW_DOUBLE_DENORM_IN_bit | ALLOW_DOUBLE_DENORM_OUT_bit
    };
  }
  {
    asic_cmd reg(this);
    reg[SQ_PGM_START_VS] = 0;
    reg.reloc(sh->binary_code_bo, RADEON_GEM_DOMAIN_VRAM, RADEON_GEM_DOMAIN_VRAM);
  }
  {
    asic_cmd reg(this);
    reg[SQ_PGM_RESOURCES_VS] = {
      (sh->num_gprs | (sh->stack_size << STACK_SIZE_shift)) | PRIME_CACHE_ENABLE,
      SQ_ROUND_NEAREST_EVEN | ALLOW_DOUBLE_DENORM_IN_bit | ALLOW_DOUBLE_DENORM_OUT_bit
    };
  }
  {
    asic_cmd reg(this);
    
    reg[SQ_LDS_ALLOC_PS] = sh->lds_alloc; //in 32 bit words
    reg[SQ_GLOBAL_GPR_RESOURCE_MGMT_1] = (0 << PS_GGPR_BASE_shift) | (sh->global_gprs << VS_GGPR_BASE_shift);
    reg[SQ_GLOBAL_GPR_RESOURCE_MGMT_2] = 0;
    reg[SQ_THREAD_RESOURCE_MGMT] = (sq_conf.num_ps_threads << NUM_PS_THREADS_shift) | (sh->thread_num << NUM_VS_THREADS_shift);
    reg[SQ_STACK_RESOURCE_MGMT_1] = (1 << NUM_PS_STACK_ENTRIES_shift) | (sh->stack_size << NUM_VS_STACK_ENTRIES_shift);
    reg[SQ_PGM_EXPORTS_PS] = 0; 
  }
}


