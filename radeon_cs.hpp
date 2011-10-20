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

#ifndef RADEON_CS_HPP
#define RADEON_CS_HPP

#include <vector>
#include <cstdint>

struct radeon_bo;

class radeon_cmd_stream
{
private:
  void packet3(uint32_t cmd, uint32_t num);
  void *cs;
  void *gem;
  
  struct token
  {
    token(uint32_t dw) : bo_reloc(false) ,dw(dw) {}
    token(struct radeon_bo *bo, uint32_t rd, uint32_t wd) : bo_reloc(true), dw(0), bo(bo), rd(rd), wd(wd) {}
    bool bo_reloc;
    uint32_t dw;
    struct radeon_bo *bo;
    uint32_t rd;
    uint32_t wd;
  };
  
  std::vector<token> queue;
  int reloc_num;
  std::vector<token> pers_bos;
  
  static void radeon_cs_flush_indirect(radeon_cmd_stream*);
  
public:
  class regsetter
  {
    friend class radeon_cmd_stream;
      uint32_t reg;
      radeon_cmd_stream *stream;
      regsetter(radeon_cmd_stream *stream, uint32_t reg);
    public:
      void operator=(uint32_t val);
      void operator=(float val);
      void operator=(int val);
      void operator=(const std::vector<uint32_t>& vec);
      void operator=(const std::initializer_list<uint32_t>& vec);
  };

  radeon_cmd_stream(int fd);
  radeon_cmd_stream(const radeon_cmd_stream&) = delete;
  radeon_cmd_stream(radeon_cmd_stream&&);
  regsetter operator[](uint32_t index);
  
  void reloc(struct radeon_bo *bo, uint32_t rd, uint32_t wr);
  void write_dword(uint32_t dword);
  void write_float(float f);
  void packet3(uint32_t cmd, std::vector<uint32_t> vals);
  void packet3(uint32_t cmd, std::initializer_list<uint32_t> vals);
  void packet0(uint32_t reg, uint32_t num);
  void set_reg(uint32_t reg, uint32_t val);
  void set_reg(uint32_t reg, int val);
  void set_reg(uint32_t reg, float val);
  void set_regs(uint32_t reg, std::vector<uint32_t> vals);  
  
  void space_check();
  void cs_emit();
  void cs_erase();
  void add_persistent_bo(struct radeon_bo *bo, uint32_t read_domain, uint32_t write_domain);
  void set_limit(uint32_t domain, uint64_t limit);
  ~radeon_cmd_stream();
};

#endif
