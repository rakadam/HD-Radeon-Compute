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
  KCACHE_BANK0(0) KCACHE_MODE0.CF_KCACHE_LOCK_1;
  BARRIER;
  
  ADD_INT:
    SRC0_SEL.GPR(0) SRC0_CHAN.CHAN_X
    SRC1_SEL.GPR(0) SRC1_CHAN.CHAN_Y
    WRITE_MASK DST_GPR(3) DST_CHAN.CHAN_X;
    LAST;
  
  MULLO_INT:
    SRC0_SEL.GPR(0) SRC0_CHAN.CHAN_Y
    SRC1_SEL.Kcache_bank0(0) SRC1_CHAN.CHAN_X 
    WRITE_MASK DST_GPR(2) DST_CHAN.CHAN_Y
    LAST;
    
  ADD_INT:
    SRC0_SEL.GPR(2) SRC0_CHAN.CHAN_Y
    SRC1_SEL.GPR(0) SRC1_CHAN.CHAN_X
    WRITE_MASK DST_GPR(2) DST_CHAN.CHAN_X
    LAST;
    
    
MEM_RAT_CACHELESS:
        RAT_ID(11) RAT_INST.EXPORT_RAT_INST_STORE_RAW TYPE(1) RW_GPR(3) INDEX_GPR(2) ELEM_SIZE(0);
        COMP_MASK(1) BARRIER ARRAY_SIZE(0);

NOP: 
	BARRIER END_OF_PROGRAM;

end;
