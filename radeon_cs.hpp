#ifndef RADEON_CS_HPP
#define RADEON_CS_HPP

struct radeon_bo;

class radeon_cmd_stream
{
private:
  void packet3(uint32_t cmd, uint32_t num);
  void *cs;
  void *gem;
  
public:
  struct regsetter
  {
    uint32_t reg;
    radeon_cmd_stream *stream;
    regsetter(radeon_cmd_stream *stream, uint32_t reg);
    void operator=(uint32_t val);
    void operator=(float val);
    void operator=(int val);
    void operator=(const std::vector<uint32_t>& vec);
  };

  radeon_cmd_stream(int fd);
  radeon_cmd_stream(const radeon_cmd_stream&) = delete;
  radeon_cmd_stream(radeon_cmd_stream&&);
  regsetter operator[](uint32_t index);
  
  void reloc(struct radeon_bo *bo, uint32_t rd, uint32_t wd);
  void write_dword(uint32_t dword);
  void write_float(float f);
  void packet3(uint32_t cmd, std::vector<uint32_t> vals);
  void packet0(uint32_t reg, uint32_t num);
  void set_reg(uint32_t reg, uint32_t val);
  void set_reg(uint32_t reg, int val);
  void set_reg(uint32_t reg, float val);
  void set_regs(uint32_t reg, std::vector<uint32_t> vals);  
  
  ~radeon_cmd_stream();
};

#endif
