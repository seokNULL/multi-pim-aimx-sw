
#include "pim_math.h"

#include <time.h>
// #define CUSTOM
// #define pdmm_vec_test
int _elewise(pim_args *pim_args, uint32_t opcode, int fpga_id);
int _bias_op(pim_args *pim_args, uint32_t opcode, int fpga_id);
/* Wrapper for code reuse */
int _elewise_add(struct id_pim_args in)
{
    int fpga_id_out = in.fpga_id? in.fpga_id : 0;
    return _elewise(in.pim_args, OPCODE_ELE_ADD, fpga_id_out);
}
int _elewise_sub(struct id_pim_args in)
{
    int fpga_id_out = in.fpga_id? in.fpga_id : 0;
    return _elewise(in.pim_args, OPCODE_ELE_SUB, fpga_id_out);
}
int _elewise_mul(struct id_pim_args in)
{
    int fpga_id_out = in.fpga_id? in.fpga_id : 0;
    return _elewise(in.pim_args, OPCODE_ELE_MUL, fpga_id_out);
}
int _bias_add(struct id_pim_args in)
{
    int fpga_id_out = in.fpga_id? in.fpga_id : 0;
    return _bias_op(in.pim_args, OPCODE_ELE_ADD, fpga_id_out);
}
int _bias_sub(struct id_pim_args in)
{
    int fpga_id_out = in.fpga_id? in.fpga_id : 0;
    return _bias_op(in.pim_args, OPCODE_ELE_SUB, fpga_id_out);
}
int _bias_mul(struct id_pim_args in)
{
    int fpga_id_out = in.fpga_id? in.fpga_id : 0;
    return _bias_op(in.pim_args, OPCODE_ELE_MUL, fpga_id_out);
}
int _elewise(pim_args *pim_args, uint32_t opcode, int fpga_id)
{

// =========== without CL =========== 
	uint32_t desc_idx, next_desc, p_loop_var, q_loop_var, r_loop_var, p, r;
	uint32_t code_idx, next_code;
	uint32_t p_size, r_size;
    uint64_t A_pa, B_pa, C_pa, offset;
    uint64_t A_base, B_base, C_base;
	uint64_t srcA_va, srcB_va, dstC_va;
	uint64_t srcA_pa, srcB_pa, dstC_pa;
    uint64_t* pmm_va;
    uint64_t  pmm_pa;

    pim_isa_t* desc_list;
    uint64_t desc_list_pa;
    int n_desc, n_code;
    
    uint32_t null_idx  = 0;
    uint32_t null_desc = 0;

	srcA_va = pim_args->srcA_va;
    srcB_va = pim_args->srcB_va;
    dstC_va = pim_args->dstC_va;

	srcA_pa = VA2PA(srcA_va);
    srcB_pa = VA2PA(srcB_va);
    dstC_pa = VA2PA(dstC_va);

    p_size = pim_args->p_size;
    r_size = pim_args->r_size;
    printf("AIMX_SPIM_CH_NUM: %d\n", AIMX_SPIM_CH_NUM);
    p_loop_var = p_size;
    r_loop_var = (r_size * TYPE_SIZE) / (PIM_WIDTH * AIMX_SPIM_CH_NUM);
    printf("opcode: %x\n", opcode);
    // ============== Allocate PIM code space ==============
    pim_isa_32_t* code_list;
    uint64_t code_list_pa;
    n_code = (p_size * r_loop_var) * (AIMX_SPIM_CH_NUM + AIMX_SPIM_CH_NUM + AIMX_SPIM_CH_NUM);
	code_list = (pim_isa_32_t*)pim_malloc(sizeof(pim_isa_32_t) * n_code);
    code_list_pa = VA2PA((uint64_t)&code_list[0]);
    printf("code_list_pa: %x\n", code_list_pa);

    // ============== Allocate Desc space ============== 
    n_desc = (p_size * r_loop_var) * (2 + 2 + 2);
	desc_list = (pim_isa_t*)pim_malloc(sizeof(pim_isa_t) * n_desc);
    // desc_list_pa = VA2PA((uint64_t)&desc_list[0]);
    desc_idx = 0;
    code_idx = 0;
    // next_desc = desc_list_pa + 0x40; 
    // pim_args->desc_base = (uint64_t)&desc_list[0];


    uint64_t desc_base = pl_dma[fpga_id].desc_base;
    printf("desc_base: %x\n", desc_base);
    next_desc = desc_base  + 0x40; 
    // // ============== Allocate Cl data space ==============
    // cl_data_t* op_attr_data  = (cl_data_t*)pim_malloc(sizeof(cl_data_t));
    // uint64_t op_attr_data_pa = VA2PA((uint64_t)&op_attr_data[0], fpga_id);
    
    // ============== Load PMM base address ==============
    pmm_va = pim_args->pmm_base_addr;  
    pmm_pa = VA2PA(pmm_va);

    // ================ Generate PIM code ================
    A_base = 0x0ULL;
    B_base = 0x0ULL;
    C_base = 0x0ULL;
    PIM_MATH_LOG("%s: p:%d, r:%d\n", __func__, p_size, r_size);
    size_t iter, batch_chunk, total_size, remain_size, opsize, last_iter_size, batch_size;
    total_size  = p_size*r_size*TYPE_SIZE;
    batch_size  = (p_size < MAX_BATCH) ? p_size : MAX_BATCH;
    batch_chunk = (total_size < 32*16*CH_NUM*MAX_BATCH)? total_size : 32*16*CH_NUM*MAX_BATCH;
    iter        = total_size/batch_chunk;
    // remain_size = (total_size % batch_chunk);
    // iter  = (total_size < batch_chunk)? 1 : total_size/batch_chunk;
    // iter  = (remain_size == 0)? iter : iter + 1;
    srcA_pa = VA2PA(srcA_va, fpga_id);
    srcB_pa = VA2PA(srcB_va, fpga_id);
    dstC_pa = VA2PA(dstC_va, fpga_id);
    uint64_t srcA_reg_offset, srcA_attr;
    srcA_reg_offset = (opcode == OPCODE_ELE_MUL)? VECA_OFFSET : VACC_OFFSET;
    srcA_attr       = (opcode == OPCODE_ELE_MUL)? WR_A_ATTR : WR_C_ATTR;
#ifdef QDMA
    cl_data_t* op_attr_data  = (cl_data_t*)malloc(sizeof(cl_data_t));
    cl_data_t* op_attr_data_pim  = (cl_data_t*)pim_malloc(sizeof(cl_data_t), fpga_id);
    // ConfigOpAttr(&desc_idx, &next_desc, op_attr_data, op_attr_data_pim, pmm_pa+VECA_OFFSET, srcB_pa, 0x0, batch_col, batch_size, p_size, q_loop_size, r_loop_size, opcode, fpga_id);
    ConfigOpAttr(&desc_idx, &next_desc, op_attr_data, op_attr_data_pim, 0x0, srcB_pa, 0x0, iter, batch_chunk, p_size, r_size, r_size, opcode, fpga_id); // iter naming in cl config
    // cl_data_t* op_attr_data  = (cl_data_t*)pim_malloc(sizeof(cl_data_t));
#else
    cl_data_t* op_attr_data  = (cl_data_t*)pim_malloc(sizeof(cl_data_t));
    ConfigOpAttr(&desc_idx, &next_desc, op_attr_data, 0x0, srcB_pa, 0x0, iter, batch_chunk, p_size, r_size, r_size, opcode, fpga_id); // iter naming in cl config
#endif
    printf("iter: %d\n",iter);
    for (size_t i=0; i<iter; i++) {
        A_pa = srcA_pa+i*batch_chunk;
        B_pa = srcB_pa+i*batch_chunk;
        C_pa = dstC_pa+i*batch_chunk;

        RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR,  32 * AIMX_SPIM_CH_NUM, opcode, 0); // vACC WR
        for (int i = 0; i < AIMX_SPIM_CH_NUM; i++) ConfigPimWrInstr(&code_idx, A_pa, pmm_pa+srcA_reg_offset, batch_chunk, BATCH8 | srcA_attr | (opcode << 0x4), 0, code_list);
        WrPimReg(&desc_idx, &next_desc, A_pa, pmm_pa+srcA_reg_offset, batch_chunk, opcode, 0);

        RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR, 32 * AIMX_SPIM_CH_NUM, opcode, fpga_id);
        for (int i = 0; i < AIMX_SPIM_CH_NUM; i++) ConfigPimRdInstr(&code_idx, B_pa, 0xd0000000, batch_chunk, RD_B_ATTR | (opcode << 0x4), fpga_id, code_list);
        PIM_RD_INSTR(&desc_idx, &next_desc, B_pa, 0xd0000000, 32*CH_NUM, opcode, fpga_id);
        // PIM_RD_INSTR(&desc_idx, &next_desc, B_pa, 0xd0000000, batch_chunk, opcode, fpga_id);

        RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR,  32 * AIMX_SPIM_CH_NUM, opcode, 0); // vACC RD
        for (int i = 0; i < AIMX_SPIM_CH_NUM; i++) ConfigPimRdInstr(&code_idx, pmm_pa+VACC_OFFSET, C_pa, batch_chunk, RD_C_ATTR | (opcode << 0x4), 0, code_list);
        RdPimReg(&desc_idx, &next_desc, pmm_pa+VACC_OFFSET, C_pa, batch_chunk, opcode, 0);
    }

    pim_args->desc_idx = desc_idx - 1;
    pim_args->desc_last = next_desc - 0x40;       
    if (pim_exec(pim_args, fpga_id) < 0) {
        printf("DMA transaction failed \n");
        goto free;
    }
free:
    pim_free(code_list);
    pim_free(desc_list);
    pim_free(op_attr_data);
    return 0;
}

int _bias_op(pim_args *pim_args, uint32_t opcode, int fpga_id)
{
    uint32_t desc_idx, next_desc, r_loop_var, q_loop_var, q, r;
    uint32_t q_size, r_size;
    uint32_t A_pa, B_pa, C_pa, offset, A_off;
    uint64_t A_base, B_base, C_base;
    uint64_t srcA_va, srcB_va, dstC_va;

    srcA_va = pim_args->srcA_va;
    srcB_va = pim_args->srcB_va;
    dstC_va = pim_args->dstC_va;

    q_size = pim_args->q_size;
    r_size = pim_args->r_size;

    desc_idx = 0;
    uint64_t desc_base = pl_dma[fpga_id].desc_base;
    next_desc = desc_base + 0x40;

    q_loop_var = q_size;
    r_loop_var = r_size;

    A_base = 0x0ULL;
    B_base = 0x0ULL;
    C_base = 0x0ULL;

    PIM_MATH_LOG("%s: q:%d, r:%d\n", __func__, q_size, r_size);
    for (r = 0; r < r_loop_var; r = r + (REG_SIZE * NUM_BANKS)){
        for (q = 0; q < q_loop_var; q++){
            PIM_MATH_LOG("[q:%d, r:%d] \n", q, r);
            A_off = (r * TYPE_SIZE);
            if ((A_off % HUGE_PAGE_SIZE) == 0) {
                A_base = VA2PA(srcA_va + A_off, fpga_id);
                A_pa = A_base;
            } else {
                A_pa = A_base + A_off;
            }
            offset = (q * r_size * TYPE_SIZE) + (r * TYPE_SIZE);
            if ((offset % HUGE_PAGE_SIZE) == 0) {
                B_base = VA2PA(srcB_va + offset, fpga_id);
                C_base = VA2PA(dstC_va + offset, fpga_id);
                B_pa = B_base;
                C_pa = C_base;
            } else {
                B_pa = B_base + offset;
                C_pa = C_base + offset;
            }
            PIM_RD_INSTR(&desc_idx, &next_desc, A_pa, BRAM_DUMMY, PIM_WIDTH, RD_A_ATTR | (opcode << 0x4), fpga_id);
            PIM_RD_INSTR(&desc_idx, &next_desc, B_pa, BRAM_DUMMY, PIM_WIDTH, RD_B_ATTR | (opcode << 0x4), fpga_id);
            PIM_WR_INSTR(&desc_idx, &next_desc, BRAM_DUMMY, C_pa, PIM_WIDTH, WR_C_ATTR | (opcode << 0x4), fpga_id);        
        }
    }
    pim_args->desc_idx = desc_idx - 1;
    pim_args->desc_last = next_desc - 0x40;
    if (pim_exec(pim_args, fpga_id) < 0) {
        printf("DMA transaction failed \n");
        return -1;
    }
    return 0;
}