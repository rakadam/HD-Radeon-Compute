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
 *          Carl-Philip Hänsch <s3734770@mail.zih.tu-dresden.de>
 *
 *
 */

// ToDo: Thread group init
// gpr(0) = x, y, matwid*x, matwid*y
// gpr(1) = meta_x, meta_y
// gpr(2) = matwid*x+y
// gpr(3) = sum, 4, 0
// gpr(4) = a (oder idx)
// gpr(5) = b (oder idx)
// gpr(6) = result
ALU:
  KCACHE_BANK0(0) KCACHE_MODE0.CF_KCACHE_LOCK_1;
  BARRIER;

  // calc position x y from x and meta_x
  MULLO_INT:
    SRC0_SEL.GPR(1) SRC0_CHAN.CHAN_X;
    SRC1_SEL.Kcache_bank0(0) SRC1_CHAN.CHAN_Y;
    LAST;
    WRITE_MASK DST_GPR(1) DST_CHAN.CHAN_X;
  MULLO_INT:
    SRC0_SEL.GPR(1) SRC0_CHAN.CHAN_Y;
    SRC1_SEL.Kcache_bank0(0) SRC1_CHAN.CHAN_Y;
    LAST;
    WRITE_MASK DST_GPR(1) DST_CHAN.CHAN_Y;

  ADD_INT:
    SRC0_SEL.GPR(1) SRC0_CHAN.CHAN_X;
    SRC1_SEL.GPR(0) SRC1_CHAN.CHAN_X;
    WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_X;
  ADD_INT:
    SRC0_SEL.GPR(1) SRC0_CHAN.CHAN_Y;
    SRC1_SEL.GPR(0) SRC1_CHAN.CHAN_Y;
    WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_Y;
    LAST;

  // calc output position to write to
  MULLO_INT:
    SRC0_SEL.GPR(0) SRC0_CHAN.CHAN_X; ///< local_ID.x
    SRC1_SEL.Kcache_bank0(0) SRC1_CHAN.CHAN_X;
    WRITE_MASK DST_GPR(2) DST_CHAN.CHAN_Y;
    LAST;

  MULLO_INT:
    SRC0_SEL.GPR(0) SRC0_CHAN.CHAN_X; ///< local_ID.x
    SRC1_SEL.Kcache_bank0(0) SRC1_CHAN.CHAN_X;
    WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_Z;
    LAST;

  MULLO_INT:
    SRC0_SEL.GPR(0) SRC0_CHAN.CHAN_Y; ///< local_ID.x
    SRC1_SEL.Kcache_bank0(0) SRC1_CHAN.CHAN_X;
    LAST;
    WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_W;

  MOV:
    SRC0_SEL.ALU_SRC_0 SRC0_CHAN.CHAN_X;
    WRITE_MASK DST_GPR(3) DST_CHAN.CHAN_X;
  MOV:
    SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_X;
    WRITE_MASK DST_GPR(3) DST_CHAN.CHAN_Z;
  MOV:
    SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_Y;
    WRITE_MASK DST_GPR(3) DST_CHAN.CHAN_Y;

    LAST;
    0x0;
    0x4;

  ADD_INT:
    SRC0_SEL.GPR(2) SRC0_CHAN.CHAN_Y;
    SRC1_SEL.GPR(0) SRC1_CHAN.CHAN_Y;
    LAST;
    WRITE_MASK DST_GPR(2) DST_CHAN.CHAN_X;


LOOP_START:
       ADDR(@1);
       CF_CONST(0);
       BARRIER;
@2


/// ToDo: ALU_SRC_LOOP_IDX
ALU:
  BARRIER;

  ADD_INT:
    SRC0_SEL.GPR(0) SRC0_CHAN.CHAN_Z
    SRC1_SEL.ALU_SRC_LOOP_IDX SRC0_CHAN.CHAN_X
    LAST;
    WRITE_MASK DST_GPR(4) DST_CHAN.CHAN_X;
  ADD_INT:
    SRC0_SEL.GPR(0) SRC0_CHAN.CHAN_W
    SRC1_SEL.ALU_SRC_LOOP_IDX SRC0_CHAN.CHAN_X
    LAST;
    WRITE_MASK DST_GPR(5) DST_CHAN.CHAN_X;

TC:
  BARRIER;

  FETCH:
    FETCH_TYPE.VTX_FETCH_NO_INDEX_OFFSET BUFFER_ID(0) SRC_GPR(4) SRC_SEL_X.SEL_X MEGA_FETCH_COUNT(15);
    DST_GPR(4) DST_SEL_X.SEL_X DST_SEL_Y.SEL_Y DST_SEL_Z.SEL_Z DST_SEL_W.SEL_W USE_CONST_FIELDS;
    MEGA_FETCH;

  FETCH:
    FETCH_TYPE.VTX_FETCH_NO_INDEX_OFFSET BUFFER_ID(1) SRC_GPR(5) SRC_SEL_X.SEL_X MEGA_FETCH_COUNT(15);
    DST_GPR(5) DST_SEL_X.SEL_X DST_SEL_Y.SEL_Y DST_SEL_Z.SEL_Z DST_SEL_W.SEL_W USE_CONST_FIELDS;
    MEGA_FETCH;


ALU:
  BARRIER;

  DOT4_IEEE:
    SRC0_SEL.GPR(4) SRC0_CHAN.CHAN_X;
    SRC1_SEL.GPR(5) SRC1_CHAN.CHAN_X;
    WRITE_MASK DST_GPR(6) DST_CHAN.CHAN_X;

  DOT4_IEEE:
    SRC0_SEL.GPR(4) SRC0_CHAN.CHAN_Y;
    SRC1_SEL.GPR(5) SRC1_CHAN.CHAN_Y;
    WRITE_MASK DST_GPR(6) DST_CHAN.CHAN_Y;

  DOT4_IEEE:
    SRC0_SEL.GPR(4) SRC0_CHAN.CHAN_Z;
    SRC1_SEL.GPR(5) SRC1_CHAN.CHAN_Z;
    WRITE_MASK DST_GPR(6) DST_CHAN.CHAN_Z;

  DOT4_IEEE:
    SRC0_SEL.GPR(4) SRC0_CHAN.CHAN_W;
    SRC1_SEL.GPR(5) SRC1_CHAN.CHAN_W;
    WRITE_MASK DST_GPR(6) DST_CHAN.CHAN_W;
    LAST;

  ADD:
    SRC0_SEL.GPR(6) SRC0_CHAN.CHAN_X;
    SRC1_SEL.GPR(3) SRC0_CHAN.CHAN_X;
    WRITE_MASK DST_GPR(3) DST_CHAN.CHAN_X;
    LAST;

LOOP_END:
         ADDR(@2);
         BARRIER;

@1


MEM_RAT_CACHELESS:
  RAT_ID(11) RAT_INST.EXPORT_RAT_INST_STORE_RAW TYPE(1) RW_GPR(3) INDEX_GPR(2) ELEM_SIZE(0);
  COMP_MASK(1) BARRIER ARRAY_SIZE(0);

NOP:
  BARRIER END_OF_PROGRAM;

end;
