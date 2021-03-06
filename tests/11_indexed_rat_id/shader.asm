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

ALU:
  BARRIER;
  
  MOV:
    SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_X;
    WRITE_MASK DST_GPR(1) DST_CHAN.CHAN_X;
  MOV:
    SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_X;
    WRITE_MASK DST_GPR(1) DST_CHAN.CHAN_Y;
  MOV:
    SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_X;
    WRITE_MASK DST_GPR(1) DST_CHAN.CHAN_Z;
  MOV:
    SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_X;
    WRITE_MASK DST_GPR(1) DST_CHAN.CHAN_W;
    LAST;
    0x0;
    0x0;
  
TC:
  BARRIER;

  FETCH:
    FETCH_TYPE.VTX_FETCH_NO_INDEX_OFFSET BUFFER_ID(0) SRC_GPR(1) SRC_SEL_X.SEL_X MEGA_FETCH_COUNT(15);
    DST_GPR(0) DST_SEL_X.SEL_X DST_SEL_Y.SEL_Y DST_SEL_Z.SEL_Z DST_SEL_W.SEL_W USE_CONST_FIELDS;
    MEGA_FETCH;

ALU:
  BARRIER;

  MOV:
    SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_X;
    WRITE_MASK DST_GPR(2) DST_CHAN.CHAN_X;
    LAST;
    0x0;
    0x0;

ALU:
  BARRIER;

  SET_CF_IDX0:
    SRC0_SEL.GPR(2) SRC0_CHAN.CHAN_X;
    DST_CHAN.CHAN_X;
    LAST;

MEM_RAT_CACHELESS:
  RAT_ID(0) RAT_INDEX_MODE.CF_INDEX_0 RAT_INST.EXPORT_RAT_INST_STORE_RAW TYPE(1) RW_GPR(0) INDEX_GPR(1) ELEM_SIZE(3);
  COMP_MASK(15) BARRIER ARRAY_SIZE(0);

NOP: 
  BARRIER END_OF_PROGRAM;

end;
