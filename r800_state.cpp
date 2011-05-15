#include <iostream>
#include <assert.h>
#include <sys/ioctl.h>
#include "r800_state.h"
#include "radeon_reg.h"
#include "evergreen_reg.h"

using namespace std;

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
  radeon_cs_space_set_flush(cs, (void (*)(void*))radeon_cs_flush_indirect, this);
  
//   TODO: OUTREG(RADEON_RB3D_CNTL, 0); need to map PCI memory for radeon MMIO-> need pciaccess.h
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
    
    asic_cmd cmd(this);
    
    cmd[SQ_DYN_GPR_CNTL_PS_FLUSH_REQ] = 0; // disable dyn gprs
    
    cmd[SQ_CONFIG] = {
      sq_config,
      sq_gpr_resource_mgmt_1,
      sq_gpr_resource_mgmt_2,
      sq_gpr_resource_mgmt_3
    };
    
    cmd[SQ_THREAD_RESOURCE_MGMT] = {
      sq_thread_resource_mgmt,
      sq_thread_resource_mgmt_2,
      sq_stack_resource_mgmt_1,
      sq_stack_resource_mgmt_2,
      sq_stack_resource_mgmt_3      
    };   
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
  {
    asic_cmd cmd(this);
    
    cmd[VGT_PRIMITIVEID_EN] = 0;
    cmd[VGT_MULTI_PRIM_IB_RESET_EN] = 0;
    cmd[VGT_SHADER_STAGES_EN] = 0;
    cmd[VGT_STRMOUT_CONFIG] = {0, 0};
  }
}

void r800_state::flush_cs()
{
  //should unmap and unref bo-s first!
  radeon_cs_emit(cs);
  radeon_cs_erase(cs);
}



