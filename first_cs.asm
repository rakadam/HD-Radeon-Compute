
// NOP: BARRIER;

ALU: BARRIER;
        MULLO_INT:
                SRC0_SEL.GPR(0) SRC0_CHAN.CHAN_X LAST;
                SRC1_SEL.ALU_SRC_LITERAL SRC1_CHAN.CHAN_X;
                WRITE_MASK DST_GPR(4) DST_CHAN.CHAN_X;
        0x00000004;
        0x00000000;
                
        MULLO_INT:
                SRC0_SEL.GPR(0) SRC0_CHAN.CHAN_Y LAST;
                SRC1_SEL.ALU_SRC_LITERAL SRC1_CHAN.CHAN_X;
                WRITE_MASK DST_GPR(4) DST_CHAN.CHAN_Y;
        0x00000020;
        0x00000000;
        
        MULLO_INT:
                SRC0_SEL.GPR(1) SRC0_CHAN.CHAN_X LAST;
                SRC1_SEL.ALU_SRC_LITERAL SRC1_CHAN.CHAN_X;
                WRITE_MASK DST_GPR(4) DST_CHAN.CHAN_Z;
        0x00000100;
        0x00000000;
        
        MULLO_INT:
                SRC0_SEL.GPR(1) SRC0_CHAN.CHAN_Y LAST;
                SRC1_SEL.ALU_SRC_LITERAL SRC1_CHAN.CHAN_X;
                WRITE_MASK DST_GPR(4) DST_CHAN.CHAN_W;
        0x00000400;
        0x00000000;
        
        ADD_INT:
            SRC0_SEL.GPR(4) SRC0_CHAN.CHAN_X LAST;
            SRC1_SEL.GPR(4) SRC1_CHAN.CHAN_Y;
            DST_CHAN.CHAN_X;
//             WRITE_MASK DST_GPR(5) DST_CHAN.CHAN_X;

        ADD_INT:
            SRC0_SEL.ALU_SRC_PV SRC0_CHAN.CHAN_X LAST;
            SRC1_SEL.GPR(4) SRC1_CHAN.CHAN_Z;
            DST_CHAN.CHAN_X;
//             WRITE_MASK DST_GPR(5) DST_CHAN.CHAN_X;


/*        MOV:
            SRC0_SEL.ALU_SRC_PV SRC0_CHAN.CHAN_X LAST;
            WRITE_MASK DST_GPR(3) DST_CHAN.CHAN_X;*/
//             
        ADD_INT:
            SRC0_SEL.ALU_SRC_PV SRC0_CHAN.CHAN_X LAST;
            SRC1_SEL.GPR(4) SRC1_CHAN.CHAN_W;
            WRITE_MASK DST_GPR(5) DST_CHAN.CHAN_X;

        MOV:
            SRC0_SEL.GPR(1) SRC0_CHAN.CHAN_X LAST;
            WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_Z;
            
        MOV:
            SRC0_SEL.GPR(1) SRC0_CHAN.CHAN_Y LAST;
            WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_W;

//         ADD_INT:
//             SRC0_SEL.ALU_SRC_1_INT SRC0_CHAN.CHAN_X LAST;
//             SRC1_SEL.GPR(0) SRC1_CHAN.CHAN_X;
//             WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_X;

        ADD_INT:
            SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_X;
            SRC1_SEL.GPR(5) SRC1_CHAN.CHAN_X;
            WRITE_MASK DST_GPR(6) DST_CHAN.CHAN_X;
        ADD_INT:
            SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_Y LAST;
            SRC1_SEL.GPR(5) SRC1_CHAN.CHAN_X;
            WRITE_MASK DST_GPR(7) DST_CHAN.CHAN_X;
            0x1;
            0x2;
        ADD_INT:
            SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_X;
            SRC1_SEL.GPR(5) SRC1_CHAN.CHAN_X;
            WRITE_MASK DST_GPR(8) DST_CHAN.CHAN_X;
        ADD_INT:
            SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_Y LAST;
            SRC1_SEL.GPR(5) SRC1_CHAN.CHAN_X;
            WRITE_MASK DST_GPR(9) DST_CHAN.CHAN_X;
            0x7;
            0xB;

        MOV:
            SRC0_SEL.ALU_SRC_LITERAL SRC0_CHAN.CHAN_X LAST;
            WRITE_MASK DST_GPR(1) DST_CHAN.CHAN_X;
            0x0;
            0x0;

         MOV:
                 SRC0_SEL.ALU_SRC_TIME_LO SRC0_CHAN.CHAN_X LAST;
                 WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_X;

         MOV:
                 SRC0_SEL.ALU_SRC_SE_ID SRC0_CHAN.CHAN_X LAST;
                 WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_Z;
         MOV:
                 SRC0_SEL.ALU_SRC_SIMD_ID SRC0_CHAN.CHAN_X LAST;
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

LOOP_START_DX10:
       ADDR(@1);
       CF_CONST(0);
       BARRIER;
@2
//NOP: BARRIER;
//NOP: BARRIER;

LOOP_END:
         ADDR(@2);
         BARRIER;

@1
NOP: BARRIER;


ALU: BARRIER;
         MOV:
                 SRC0_SEL.ALU_SRC_TIME_LO SRC0_CHAN.CHAN_X LAST;
                 WRITE_MASK DST_GPR(0) DST_CHAN.CHAN_Y;


//@2
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

NOP: BARRIER;

/*MEM_RAT:
        RAT_ID(2) RAT_INST.EXPORT_RAT_INST_OR_RTN TYPE(1) RW_GPR(1) INDEX_GPR(0) ELEM_SIZE(0);
        COMP_MASK(1) BARRIER MARK;*/

/*
MEM_RAT:
        RAT_ID(2) TYPE(1) RAT_INST.EXPORT_RAT_INST_ADD RW_GPR(0) INDEX_GPR(5) ELEM_SIZE(0);
        COMP_MASK(15) BARRIER ARRAY_SIZE(0) MARK;

WAIT_ACK: BARRIER;
*/
/*MEM_RAT:
        RAT_ID(11) RAT_INST.EXPORT_RAT_INST_STORE_RAW TYPE(1) INDEX_GPR(5) RW_GPR(0);
        COMP_MASK(1) BARRIER MARK;*/
        
/*MEM_RAT:
        RAT_ID(11) RAT_INST.EXPORT_RAT_INST_STORE_RAW TYPE(1) INDEX_GPR(6) RW_GPR(0);
        COMP_MASK(15) BARRIER MARK;*/

/*TC:
        BARRIER;

        FETCH:
                FETCH_TYPE.VTX_FETCH_NO_INDEX_OFFSET BUFFER_ID(0) SRC_GPR(5) SRC_SEL_X.SEL_X MEGA_FETCH_COUNT(15);
                DST_GPR(0) DST_SEL_X.SEL_X DST_SEL_Y.SEL_Y DST_SEL_Z.SEL_Z DST_SEL_W.SEL_W USE_CONST_FIELDS;
                MEGA_FETCH;
*/
MEM_RAT_CACHELESS:
        RAT_ID(11) RAT_INST.EXPORT_RAT_INST_STORE_RAW TYPE(1) RW_GPR(0) INDEX_GPR(5) ELEM_SIZE(0);
        COMP_MASK(15) BARRIER ARRAY_SIZE(0);

NOP: END_OF_PROGRAM;

end;
