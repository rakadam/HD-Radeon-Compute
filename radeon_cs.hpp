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
  void set_limit(uint32_t domain, uint32_t limit);
  ~radeon_cmd_stream();
};

#endif
