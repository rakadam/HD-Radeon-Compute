// VC:
//         BARRIER;
// 
//         FETCH:
//                 FETCH_TYPE.VTX_FETCH_VERTEX_DATA SRC_SEL_X.SEL_X MEGA_FETCH_COUNT(7);
//                 DST_GRP(1) DST_SEL_X.SEL_X DST_SEL_Y.SEL_Y DST_SEL_Z.SEL_0 DST_SEL_W.SEL_1 DATA_FORMAT.FMT_32_32_FLOAT NUM_FORMAT_ALL.NUM_FORMAT_SCALED FORMAT_COMP_ALL.FORMAT_COMP_SIGNED;
//                 MEGA_FETCH;


// NOP: BARRIER;

ALU: BARRIER;
        MULLO_INT:
                SRC0_SEL.GPR(0) SRC0_CHAN.CHAN_X LAST;
                SRC1_SEL.ALU_SRC_LITERAL SRC1_CHAN.CHAN_X;
                WRITE_MASK DST_GPR(5) DST_CHAN.CHAN_X;
        0x00000004;
        0x00000000;
                
        MULLO_INT:
                SRC0_SEL.GPR(0) SRC0_CHAN.CHAN_Y LAST;
                SRC1_SEL.ALU_SRC_LITERAL SRC1_CHAN.CHAN_X;
                WRITE_MASK DST_GPR(5) DST_CHAN.CHAN_Y;
        0x00000008;
        0x00000000;
        
        MULLO_INT:
                SRC0_SEL.GPR(1) SRC0_CHAN.CHAN_X LAST;
                SRC1_SEL.ALU_SRC_LITERAL SRC1_CHAN.CHAN_X;
                WRITE_MASK DST_GPR(5) DST_CHAN.CHAN_Z;
        0x00000010;
        0x00000000;
        
        MULLO_INT:
                SRC0_SEL.GPR(1) SRC0_CHAN.CHAN_Y LAST;
                SRC1_SEL.ALU_SRC_LITERAL SRC1_CHAN.CHAN_X;
                WRITE_MASK DST_GPR(5) DST_CHAN.CHAN_W;
        0x00000020;
        0x00000000;
        
        ADD_INT:
            SRC0_SEL.GPR(5) SRC0_CHAN.CHAN_X LAST;
            SRC1_SEL.GPR(5) SRC0_CHAN.CHAN_Y;
            WRITE_MASK DST_GPR(5) DST_CHAN.CHAN_X;
            
        ADD_INT:
            SRC0_SEL.GPR(5) SRC0_CHAN.CHAN_X LAST;
            SRC1_SEL.GPR(5) SRC0_CHAN.CHAN_Z;
            WRITE_MASK DST_GPR(5) DST_CHAN.CHAN_X;
            
        ADD_INT:
            SRC0_SEL.GPR(5) SRC0_CHAN.CHAN_X LAST;
            SRC1_SEL.GPR(5) SRC0_CHAN.CHAN_W;
            WRITE_MASK DST_GPR(5) DST_CHAN.CHAN_X;

        MOV:
            SRC0_SEL.GPR(1) SRC0_CHAN.CHAN_X LAST;
            WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_Z;
            
        MOV:
            SRC0_SEL.GPR(1) SRC0_CHAN.CHAN_Y LAST;
            WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_W;
            
/*
ALU: BARRIER;
//         MOV:
//                 SRC0_SEL.ALU_SRC_WAVE_ID_IN_GRP SRC0_CHAN.CHAN_X LAST;
//                 WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_X;

        MULLO_INT:
                SRC0_SEL.ALU_SRC_WAVE_ID_IN_GRP SRC0_CHAN.CHAN_X LAST;
		SRC1_SEL.ALU_SRC_LITERAL SRC1_CHAN.CHAN_X;
                WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_X;
		0x00000004;
		0x00000000;
		
*/
/*        MOV:
                SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_X LAST;
                WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_Y LAST;
		0x00000001;
		0x00000001;
        MOV:
                SRC0_SEL.ALU_SRC_1_INT SRC1_SEL.ALU_SRC_1_INT SRC0_CHAN.CHAN_Y SRC1_CHAN.CHAN_X LAST;
                WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_Z LAST;
        MOV:
                SRC0_SEL.ALU_SRC_1_INT SRC1_SEL.ALU_SRC_1_INT SRC0_CHAN.CHAN_Y SRC1_CHAN.CHAN_X LAST;
                WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_W LAST;*/
		/*
        MOV:
                SRC0_SEL.ALU_SRC_MASK_HI SRC0_CHAN.CHAN_X;
                WRITE_MASK DST_GPR(1) DST_CHAN.CHAN_X;
        MOV:
                SRC0_SEL.ALU_SRC_MASK_LO SRC0_CHAN.CHAN_X;
                WRITE_MASK DST_GPR(1) DST_CHAN.CHAN_Y;
        MOV:
                SRC0_SEL. ALU_SRC_HW_THREADGRP_ID SRC0_CHAN.CHAN_X;
                WRITE_MASK DST_GPR(1) DST_CHAN.CHAN_Z;
        MOV:
                SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_Y LAST;
                WRITE_MASK DST_GPR(1) DST_CHAN.CHAN_W;
		0xb000face;
		0x00000000;
		*/
/*MEM_RAT:
        RAT_ID(2) RAT_INST.EXPORT_RAT_INST_OR_RTN TYPE(1) RW_GPR(1) INDEX_GPR(0) ELEM_SIZE(1);
        COMP_MASK(1) BARRIER MARK;*/
/*MEM_RAT:
        RAT_ID(2) RAT_INST.EXPORT_RAT_INST_ADD_RTN RAT_INDEX_MODE.CF_INDEX_NONE TYPE(3) RW_GPR(1) INDEX_GPR(0);
        COMP_MASK(15) VALID_PIXEL_MODE MARK BARRIER;

WAIT_ACK: BARRIER;*/
/*
LOOP_START_DX10:    
       ADDR(@1);
       CF_CONST(0);
       BARRIER;

@2*/
// JUMP: ADDR(@3);

/*MEM_RAT_CACHELESS:
        RAT_ID(2) TYPE(3) RW_GPR(1) INDEX_GPR(0) ELEM_SIZE(0);
        COMP_MASK(7) BARRIER ARRAY_SIZE(0) MARK;*/
	
// WAIT_ACK: BARRIER;

/*ALU: BARRIER;
    ADD_INT:
	    SRC0_SEL.GPR(1) SRC0_CHAN.CHAN_W LAST;
	    SRC1_SEL.ALU_SRC_1_INT;
	    WRITE_MASK DST_GPR(1) DST_CHAN.CHAN_W;*/
    
// LOOP_BREAK: ADDR(@4);

NOP: BARRIER;

// @4
// LOOP_END:
//         ADDR(@2);
//         BARRIER;

@1
NOP: BARRIER;


/*MEM_RAT:
        RAT_ID(2) RAT_INST.EXPORT_RAT_INST_OR_RTN TYPE(1) RW_GPR(1) INDEX_GPR(0) ELEM_SIZE(0);
        COMP_MASK(1) BARRIER MARK;*/

// MEM_RAT:
//         RAT_ID(3) RAT_INST.EXPORT_RAT_INST_ADD_RTN TYPE(1) RW_GPR(1) INDEX_GPR(0);
//         COMP_MASK(15) VALID_PIXEL_MODE MARK BARRIER;
// 	
// WAIT_ACK: BARRIER;

MEM_RAT_CACHELESS:
        RAT_ID(2) TYPE(3) RW_GPR(0) INDEX_GPR(5) ELEM_SIZE(0);
        COMP_MASK(15) BARRIER ARRAY_SIZE(0) MARK;

NOP: END_OF_PROGRAM;

end;
