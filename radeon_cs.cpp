#include <cstdlib>
#include <vector>
#include <assert.h>
extern "C" {
#include <drm.h>
#include <xf86drm.h>
#include <radeon_cs_gem.h>
#include <radeon_bo_gem.h>
};

#include "radeon_cs.hpp"
#include "evergreen_reg.h"

#define RADEON_BUFFER_SIZE 65536
#define RADEON_CP_PACKET0 0x00000000
#define RADEON_CP_PACKET1 0x40000000
#define RADEON_CP_PACKET2 0x80000000
#define RADEON_CP_PACKET3 0xC0000000

#define CP_PACKET0(reg, n)                                              \
        (RADEON_CP_PACKET0 | ((n) << 16) | ((reg) >> 2))
#define CP_PACKET1(reg0, reg1)                                          \
        (RADEON_CP_PACKET1 | (((reg1) >> 2) << 11) | ((reg0) >> 2))
#define CP_PACKET2()                                                    \
        (RADEON_CP_PACKET2)
#define CP_PACKET3(pkt, n)                                              \
        (RADEON_CP_PACKET3 | (pkt) | ((n) << 16))

radeon_cmd_stream::regsetter::regsetter(radeon_cmd_stream *stream, uint32_t reg) : stream(stream), reg(reg)
{
}

void radeon_cmd_stream::regsetter::operator=(uint32_t val)
{
  stream->set_reg(reg, val);
}

void radeon_cmd_stream::regsetter::operator=(float val)
{
  stream->set_reg(reg, val);
}
    
void radeon_cmd_stream::regsetter::operator=(int val)
{
  stream->set_reg(reg, val);
}

void radeon_cmd_stream::regsetter::operator=(const std::vector<uint32_t>& vec)
{
  stream->set_regs(reg, vec);
}


radeon_cmd_stream::radeon_cmd_stream(int fd)
{
  gem = radeon_cs_manager_gem_ctor(fd);
  assert(gem);
  cs = radeon_cs_create((radeon_cs_manager*)gem, RADEON_BUFFER_SIZE/4);
  assert(cs);
}

radeon_cmd_stream::radeon_cmd_stream(radeon_cmd_stream&& b) : cs(b.cs), gem(b.gem)
{
  b.cs = NULL;
  b.gem = NULL;
}

radeon_cmd_stream::regsetter radeon_cmd_stream::operator[](uint32_t index)
{
  return regsetter(this, index);
}

void radeon_cmd_stream::reloc(struct radeon_bo * bo, uint32_t rd, uint32_t wd)
{
  radeon_cs_write_reloc((radeon_cs*)cs, bo, rd, wd, 0);
}

void radeon_cmd_stream::write_dword(uint32_t dword)
{
  radeon_cs_write_dword((radeon_cs*)cs, dword);
}

void radeon_cmd_stream::write_float(float f)
{
  union uni_ {
    float f;
    uint32_t u32;
  } uni;
  
  uni.f = f;
  write_dword(uni.u32);
}

void radeon_cmd_stream::packet3(uint32_t cmd, uint32_t num)
{
  assert(num);
  write_dword(RADEON_CP_PACKET3 | ((cmd) << 8) | ((((num) - 1) & 0x3fff) << 16));
}

void radeon_cmd_stream::packet3(uint32_t cmd, std::vector<uint32_t> vals)
{
  assert(vals.size());
  packet3(cmd, vals.size());
  
  for (int i = 0; i < int(vals.size()); i++)
  {
    write_dword(vals[i]);
  }
}

void radeon_cmd_stream::packet0(uint32_t reg, uint32_t num)
{
  if ((reg) >= SET_CONFIG_REG_offset && (reg) < SET_CONFIG_REG_end)
  {
    packet3(IT_SET_CONFIG_REG, (num) + 1);			
    write_dword(((reg) - SET_CONFIG_REG_offset) >> 2);                  
  }
  else if ((reg) >= SET_CONTEXT_REG_offset && (reg) < SET_CONTEXT_REG_end)
  { 
    packet3(IT_SET_CONTEXT_REG, (num) + 1);			
    write_dword(((reg) - SET_CONTEXT_REG_offset) >> 2);			
  }
  else if ((reg) >= SET_RESOURCE_offset && (reg) < SET_RESOURCE_end)
  { 
    packet3(IT_SET_RESOURCE, num + 1);				
    write_dword(((reg) - SET_RESOURCE_offset) >> 2);			
  }
  else if ((reg) >= SET_SAMPLER_offset && (reg) < SET_SAMPLER_end)
  { 
    packet3(IT_SET_SAMPLER, (num) + 1);				
    write_dword((reg - SET_SAMPLER_offset) >> 2);			
  }
  else if ((reg) >= SET_CTL_CONST_offset && (reg) < SET_CTL_CONST_end)
  { 
    packet3(IT_SET_CTL_CONST, (num) + 1);			
    write_dword(((reg) - SET_CTL_CONST_offset) >> 2);		
  }
  else if ((reg) >= SET_LOOP_CONST_offset && (reg) < SET_LOOP_CONST_end)
  { 
    packet3(IT_SET_LOOP_CONST, (num) + 1);			
    write_dword(((reg) - SET_LOOP_CONST_offset) >> 2);
  }
  else if ((reg) >= SET_BOOL_CONST_offset && (reg) < SET_BOOL_CONST_end)
  { 
    packet3(IT_SET_BOOL_CONST, (num) + 1);
    write_dword(((reg) - SET_BOOL_CONST_offset) >> 2);
  }
  else
  {
    write_dword(CP_PACKET0 ((reg), (num) - 1));
  }
}

void radeon_cmd_stream::set_reg(uint32_t reg, uint32_t val)
{
    packet0(reg, 1);
    write_dword(val);
}

void radeon_cmd_stream::set_reg(uint32_t reg, int val)
{
    packet0(reg, 1);
    write_dword(val);
}

void radeon_cmd_stream::set_reg(uint32_t reg, float val)
{
    packet0(reg, 1);
    write_float(val);
}

void radeon_cmd_stream::set_regs(uint32_t reg, std::vector<uint32_t> vals)
{
  packet0(reg, vals.size());
  
  for (int i = 0; i < int(vals.size()); i++)
  {
    write_dword(vals[i]);
  }
}

radeon_cmd_stream::~radeon_cmd_stream()
{
  radeon_cs_destroy((radeon_cs*)cs);
  radeon_cs_manager_gem_dtor((radeon_cs_manager*)gem);
}
