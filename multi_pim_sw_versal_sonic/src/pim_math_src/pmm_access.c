
#include "pim_math.h"

#include <time.h>
int pmm_access(pim_args *pim_args)
{
    
	uint32_t desc_idx, next_desc, p_loop_var, q_loop_var, r_loop_var, p, r;
	uint32_t code_idx, next_code;
	uint32_t p_size, r_size;
    uint64_t A_pa, B_pa, C_pa, offset;
    uint64_t A_base, B_base, C_base;
	uint64_t srcA_va, srcB_va, dstC_va;
	uint64_t srcA_pa, srcB_pa, dstC_pa;
    uint64_t* pmm_va;
    uint64_t  pmm_pa;
	uint64_t vecA_buf_va, vACC_buf_va;
	uint64_t vecA_buf_pa, vACC_buf_pa;


    int n_desc, n_code;

    uint32_t null_idx  = 0;
    uint32_t null_desc = 0;

    uint32_t opcode;
    opcode = OPCODE_ELE_ADD;
	srcA_va = pim_args->srcA_va;
    srcB_va = pim_args->srcB_va;
    dstC_va = pim_args->dstC_va;
	srcA_pa = VA2PA(srcA_va);
    srcB_pa = VA2PA(srcB_va);
    dstC_pa = VA2PA(dstC_va);
	
    vecA_buf_va = pim_args->vecA_buf;
    vACC_buf_va = pim_args->vACC_buf;
	vecA_buf_pa = VA2PA(vecA_buf_va);
    vACC_buf_pa = VA2PA(vACC_buf_va);
    printf("vecA_buf_va: %llx\n", vecA_buf_va);
    printf("vecA_buf_pa: %llx\n", vecA_buf_pa);

    p_size = pim_args->p_size;
    r_size = pim_args->r_size;
    p_loop_var = p_size;
    r_loop_var = (r_size * TYPE_SIZE) / (PIM_WIDTH * AIMX_SPIM_CH_NUM);
    // ============== Allocate PIM code space ==============
    pim_isa_32_t* code_list;
    uint64_t code_list_pa;
    n_code = (p_size * r_loop_var) * (AIMX_SPIM_CH_NUM + AIMX_SPIM_CH_NUM + AIMX_SPIM_CH_NUM) * 10;
    // n_code = (p_size * r_loop_var) * (AIMX_SPIM_CH_NUM + AIMX_SPIM_CH_NUM + AIMX_SPIM_CH_NUM);
	code_list = (pim_isa_32_t*)pim_malloc(sizeof(pim_isa_32_t) * n_code);
    code_list_pa = VA2PA((uint64_t)&code_list[0]);

    // ============== Allocate Desc space ============== 
    desc_idx = 0;
    code_idx = 0;
    uint64_t desc_base = pl_dma[0].desc_base;
    next_desc = desc_base  + 0x40; 

    // ============== Allocate Cl data space ==============
    cl_data_t* op_attr_data  = (cl_data_t*)pim_malloc(sizeof(cl_data_t));
    uint64_t op_attr_data_pa = VA2PA((uint64_t)&op_attr_data[0], 0);
     
    // ============== Load PMM base address ==============
    pmm_va = pim_args->pmm_base_addr;  
    pmm_pa = VA2PA(pmm_va);

    // ================ Generate PIM code ================
    A_base = 0x0ULL;
    B_base = 0x0ULL;
    C_base = 0x0ULL;
    PIM_MATH_LOG("%s: p:%d, r:%d\n", __func__, p_size, r_size);
    
    // for (p = 0; p < p_size; p++) {
    //     for(r = 0; r < r_size; r = r + (ALU_SIZE * NUM_BANKS * AIMX_SPIM_CH_NUM))
    //     // for(r = 0; r < r_size; r = r + (REG_SIZE * NUM_BANKS * AIMX_SPIM_CH_NUM))
    //     {
    //         offset = (p * r_size * TYPE_SIZE) + (r * TYPE_SIZE);
    //         if ((offset % HUGE_PAGE_SIZE) == 0) {
    //             srcA_pa = VA2PA(srcA_va + offset, 0);
    //             srcB_pa = VA2PA(srcB_va + offset, 0);
    //             dstC_pa = VA2PA(dstC_va + offset, 0);
    //             A_pa    = srcA_pa;
    //             B_pa    = srcB_pa;
    //             C_pa    = dstC_pa;
    //         } else {
    //             A_pa    = srcA_pa + (offset % HUGE_PAGE_SIZE);
    //             B_pa    = srcB_pa + (offset % HUGE_PAGE_SIZE);
    //             C_pa    = dstC_pa + (offset % HUGE_PAGE_SIZE);
    //         }
    //         RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR, 32 * AIMX_SPIM_CH_NUM, opcode, 0);
    //             for (int i = 0; i < AIMX_SPIM_CH_NUM; i++) ConfigPimRdInstr(&code_idx, A_pa, BRAM_DUMMY, PIM_WIDTH * AIMX_SPIM_CH_NUM, RD_A_ATTR | (opcode << 0x4), 0, code_list);
    //         PIM_RD_INSTR(&desc_idx, &next_desc, A_pa, BRAM_DUMMY, PIM_WIDTH * AIMX_SPIM_CH_NUM, opcode, 0);

    //         RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR, 32 * AIMX_SPIM_CH_NUM, opcode, 0);
    //             for (int i = 0; i < AIMX_SPIM_CH_NUM; i++) ConfigPimRdInstr(&code_idx, B_pa, BRAM_DUMMY, PIM_WIDTH * AIMX_SPIM_CH_NUM, RD_B_ATTR | (opcode << 0x4), 0, code_list);
    //         PIM_RD_INSTR(&desc_idx, &next_desc, B_pa, BRAM_DUMMY, PIM_WIDTH * AIMX_SPIM_CH_NUM, opcode, 0);

    //         RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR, 32 * AIMX_SPIM_CH_NUM, opcode, 0);
    //             for (int i = 0; i < AIMX_SPIM_CH_NUM; i++) ConfigPimWrInstr(&code_idx, BRAM_DUMMY, C_pa, PIM_WIDTH * AIMX_SPIM_CH_NUM, WR_C_ATTR | (opcode << 0x4), 0, code_list);
    //         PIM_WR_INSTR(&desc_idx, &next_desc, BRAM_DUMMY, C_pa, PIM_WIDTH * AIMX_SPIM_CH_NUM, opcode, 0);
    //     }
    // }
	A_pa = VA2PA(srcA_va);
    B_pa = VA2PA(srcB_va);
    // RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR,  32 * AIMX_SPIM_CH_NUM, opcode, 0);
    // for (int i = 0; i < AIMX_SPIM_CH_NUM; i++) ConfigPimWrInstr(&code_idx, A_pa, pmm_pa+VECA_OFFSET, 32*16* AIMX_SPIM_CH_NUM, RD_A_ATTR | (opcode << 0x4) | 0x1, 0, code_list);
    // WrPimReg(&desc_idx, &next_desc, A_pa, pmm_pa+VECA_OFFSET, 32*16* AIMX_SPIM_CH_NUM , opcode, 0);

    RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR,  32 * AIMX_SPIM_CH_NUM, opcode, 0);
    for (int i = 0; i < AIMX_SPIM_CH_NUM; i++) ConfigPimWrInstr(&code_idx, A_pa, pmm_pa+VACC_OFFSET, 32*16* AIMX_SPIM_CH_NUM*8, WR_C_ATTR | (opcode << 0x4) | 0x1, 0, code_list);
    WrPimReg(&desc_idx, &next_desc, A_pa, pmm_pa+VACC_OFFSET, 32*16* AIMX_SPIM_CH_NUM*8 , opcode, 0);

    // RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR,  32 * AIMX_SPIM_CH_NUM, opcode, 0);
    // for (int i = 0; i < AIMX_SPIM_CH_NUM; i++) ConfigPimWrInstr(&code_idx, A_pa, pmm_pa+VECA_OFFSET, 32*16* AIMX_SPIM_CH_NUM, RD_A_ATTR | (opcode << 0x4) | 0x1, 0, code_list);
    // WrPimReg(&desc_idx, &next_desc, B_pa, pmm_pa+VECA_OFFSET, 32*16* AIMX_SPIM_CH_NUM , opcode, 0);

    // RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR,  32 * AIMX_SPIM_CH_NUM, opcode, 0);
    // for (int i = 0; i < AIMX_SPIM_CH_NUM; i++) ConfigPimRdInstr(&code_idx, pmm_pa+VECA_OFFSET, vecA_buf_pa, 32*16* AIMX_SPIM_CH_NUM, RD_A_ATTR | (opcode << 0x4), 0, code_list);
    // RdPimReg(&desc_idx, &next_desc, pmm_pa+VECA_OFFSET, vecA_buf_pa, 32*16* AIMX_SPIM_CH_NUM, opcode, 0);

    RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR,  32 * AIMX_SPIM_CH_NUM, opcode, 0);
    for (int i = 0; i < AIMX_SPIM_CH_NUM; i++) ConfigPimRdInstr(&code_idx, pmm_pa+VACC_OFFSET, vecA_buf_pa, 32*16* AIMX_SPIM_CH_NUM*8, WR_C_ATTR | (opcode << 0x4), 0, code_list);
    RdPimReg(&desc_idx, &next_desc, pmm_pa+VACC_OFFSET, vecA_buf_pa, 32*16* AIMX_SPIM_CH_NUM*8, opcode, 0);


    pim_args->desc_idx = desc_idx - 1;
    pim_args->desc_last = next_desc - 0x40;       
    if (pim_exec(pim_args, 0) < 0) {
        printf("DMA transaction failed \n");
        goto free;
    }
free:
    pim_free(code_list);
    pim_free(op_attr_data);
    return 0;
}
