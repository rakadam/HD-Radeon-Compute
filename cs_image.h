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

#ifndef CS_IMAGE_H
#define CS_IMAGE_H

#include <cstdint>
#include <vector>
#include <string>

struct cs_image_header
{
  uint32_t magic;
  uint32_t chksum;
  uint32_t size; //size of the binary image, excluding the header
  uint32_t lds_alloc;
  uint32_t num_gprs;
  uint32_t temp_gprs;
  uint32_t global_gprs;
  uint32_t stack_size; //stack size in enties
  uint32_t thread_num; //how much wavefronts are allowed per SIMD
  uint32_t dyn_gpr_limit;
  uint32_t reserved[6];
} __attribute__((packed));

struct r800_state;
struct radeon_bo;

struct compute_shader
{
  compute_shader(r800_state* state, const std::vector<char>& binary);
  compute_shader(r800_state* state, std::string fname); //open raw for now

  struct radeon_bo* binary_code_bo;
  
  int alloc_size; //allocated bo size in bytes
  int lds_alloc; //local memory per workgroup in 32bit words
  int num_gprs; //number or GPRs used by the shader
  int temp_gprs; //number of temporary GRPs
  int global_gprs; //number of global GPRs (on a SIMD)
  int stack_size; //size of the stack to allocate
  int thread_num; //per SIMD
  int dyn_gpr_limit; //WTF?
};


// compute_shader cs_read_from_file(std::string fname);

#endif



