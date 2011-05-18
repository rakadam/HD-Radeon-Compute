#ifndef CS_IMAGE_H
#define CS_IMAGE_H

#include <cstdint>

struct cs_image_header
{
  uint32_t magic;
  uint32_t chksum;
  uint32_t size; //size of the binary image, excluding the header
  uint32_t lds_alloc;
  uint32_t num_gprs;
  uint32_t temp_gprs;
  uint32_t global_gprs;
  uint32_t stack_size;
  uint32_t thread_num;
  uint32_t dyn_gpr_limit;
  uint32_t reserved[6];
} __attribute__((packed));

struct r800_state;
struct radeon_bo;

struct compute_shader
{
  compute_shader(r800_state* state, const std::vector<char>& binary);
  
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

#endif



