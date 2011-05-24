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

#include <cstdlib>
#include <vector>
#include <assert.h>
extern "C" {
#include <drm.h>
#include <xf86drm.h>
#include <radeon_cs_gem.h>
#include <radeon_bo_gem.h>
};

#include <iostream>
#include <stdexcept>
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

using namespace std;

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

void radeon_cmd_stream::regsetter::operator=(const std::initializer_list<uint32_t>& vec)
{
  stream->set_regs(reg, vec);  
}

void radeon_cmd_stream::radeon_cs_flush_indirect(radeon_cmd_stream* stream)
{
  radeon_cs_emit((radeon_cs*)stream->cs);
  radeon_cs_erase((radeon_cs*)stream->cs);
}

radeon_cmd_stream::radeon_cmd_stream(int fd)
{
  gem = radeon_cs_manager_gem_ctor(fd);
  assert(gem);
  cs = radeon_cs_create((radeon_cs_manager*)gem, RADEON_BUFFER_SIZE/4);
  assert(cs);
  reloc_num = 0;
  radeon_cs_space_set_flush((radeon_cs*)cs, (void (*)(void*))radeon_cs_flush_indirect, this);
}

radeon_cmd_stream::radeon_cmd_stream(radeon_cmd_stream&& b)
  : cs(b.cs), gem(b.gem), queue(b.queue), reloc_num(b.reloc_num), pers_bos(b.pers_bos)
{
  b.cs = NULL;
  b.gem = NULL;
}

radeon_cmd_stream::regsetter radeon_cmd_stream::operator[](uint32_t index)
{
  return regsetter(this, index);
}

void radeon_cmd_stream::reloc(struct radeon_bo * bo, uint32_t rd, uint32_t wr)
{
  queue.push_back(token(bo, rd, wr));
  reloc_num++;
}

void radeon_cmd_stream::write_dword(uint32_t dword)
{
  queue.push_back(token(dword));
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

void radeon_cmd_stream::packet3(uint32_t cmd, std::initializer_list<uint32_t> vals)
{
  packet3(cmd, vector<uint32_t>(vals));
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
  else //if its actually a packet0
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

void radeon_cmd_stream::space_check()
{
  for (int i = 0; i < int(pers_bos.size()); i++)
  {
//     cout << pers_bos[i].bo->size << endl;
    radeon_cs_space_add_persistent_bo((radeon_cs*)cs, pers_bos[i].bo, pers_bos[i].rd, pers_bos[i].wd);
  }
  
  int ret = radeon_cs_space_check((radeon_cs*)cs);
  
  if (ret)
  {
    cerr << ret << " " << pers_bos.size() << endl;
    throw runtime_error("BO space check failure!");
  }
}

void radeon_cmd_stream::cs_emit()
{
  int ndw = queue.size() + reloc_num;
  
  radeon_cs_begin((radeon_cs*)cs, ndw, __FILE__, __func__, __LINE__);
  
  for (int i = 0; i < int(queue.size()); i++)
  {
    if (queue[i].bo_reloc)
    {
      if (radeon_cs_write_reloc((radeon_cs*)cs, queue[i].bo, queue[i].rd, queue[i].wd, 0))
      {
	throw runtime_error("Relocation error");
      }
    }
    else
    {
      radeon_cs_write_dword((radeon_cs*)cs, queue[i].dw); 
    }
  }
  
  radeon_cs_end((radeon_cs*)cs, __FILE__, __func__, __LINE__);
  
  radeon_cs_emit((radeon_cs*)cs);
  radeon_cs_erase((radeon_cs*)cs);
}

void radeon_cmd_stream::cs_erase()
{
  queue.clear();
  reloc_num = 0;
}

void radeon_cmd_stream::add_persistent_bo(struct radeon_bo *bo,
  uint32_t read_domain,
  uint32_t write_domain)
{
  pers_bos.push_back(token(bo, read_domain, write_domain));
}

void radeon_cmd_stream::set_limit(uint32_t domain, uint32_t limit)
{
  radeon_cs_set_limit((radeon_cs*)cs, domain, limit);
}

radeon_cmd_stream::~radeon_cmd_stream()
{
  radeon_cs_destroy((radeon_cs*)cs);
  radeon_cs_manager_gem_dtor((radeon_cs_manager*)gem);
}
