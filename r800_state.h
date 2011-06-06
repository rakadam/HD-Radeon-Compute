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

#ifndef R800_STATE_H
#define R800_STATE_H
#include <cstdlib>
#include <vector>
extern "C" {
#include <drm.h>
#include <xf86drm.h>
#include <radeon_cs_gem.h>
#include <radeon_bo_gem.h>
};

#include "cs_image.h"
#include "radeon_cs.hpp"

#include <cstdint>

typedef enum {
    CHIP_FAMILY_UNKNOW,
    CHIP_FAMILY_LEGACY,
    CHIP_FAMILY_RADEON,
    CHIP_FAMILY_RV100,
    CHIP_FAMILY_RS100,    /* U1 (IGP320M) or A3 (IGP320)*/
    CHIP_FAMILY_RV200,
    CHIP_FAMILY_RS200,    /* U2 (IGP330M/340M/350M) or A4 (IGP330/340/345/350), RS250 (IGP 7000) */
    CHIP_FAMILY_R200,
    CHIP_FAMILY_RV250,
    CHIP_FAMILY_RS300,    /* RS300/RS350 */
    CHIP_FAMILY_RV280,
    CHIP_FAMILY_R300,
    CHIP_FAMILY_R350,
    CHIP_FAMILY_RV350,
    CHIP_FAMILY_RV380,    /* RV370/RV380/M22/M24 */
    CHIP_FAMILY_R420,     /* R420/R423/M18 */
    CHIP_FAMILY_RV410,    /* RV410, M26 */
    CHIP_FAMILY_RS400,    /* xpress 200, 200m (RS400) Intel */
    CHIP_FAMILY_RS480,    /* xpress 200, 200m (RS410/480/482/485) AMD */
    CHIP_FAMILY_RV515,    /* rv515 */
    CHIP_FAMILY_R520,    /* r520 */
    CHIP_FAMILY_RV530,    /* rv530 */
    CHIP_FAMILY_R580,    /* r580 */
    CHIP_FAMILY_RV560,   /* rv560 */
    CHIP_FAMILY_RV570,   /* rv570 */
    CHIP_FAMILY_RS600,
    CHIP_FAMILY_RS690,
    CHIP_FAMILY_RS740,
    CHIP_FAMILY_R600,    /* r600 */
    CHIP_FAMILY_RV610,
    CHIP_FAMILY_RV630,
    CHIP_FAMILY_RV670,
    CHIP_FAMILY_RV620,
    CHIP_FAMILY_RV635,
    CHIP_FAMILY_RS780,
    CHIP_FAMILY_RS880,
    CHIP_FAMILY_RV770,   /* r700 */
    CHIP_FAMILY_RV730,
    CHIP_FAMILY_RV710,
    CHIP_FAMILY_RV740,
    CHIP_FAMILY_CEDAR,   /* evergreen */
    CHIP_FAMILY_REDWOOD,
    CHIP_FAMILY_JUNIPER,
    CHIP_FAMILY_CYPRESS,
    CHIP_FAMILY_HEMLOCK,
    CHIP_FAMILY_PALM,
    CHIP_FAMILY_BARTS,
    CHIP_FAMILY_TURKS,
    CHIP_FAMILY_CAICOS,
    CHIP_FAMILY_CAYMAN,
    CHIP_FAMILY_LAST
} RADEONChipFamily;

typedef bool bool32;

/* Sequencer / thread handling */
typedef struct {
    int ps_prio;
    int vs_prio;
    int gs_prio;
    int es_prio;
    int hs_prio;
    int ls_prio;
    int cs_prio;
    int num_ps_gprs;
    int num_vs_gprs;
    int num_gs_gprs;
    int num_es_gprs;
    int num_hs_gprs;
    int num_ls_gprs;
    int num_cs_gprs;
    int num_temp_gprs;
    int num_ps_threads;
    int num_vs_threads;
    int num_gs_threads;
    int num_es_threads;
    int num_hs_threads;
    int num_ls_threads;
    int num_ps_stack_entries;
    int num_vs_stack_entries;
    int num_gs_stack_entries;
    int num_es_stack_entries;
    int num_hs_stack_entries;
    int num_ls_stack_entries;
} sq_config_t;

/* Color buffer / render target */
typedef struct {
    int id;
    int w;
    int h;
    uint64_t base;
    int format;
    int endian;
    int array_mode;						// tiling
    int non_disp_tiling;
    int number_type;
    int read_size;
    int comp_swap;
    int tile_mode;
    int blend_clamp;
    int clear_color;
    int blend_bypass;
    int simple_float;
    int round_mode;
    int tile_compact;
    int source_format;
    int resource_type;
    int fast_clear;
    int compression;
    int rat;
    /* 2D related CB state */
    uint32_t pmask;
    int rop;
    int blend_enable;
    uint32_t blendcntl;
    struct radeon_bo *bo;
} cb_config_t;

/* Shader */
typedef struct {
    uint64_t shader_addr;
    uint32_t shader_size;
    int num_gprs;
    int stack_size;
    int dx10_clamp;
    int clamp_consts;
    int export_mode;
    int uncached_first_inst;
    int single_round;
    int double_round;
    int allow_sdi;
    int allow_sd0;
    int allow_ddi;
    int allow_ddo;
    struct radeon_bo *bo;
} shader_config_t;

/* Shader consts */
typedef struct {
    int type;
    int size_bytes;
    uint64_t const_addr;
    struct radeon_bo *bo;
} const_config_t;

/* Vertex buffer / vtx resource */
typedef struct {
    int id;
    uint32_t offset;
    uint64_t vb_addr;
    uint32_t vtx_num_entries;
    uint32_t vtx_size_dw;
    int clamp_x;
    int format;
    int num_format_all;
    int format_comp_all;
    int srf_mode_all;
    int endian;
    int mem_req_size;
    int dst_sel_x;
    int dst_sel_y;
    int dst_sel_z;
    int dst_sel_w;
    int uncached;
    struct radeon_bo *bo;
} vtx_resource_t;

/* Texture resource */
typedef struct {
    int id;
    int w;
    int h;
    int pitch;
    int depth;
    int dim;
    int array_mode;
    int tile_type;
    int format;
    uint64_t base;
    uint64_t mip_base;
    uint32_t size;
    int format_comp_x;
    int format_comp_y;
    int format_comp_z;
    int format_comp_w;
    int num_format_all;
    int srf_mode_all;
    int force_degamma;
    int endian;
    int dst_sel_x;
    int dst_sel_y;
    int dst_sel_z;
    int dst_sel_w;
    int base_level;
    int last_level;
    int base_array;
    int last_array;
    int perf_modulation;
    int interlaced;
    int min_lod;
    struct radeon_bo *bo;
    struct radeon_bo *mip_bo;
} tex_resource_t;

/* Texture sampler */
typedef struct {
    int				id;
    /* Clamping */
    int				clamp_x, clamp_y, clamp_z;
    int		       		border_color;
    /* Filtering */
    int				xy_mag_filter, xy_min_filter;
    int				z_filter;
    int				mip_filter;
    bool32			high_precision_filter;	/* ? */
    int				perf_mip;		/* ? 0-7 */
    int				perf_z;			/* ? 3 */
    /* LoD selection */
    int				min_lod, max_lod;	/* 0-0x3ff */
    int                         lod_bias;		/* 0-0xfff (signed?) */
    int                         lod_bias2;		/* ? 0-0xfff (signed?) */
    bool32			lod_uses_minor_axis;	/* ? */
    /* Other stuff */
    bool32			point_sampling_clamp;	/* ? */
    bool32			tex_array_override;	/* ? */
    bool32                      mc_coord_truncate;	/* ? */
    bool32			force_degamma;		/* ? */
    bool32			fetch_4;		/* ? */
    bool32			sample_is_pcf;		/* ? */
    bool32			type;			/* ? */
    int				depth_compare;		/* only depth textures? */
    int				chroma_key;
    int                         truncate_coord;
    bool32                      disable_cube_wrap;
} tex_sampler_t;

class r800_state
{
  int fd;
  struct radeon_bo_manager * bom;
  radeon_cmd_stream cs;
  
  sq_config_t sq_conf;
  struct drm_radeon_gem_info mminfo;
  
  RADEONChipFamily ChipFamily;
  
  struct radeon_bo *dummy_bo, *dummy_bo_ps, *dummy_bo_cb, *dummy_vbo;
  
  public:
    r800_state(int fd);
    ~r800_state();
    
    struct radeon_bo *bo_open(uint32_t size,
                                 uint32_t alignment,
                                 uint32_t domains,
                                 uint32_t flags);
        
    struct radeon_bo *bo_open(uint32_t handle, uint32_t size, uint32_t alignment, uint32_t domains, uint32_t flags);
    
    int bo_is_referenced_by_cs(struct radeon_bo *bo);

    void get_master();
    void drop_master();
    void start_3d();
    void sq_setup();
    void set_default_sq();
    void set_default_state();
    void set_spi_defaults();
    void set_pa_defaults();
    void set_vgt_defaults();
    void set_db_defaults();
    void set_sx_defaults();
    
    void set_lds(int num_lds, int size, int num_waves);
    void set_gds(uint32_t addr, uint32_t size);
    void set_export(radeon_bo* bo, int offset, int size);
    void set_tmp_ring(radeon_bo* bo, int offset, int size);    
    void select_se(int se_index, bool broadcast_writed);
    void direct_dispatch(int groupnum, int local_size);
    
    void set_draw_auto(int num_indices);
    void set_dummy_render_target();
    void set_rat(int id, radeon_bo* bo, int start, int size);
    void flush_cs();
    void upload_dummy_ps();
    void set_dummy_scissors();
   
    void set_surface_sync(uint32_t sync_type, uint32_t size, uint64_t mc_addr, struct radeon_bo *bo, uint32_t rdomains, uint32_t wdomain);
    void set_vtx_resource(vtx_resource_t *res, uint32_t domain);

    void setup_const_cache(int cache_id, struct radeon_bo* cbo, int size, int offset); //for VS only for now, later CS
    
    void prepare_compute_shader(compute_shader* sh); //A VS for now.. and a dummy PS
    
    void execute_shader(compute_shader* sh);
};




#endif
