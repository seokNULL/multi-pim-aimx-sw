#include "pim_math.h"
#include "../drv_src/multi_dev_lib_user.h"
#include <time.h>
#include <pthread.h>

int _matmul_silent(pim_args *pim_args, int fpga_id);
int _matmul_fusion_silent(pim_args *pim_args, int fpga_id);
int _gemm_silent(pim_args *pim_args, int fpga_id);
int _matmul_decoupled(pim_args *pim_args, int fpga_id);
int _matmul_fusion_decoupled(pim_args *pim_args, int fpga_id);
int _gemm_decoupled(pim_args *pim_args, int fpga_id);

/* Wrapper for silent/decoupled PIM distinction */
int _matmul(struct id_pim_args in) {
    int fpga_id_out = in.fpga_id? in.fpga_id : 0;
    return _matmul_silent(in.pim_args, fpga_id_out);
}

int _matmul_tiled(struct id_pim_args in) {
    int fpga_id_out = in.fpga_id? in.fpga_id : 0;
    return _matmul_decoupled(in.pim_args, fpga_id_out);
}

int _gemm(struct id_pim_args in) {
    int fpga_id_out = in.fpga_id? in.fpga_id : 0;
    // if (in.pim_args->p_size > (BURST_SIZE/TYPE_SIZE))
        return _gemm_silent(in.pim_args, fpga_id_out);
    // else
    //     return _gemm_decoupled(in.pim_args);
}
int _matmul_fusion(struct id_pim_args in) {
    int fpga_id_out = in.fpga_id? in.fpga_id : 0;
    if (in.pim_args->p_size > (BURST_SIZE/TYPE_SIZE))
        return _matmul_fusion_silent(in.pim_args, fpga_id_out);
    else
        return _matmul_fusion_decoupled(in.pim_args, fpga_id_out);
}

int _matmul_silent(pim_args *pim_args, int fpga_id)
{
    uint32_t desc_idx, code_idx, next_desc, p, q, r;
    uint32_t q_loop_var, r_loop_var;
    uint32_t p_size, q_size, r_size;
    uint32_t A_off, B_off, C_off, batch_off;
    uint64_t A_pa, B_pa, C_pa;
    uint64_t A_base, B_base, C_base;        
    uint64_t srcA_va, srcB_va, dstC_va;
	uint64_t srcA_pa, srcB_pa, dstC_pa;
    uint32_t A_len, B_len, C_len;
    uint32_t opcode;
    uint64_t* pmm_va;
    uint64_t  pmm_pa;

    // pim_isa_t* desc_list;
    // uint64_t desc_list_pa;
    int n_desc, n_code;
    int op_attr_idx;

    A_len = BURST_SIZE * VECA_SIZE;
    B_len = REG_SIZE * PIM_WIDTH;
    C_len = PIM_WIDTH;
    srcA_va = pim_args->srcA_va;
    srcB_va = pim_args->srcB_va;
    dstC_va = pim_args->dstC_va;

    p_size = pim_args->p_size;
    q_size = pim_args->q_size;
    r_size = pim_args->r_size;

    r_loop_var = (r_size * TYPE_SIZE) / (PIM_WIDTH * AIMX_SPIM_CH_NUM);
    q_loop_var = (q_size) / REG_SIZE;
  
    // ============== Allocate PIM code space ==============
    // ============== Load PMM base address ==============
    // ============== Allocate Desc space ============== 
    uint64_t desc_base = pl_dma[fpga_id].desc_base;
    desc_idx = 0;
    next_desc = desc_base  + 0x40; 

    // ============== Allocate Cl data space ==============
    // ================ Generate PIM code ================
    opcode = OPCODE_MAT_MUL;
    PIM_MATH_LOG("%s: p:%d, r:%d\n", __func__, p_size, r_size);
    srcA_pa = VA2PA(srcA_va, fpga_id);
    srcB_pa = VA2PA(srcB_va, fpga_id);
    dstC_pa = VA2PA(dstC_va, fpga_id);
    A_pa = srcA_pa;
    B_pa = srcB_pa;
    C_pa = dstC_pa;

    uint64_t src_reg_pa, dst_reg_pa;
    uint32_t A_batch_attr, B_batch_attr;
    uint64_t vecA_pa, vACC_pa;
    uint32_t small_batch;
    uint32_t p_loop_size, q_loop_size, r_loop_size, batch_size, batch_col, col_chunk, vACC_stride;
    batch_size  = (p_size < MAX_BATCH) ? p_size : MAX_BATCH;
    vACC_stride = (MAX_BATCH/batch_size);
    col_chunk = (MAX_BATCH/batch_size)*REG_CHUNK*CH_NUM;
    batch_col = (r_size<col_chunk)? r_size/(REG_CHUNK*CH_NUM) : MAX_BATCH/batch_size;
    p_loop_size = (p_size < MAX_BATCH) ? 1 : p_size/MAX_BATCH;   // 멀티 채널 적용시 변경 필요
    q_loop_size = q_size/REG_CHUNK;     // 멀티 채널 적용시 변경 필요
    r_loop_size = r_size/(REG_CHUNK*CH_NUM*batch_col);     // 배치가 8 미만일 때 변경 필요

    switch (batch_size) {
        case 1:
            A_batch_attr = BATCH1;
            break;
        case 2:
            A_batch_attr = BATCH2;
            break;
        case 4:
            A_batch_attr = BATCH4;
            break;
        case 8:
            A_batch_attr = BATCH8;
            break;
    }
    switch (batch_col) {
        case 1:
            B_batch_attr = BATCH8;
            break;
        case 2:
            B_batch_attr = BATCH4;
            break;
        case 4:
            B_batch_attr = BATCH2;
            break;
        case 8:
            B_batch_attr = BATCH1;
            break;
    }

    uint64_t batch_A_pa, bank_A_pa, bank_reg_pa, batch_C_pa;
    for (p = 0; p < p_loop_size; p++) {
        for (r = 0; r < r_loop_size; r++) {
            for(q = 0; q < q_loop_size; q++) {
                A_off   = MAX_BATCH*q_size*TYPE_SIZE*p + REG_CHUNK*TYPE_SIZE*q;
                A_pa    = srcA_pa + A_off;
                vecA_pa = pmm_pa+VECA_OFFSET;

                B_off = (batch_col*REG_CHUNK*CH_NUM*q_size*r + batch_col*REG_CHUNK*REG_CHUNK*CH_NUM*q)*TYPE_SIZE;   
                B_pa  = srcB_pa + B_off;
                B_len = REG_CHUNK*REG_CHUNK*batch_col*CH_NUM*TYPE_SIZE; // if batch<8, B can be read batch_size times more

                for (int i = 0; i < batch_size; i++) {
                    batch_off  = q_size*TYPE_SIZE*i;
                    batch_A_pa = A_pa + batch_off;
                    dst_reg_pa = vecA_pa + CH_NUM*REG_CHUNK*TYPE_SIZE*i;    // for broadcasting, access different bank in same channel
                    RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR,  32 *CH_NUM, opcode, 0); // vecA WR
                    for (int i = 0; i < CH_NUM; i++) ConfigPimWrInstr(&code_idx, batch_A_pa, dst_reg_pa, REG_CHUNK*TYPE_SIZE*CH_NUM, A_batch_attr | WR_A_ATTR | (opcode << 0x4), 0, code_list);
                    for (int i = 0; i < NUM_BANKS; i++) {
                        bank_A_pa   = batch_A_pa + BURST_SIZE*i;
                        bank_reg_pa = dst_reg_pa + BURST_SIZE*i*CH_NUM;
                        WrPimReg(&desc_idx, &next_desc, bank_A_pa, bank_reg_pa, BURST_SIZE, opcode, 0);
                    }
                }

                RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR, 32*CH_NUM, opcode, fpga_id);
                for (int i = 0; i < CH_NUM; i++) ConfigPimRdInstr(&code_idx, B_pa, 0xd0000000, B_len, B_batch_attr | RD_B_ATTR | (opcode << 0x4), fpga_id, code_list); 
                // PIM_RD_INSTR(&desc_idx, &next_desc, B_pa, 0xd0000000, B_len, opcode, fpga_id);
                PIM_RD_INSTR(&desc_idx, &next_desc, B_pa, 0xd0000000, 32*CH_NUM, opcode, fpga_id);
            }
            C_off   = MAX_BATCH*r_size*TYPE_SIZE*p + REG_CHUNK*TYPE_SIZE*CH_NUM*batch_col*r;
            C_pa    = dstC_pa + C_off;
            vACC_pa = pmm_pa+VACC_OFFSET;
            
            for (int i = 0; i < batch_size; i++) {
                batch_off  = r_size*TYPE_SIZE*i;
                batch_C_pa = C_pa + batch_off;
                src_reg_pa = vACC_pa + REG_CHUNK*TYPE_SIZE*CH_NUM*vACC_stride*i;
                // src_reg_pa = vACC_pa + REG_CHUNK*TYPE_SIZE*CH_NUM*batch_col*i;
                RdPimCode(&desc_idx, &next_desc, code_list_pa+code_idx*0x20, CODE_WR_ADDR,  32 *CH_NUM, opcode, 0);
                for (int i = 0; i < CH_NUM; i++) ConfigPimRdInstr(&code_idx, src_reg_pa, batch_C_pa, REG_CHUNK*TYPE_SIZE*CH_NUM*batch_col, RD_C_ATTR | (opcode << 0x4), 0, code_list);
                RdPimReg(&desc_idx, &next_desc, src_reg_pa, batch_C_pa, REG_CHUNK*TYPE_SIZE*CH_NUM*batch_col, opcode, 0);
            }
        }
    }

    pim_args->desc_idx = desc_idx - 1;
    pim_args->desc_last = next_desc - 0x40;

    // struct timespec start, end;
    // long elapsed_time;
    // clock_gettime(CLOCK_MONOTONIC, &start);

    if (pim_exec(pim_args, fpga_id) < 0) {
        printf("DMA transaction failed \n");
    }
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // elapsed_time = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_nsec - start.tv_nsec) / 1000L;
    // printf("pim_exec 실행 시간: %ld us\n", elapsed_time);
    return 0;

}


int _gemm_silent(pim_args *pim_args, int fpga_id)
{
    uint32_t desc_idx, next_desc, r_loop_var, p, q, r;
    uint32_t p_size, q_size, r_size;
    uint64_t src0_pa, src1_pa, src2_pa, dst_pa;
    uint64_t src0_va, src1_va, src2_va, dst_va;
    uint32_t src0_base, src1_base, src2_base, dst_base, src0_off, src1_off, offset;
    uint32_t A_len, B_len, C_len;

    A_len = BURST_SIZE;
    B_len = REG_SIZE * PIM_WIDTH;
    C_len = PIM_WIDTH;

    src0_va = pim_args->srcA_va;
    src1_va = pim_args->srcB_va;
    src2_va = pim_args->bias_va;
    dst_va = pim_args->dstC_va;

    p_size = pim_args->p_size;
    q_size = pim_args->q_size;
    r_size = pim_args->r_size;

    r_loop_var = (r_size * TYPE_SIZE) / PIM_WIDTH;

    q = 0;
    desc_idx = 0;
    uint64_t desc_base = pl_dma[fpga_id].desc_base;
    next_desc = desc_base + 0x40;

    src0_base = 0x0ULL;
    src1_base = 0x0ULL;
    src2_base = 0x0ULL;
    dst_base = 0x0ULL;
    PIM_MATH_LOG("%s: p:%d, q:%d, r:%d\n", __func__, p_size, q_size, r_size);
    for (p = 0; p < p_size; p++) {
        for (r = 0; r < r_loop_var; r++) {
            PIM_MATH_LOG("[p:%d, q:%d, r:%d] \n", p, q, r);
            /* 
             * Operator fusion (C + (A X B) = D)
             * C is first stored in vACC before operations and then accumulated.
             */
            offset = (r * PIM_WIDTH) + (p * r_size * TYPE_SIZE);
            if ((offset % HUGE_PAGE_SIZE) == 0) {
                src2_base = VA2PA(src2_va + offset, fpga_id);
                dst_base = VA2PA(dst_va + offset, fpga_id);
                src2_pa = src2_base;
                dst_pa = dst_base;
            } else {
                src2_pa = src2_base + offset;
                dst_pa = dst_base + offset;
            }
            PIM_RD_INSTR(&desc_idx, &next_desc, src2_pa, BRAM_DUMMY + (offset % 0x40), C_len, RD_B_ATTR | (OPCODE_ELE_ADD << 0x4), fpga_id);

            for (q = 0; q < q_size; q = q + REG_SIZE) {
                src0_off = (q * TYPE_SIZE) + ((q_size * TYPE_SIZE) * p);
                if ((src0_off % HUGE_PAGE_SIZE) == 0) {
                    src0_base = VA2PA(src0_va + src0_off, fpga_id);
                    src0_pa = src0_base;
                } else {
                    // src0_pa = src0_base + src0_off;
                    src0_pa = src0_base + (src0_off % HUGE_PAGE_SIZE);
                }
                src1_off = (q_size * PIM_WIDTH * r) + (q * PIM_WIDTH);
                if ((src1_off % HUGE_PAGE_SIZE) == 0) {
                    src1_base = VA2PA(src1_va + src1_off, fpga_id);
                    src1_pa = src1_base;
                } else {
                    // src1_pa = src1_base + src1_off;
                    src1_pa = src1_base + (src1_off % HUGE_PAGE_SIZE);
                }
                PIM_RD_INSTR(&desc_idx, &next_desc, src0_pa, BRAM_DUMMY + (src0_off % 0x40), A_len, RD_A_ATTR | (OPCODE_MAT_MUL << 0x4), fpga_id);
                PIM_RD_INSTR(&desc_idx, &next_desc, src1_pa, BRAM_DUMMY + (src1_off % 0x40), B_len, RD_B_ATTR | (OPCODE_MAT_MUL << 0x4), fpga_id);
            }
            PIM_WR_INSTR(&desc_idx, &next_desc, BRAM_DUMMY + (offset % 0x40), dst_pa, C_len, WR_C_ATTR | (OPCODE_MAT_MUL << 0x4), fpga_id);
        }
    }
    pim_args->desc_idx = desc_idx - 1;
    pim_args->desc_last = next_desc - 0x40;
    if (pim_exec(pim_args, fpga_id) < 0) {
        return -1;
    }
    return 0;
}

int _matmul_fusion_silent(pim_args *pim_args, int fpga_id)
{
    uint32_t desc_idx, next_desc, r_loop_var, p, q, r, fused_op;
    uint32_t p_size, q_size, r_size;
    uint64_t src0_pa, src1_pa, src2_pa, dst_pa;
    uint64_t src0_va, src1_va, src2_va, dst_va;
    uint32_t src0_base, src1_base, src2_base, dst_base, src0_off, src1_off, offset;
    uint32_t A_len, B_len, C_len;

    A_len = BURST_SIZE;
    B_len = REG_SIZE * PIM_WIDTH;
    C_len = PIM_WIDTH;

    fused_op = pim_args->fused_op;
    if (pim_args->fused_op == FUSED_ELEW_ADD) {
        fused_op = OPCODE_ELE_ADD;
    } else if (pim_args->fused_op == FUSED_ELEW_SUB) {
        fused_op = OPCODE_ELE_SUB;
    } else {
        return -1;
    }

    src0_va = pim_args->srcA_va;
    src1_va = pim_args->srcB_va;
    src2_va = pim_args->bias_va;
    dst_va = pim_args->dstC_va;

    p_size = pim_args->p_size;
    q_size = pim_args->q_size;
    r_size = pim_args->r_size;

    r_loop_var = (r_size * TYPE_SIZE) / PIM_WIDTH;

    q = 0;
    desc_idx = 0;
    uint64_t desc_base = pl_dma[fpga_id].desc_base;
    next_desc = desc_base + 0x40;

    src0_base = 0x0ULL;
    src1_base = 0x0ULL;
    src2_base = 0x0ULL;
    dst_base = 0x0ULL;
    PIM_MATH_LOG("%s: p:%d, q:%d, r:%d\n", __func__, p_size, q_size, r_size);
    for (p = 0; p < p_size; p++) {
        for (r = 0; r < r_loop_var; r++) {
            PIM_MATH_LOG("[p:%d, q:%d, r:%d] \n", p, q, r);
            /* 
             * Operator fusion (C + (A X B) = D)
             * C is first stored in vACC before operations and then accumulated.
             */
            offset = (r * PIM_WIDTH) + (p * r_size * TYPE_SIZE);
            if ((offset % HUGE_PAGE_SIZE) == 0) {
                src2_base = VA2PA(src2_va + offset, fpga_id);
                dst_base = VA2PA(dst_va + offset, fpga_id);
                src2_pa = src2_base;
                dst_pa = dst_base;
            } else {
                src2_pa = src2_base + offset;
                dst_pa = dst_base + offset;
            }
            PIM_RD_INSTR(&desc_idx, &next_desc, src2_pa, BRAM_DUMMY + (offset % 0x40), C_len, RD_B_ATTR | (fused_op << 0x4), fpga_id);

            for (q = 0; q < q_size; q = q + REG_SIZE) {
                //read A
                src0_off = (q * TYPE_SIZE) + ((q_size * TYPE_SIZE) * p);
                if ((src0_off % HUGE_PAGE_SIZE) == 0) {
                    src0_base = VA2PA(src0_va + src0_off, fpga_id);
                    src0_pa = src0_base;
                } else {
                    src0_pa = src0_base + src0_off;
                }
                //read B
                src1_off = (q_size * PIM_WIDTH * r) + (q * PIM_WIDTH);
                if ((src1_off % HUGE_PAGE_SIZE) == 0) {
                    src1_base = VA2PA(src1_va + src1_off, fpga_id);
                    src1_pa = src1_base;
                } else {
                    src1_pa = src1_base + src1_off;
                }
                PIM_RD_INSTR(&desc_idx, &next_desc, src0_pa, BRAM_DUMMY + (src0_off % 0x40), A_len, RD_A_ATTR | (OPCODE_MAT_MUL << 0x4), fpga_id);
                PIM_RD_INSTR(&desc_idx, &next_desc, src1_pa, BRAM_DUMMY + (src1_off % 0x40), B_len, RD_B_ATTR | (OPCODE_MAT_MUL << 0x4), fpga_id);
            }
            PIM_WR_INSTR(&desc_idx, &next_desc, BRAM_DUMMY + (offset % 0x40), dst_pa, C_len, WR_C_ATTR | (OPCODE_MAT_MUL << 0x4), fpga_id);
        }
    }
    pim_args->desc_idx = desc_idx - 1;
    pim_args->desc_last = next_desc - 0x40;
    if (pim_exec(pim_args, fpga_id) < 0) {
        return -1;
    }
    return 0;
}


/*
    For the decoupled PIM
*/
int _matmul_decoupled(pim_args *pim_args, int fpga_id)
{
    uint32_t desc_idx, next_desc, p, q, r;
    uint32_t p_size, q_size, r_size;
    uint32_t A_pa, B_pa, C_pa;
    uint64_t srcA_va, srcB_va, dstC_va, A_va, B_va, C_va, cnt_A;
    uint32_t A_off, B_off, C_off;
    uint32_t A_base, B_base, C_base;
    uint32_t cnt_B, cnt_C;
    uint32_t A_len, B_len, C_len;

    A_len = REG_SIZE * REG_SIZE * TYPE_SIZE;
    B_len = REG_SIZE * NUM_BANKS * TYPE_SIZE;
    C_len = REG_SIZE * NUM_BANKS * TYPE_SIZE;

    srcA_va = pim_args->srcA_va;
    srcB_va = pim_args->srcB_va;
    dstC_va = pim_args->dstC_va;

    p_size = pim_args->p_size;
    q_size = pim_args->q_size;
    r_size = pim_args->r_size;

    //A_ROW_PAD = (p_size + COMPUTE_WAY - 1) / COMPUTE_WAY * COMPUTE_WAY;
    cnt_A = 0;
    cnt_B = 0;
    cnt_C = 0;

    desc_idx = 0;
    uint64_t desc_base = pl_dma[fpga_id].desc_base;
    next_desc = desc_base + 0x40;

    A_base = 0x0ULL;
    B_base = 0x0ULL;
    C_base = 0x0ULL;

    PIM_MATH_LOG("%s: p:%d, q:%d, r:%d\n", __func__, p_size, q_size, r_size);
    for (p = 0; p < p_size; p += REG_SIZE) {
        for (r = 0; r < r_size; r += NUM_BANKS) {
            cnt_A = 0;
            for (q = 0; q < q_size; q += REG_SIZE) {
                PIM_MATH_LOG("[p:%d, q:%d, r:%d] \n", p, q, r);
                A_off = (((cnt_B) % (q_size * r_size)) * TYPE_SIZE);
                B_off = (((p * q_size + cnt_A) % (p_size * q_size)) * TYPE_SIZE);

                if ((B_off % HUGE_PAGE_SIZE) == 0) {
                    B_va = srcB_va + B_off;
                    B_base = VA2PA(B_va, fpga_id);
                    B_pa = B_base;            
                } else {
                    // B_pa = B_base + B_off;
                    B_pa = B_base + (B_off % HUGE_PAGE_SIZE);
                }
                if ((A_off % HUGE_PAGE_SIZE) == 0) {
                    A_va = srcA_va + A_off;
                    A_base = VA2PA(A_va, fpga_id);
                    A_pa = A_base;
                } else {
                    // A_pa = A_base + A_off;
                    A_pa = A_base + (A_off % HUGE_PAGE_SIZE);
                }
                cnt_B += 256;
                cnt_A += REG_SIZE * REG_SIZE;
                PIM_RD_INSTR(&desc_idx, &next_desc, A_pa, BRAM_DUMMY + (A_off % 0x40), A_len, RD_A_ATTR | (OPCODE_MAT_MUL_DECOUPLED_8x4 << 4), fpga_id);
                PIM_RD_INSTR(&desc_idx, &next_desc, B_pa, BRAM_DUMMY + (B_off % 0x40), B_len, RD_B_ATTR | (OPCODE_MAT_MUL_DECOUPLED_8x4 << 4), fpga_id);
            }
            C_off = ((cnt_C) * TYPE_SIZE);
            if ((C_off % HUGE_PAGE_SIZE) == 0) {
                C_va = dstC_va + C_off;
                C_base = VA2PA(C_va, fpga_id);
                C_pa = C_base;
            } else {
                // C_pa = C_base + C_off;
                C_pa = C_base + (C_off % HUGE_PAGE_SIZE);
            }
            cnt_C += REG_SIZE * NUM_BANKS;
            PIM_WR_INSTR(&desc_idx, &next_desc, BRAM_DUMMY + (C_off % 0x40), C_pa, C_len, WR_C_ATTR | (OPCODE_MAT_MUL_DECOUPLED_8x4 << 4), fpga_id);
        }
    }
    pim_args->desc_idx = desc_idx - 1;
    pim_args->desc_last = next_desc - 0x40;
    if (pim_exec(pim_args, fpga_id) < 0) {
        return -1;
    }
    return 0;
}


int _gemm_decoupled(pim_args *pim_args, int fpga_id)
{
    uint32_t desc_idx, next_desc, p, q, r;
    uint32_t p_size, q_size, r_size;
    uint32_t A_pa, B_pa, C_pa;
    uint64_t srcA_va, srcB_va, bias_va, dstC_va, A_va, B_va, C_va;
    uint32_t A_off, B_off, C_off;
    uint32_t A_base, B_base, C_base;
    uint32_t cnt_B, cnt_C;
    uint32_t A_len, B_len, C_len;

    A_len = REG_SIZE * REG_SIZE * TYPE_SIZE;
    B_len = REG_SIZE * NUM_BANKS * TYPE_SIZE;
    C_len = REG_SIZE * NUM_BANKS * TYPE_SIZE;

    srcA_va = pim_args->srcA_va;
    srcB_va = pim_args->srcB_va;
    bias_va = pim_args->bias_va;    
    dstC_va = pim_args->dstC_va;

    p_size = pim_args->p_size;
    q_size = pim_args->q_size;
    r_size = pim_args->r_size;

    q = 0;
    desc_idx = 0;
    uint64_t desc_base = pl_dma[fpga_id].desc_base;
    next_desc = desc_base + 0x40;

    A_base = 0x0ULL;
    B_base = 0x0ULL;
    C_base = 0x0ULL;

    //A_ROW_PAD = (p_size + COMPUTE_WAY - 1) / COMPUTE_WAY * COMPUTE_WAY;
    cnt_B = 0;
    cnt_C = 0;

    PIM_MATH_LOG("%s: p:%d, q:%d, r:%d\n", __func__, p_size, q_size, r_size);
    for (p = 0; p < p_size; p += REG_SIZE) {
        for (r = 0; r < r_size; r += NUM_BANKS) {
            uint64_t cnt_A = 0;
            for (q = 0; q < q_size; q += REG_SIZE) {
                PIM_MATH_LOG("[p:%d, q:%d, r:%d] \n", p, q, r);
                B_off = (((cnt_B) % (q_size * r_size)) * TYPE_SIZE);
                if ((B_off % HUGE_PAGE_SIZE) == 0) {
                    B_va = srcB_va + B_off;
                    B_base = VA2PA(B_va, fpga_id);
                    B_pa = B_base;          
                } else {
                    // B_pa = B_base + B_off;
                    B_pa = B_base + (B_off % HUGE_PAGE_SIZE);
                }
                cnt_B += 256;

                A_off = (((p * q_size + cnt_A) % (p_size * q_size)) * TYPE_SIZE);
                if ((A_off % HUGE_PAGE_SIZE) == 0) {
                    A_va = srcA_va + A_off;
                    A_base = VA2PA(A_va, fpga_id);
                    A_pa = A_base;
                } else {
                    // A_pa = A_base + A_off;
                    A_pa = A_base + (A_off % HUGE_PAGE_SIZE);
                }
                cnt_A += REG_SIZE * REG_SIZE;

                PIM_RD_INSTR(&desc_idx, &next_desc, B_pa, BRAM_DUMMY + (B_off % 0x40), B_len, RD_B_ATTR | (OPCODE_MAT_MUL_DECOUPLED_8x4 << 4), fpga_id);
                PIM_RD_INSTR(&desc_idx, &next_desc, A_pa, BRAM_DUMMY + (A_off % 0x40), A_len, RD_A_ATTR | (OPCODE_MAT_MUL_DECOUPLED_8x4 << 4), fpga_id);
            }
            C_off = ((cnt_C) * TYPE_SIZE);
            if ((C_off % HUGE_PAGE_SIZE) == 0) {
                C_va = dstC_va + C_off;
                C_base = VA2PA(C_va, fpga_id);
                C_pa = C_base;
            } else {
                // C_pa = C_base + C_off;
                C_pa = C_base + (C_off % HUGE_PAGE_SIZE);
            }
            cnt_C += REG_SIZE * NUM_BANKS;
            PIM_WR_INSTR(&desc_idx, &next_desc, BRAM_DUMMY + (C_off % 0x40), C_pa, C_len, WR_C_ATTR | (OPCODE_MAT_MUL_DECOUPLED_8x4 << 4), fpga_id);

        }
    }
    uint32_t p_loop_var, offset, r_need_align, r_size_aligned, r_loop_var;
    uint32_t desc_idx2;

    srcA_va = pim_args->srcA_va;
    srcB_va = pim_args->srcB_va;
    dstC_va = pim_args->dstC_va;

    p_size = pim_args->p_size;
    q_size = pim_args->q_size;
    r_size = pim_args->r_size;

    //Element-wise Addition
    p_loop_var = p_size;
    r_loop_var = r_size;
    r_need_align = ( (r_size & 0x1F)) ? 1 : 0;

    if (r_need_align)
        r_size_aligned = (r_size / 32 + 1) * 32;
    else
        r_size_aligned = r_size;

    desc_idx2 = desc_idx;
    for (p = 0; p < p_loop_var; p++) {
        for (r = 0; r < r_loop_var; r = r + 512) {
            PIM_MATH_LOG("[p:%d, q:%d, r:%d] \n", p, q, r);
            offset = (p * r_size_aligned * TYPE_SIZE) + (r * TYPE_SIZE);
            if ((offset % HUGE_PAGE_SIZE) == 0) {
                A_base = VA2PA(bias_va + offset, fpga_id);
                B_base = VA2PA(dstC_va + offset, fpga_id);
                C_base = VA2PA(dstC_va + offset, fpga_id);
                A_pa = A_base;
                B_pa = B_base;
                C_pa = C_base; 
            } else {
                A_pa = A_base + offset;
                B_pa = B_base + offset;
                C_pa = C_base + offset;
            }
            PIM_RD_INSTR(&desc_idx2, &next_desc, A_pa, BRAM_DUMMY, C_len, RD_A_ATTR | (OPCODE_ELE_ADD << 0x4), fpga_id);
            PIM_RD_INSTR(&desc_idx2, &next_desc, B_pa, BRAM_DUMMY, C_len, RD_B_ATTR | (OPCODE_ELE_ADD << 0x4), fpga_id);
            PIM_WR_INSTR(&desc_idx2, &next_desc, BRAM_DUMMY, C_pa, C_len, WR_C_ATTR | (OPCODE_ELE_ADD << 0x4), fpga_id);
        }
    }

    pim_args->desc_idx = desc_idx2 - 1;
    pim_args->desc_last = next_desc - 0x40;
    if (pim_exec(pim_args, fpga_id) < 0) {
        return -1;
    }
    return 0;
}

int _matmul_fusion_decoupled(pim_args *pim_args, int fpga_id)
{
    uint32_t desc_idx, next_desc, p, q, r, fused_op;
    uint32_t p_size, q_size, r_size;
    uint32_t src0_pa, src1_pa, src2_pa, dst_pa;
    uint64_t src0_va, src1_va, src2_va, dstC_va;
    uint32_t src0_base, src1_base, src2_base, dst_base, src0_off, src1_off, offset;
    uint32_t A_len, B_len, C_len;
    uint32_t cnt_B, cnt_C;

    A_len = REG_SIZE * REG_SIZE * TYPE_SIZE;
    B_len = REG_SIZE * NUM_BANKS * TYPE_SIZE;
    C_len = REG_SIZE * NUM_BANKS * TYPE_SIZE;

    fused_op = pim_args->fused_op;
    if (pim_args->fused_op == FUSED_ELEW_ADD) {
        fused_op = OPCODE_ELE_ADD;
    } else if (pim_args->fused_op == FUSED_ELEW_SUB) {
        fused_op = OPCODE_ELE_SUB;
    } else {
        return -1;
    }

    src0_va = pim_args->srcA_va;
    src1_va = pim_args->srcB_va;
    src2_va = pim_args->bias_va;
    dstC_va = pim_args->dstC_va;

    p_size = pim_args->p_size;
    q_size = pim_args->q_size;
    r_size = pim_args->r_size;

    desc_idx = 0;
    uint64_t desc_base = pl_dma[fpga_id].desc_base;
    next_desc = desc_base + 0x40;

    //A_ROW_PAD = (p_size + COMPUTE_WAY - 1) / COMPUTE_WAY * COMPUTE_WAY; ???
    cnt_B = 0;
    cnt_C = 0;

    src0_base = 0x0ULL;
    src1_base = 0x0ULL;
    src2_base = 0x0ULL;
    dst_base = 0x0ULL;

    PIM_MATH_LOG("%s: p:%d, q:%d, r:%d\n", __func__, p_size, q_size, r_size);
    for (p = 0; p < p_size; p += REG_SIZE) {
        for (r = 0; r < r_size; r += NUM_BANKS) {
            /* 
             * Operator fusion (C + (A X B) = D)
             * C is first stored in vACC before operations and then accumulated.
             */
            uint64_t cnt_A = 0;
            offset = ((cnt_C) * TYPE_SIZE);
            if ((offset % HUGE_PAGE_SIZE) == 0) {
                src2_base = VA2PA(src2_va + offset, fpga_id);
                dst_base = VA2PA(dstC_va + offset, fpga_id);
                src2_pa = src2_base;
                dst_pa = dst_base;
            } else {
                src2_pa = src2_base + offset;
                dst_pa = dst_base + offset;
            }
            cnt_C += REG_SIZE * NUM_BANKS;
            PIM_RD_INSTR(&desc_idx, &next_desc, src2_pa, BRAM_DUMMY, C_len, RD_B_ATTR | (fused_op << 0x4), fpga_id);

            for (q = 0; q < q_size; q += REG_SIZE) {
                PIM_MATH_LOG("[p:%d, q:%d, r:%d] \n", p, q, r);
                src1_off = ((cnt_B) % (q_size * r_size)) * TYPE_SIZE;
                if ((src1_off % HUGE_PAGE_SIZE) == 0) {
                    src1_base = VA2PA(src1_va + src1_off, fpga_id);
                    src1_pa = src1_base;
                } else {
                    src1_pa = src1_base + src1_off;
                }                
                cnt_B += 512;

                src0_off= ((p * q_size + cnt_A) % (p_size * q_size)) * TYPE_SIZE;
                if ((src0_off % HUGE_PAGE_SIZE) == 0) {
                    src0_base = VA2PA(src0_va + src0_off, fpga_id);
                    src0_pa = src0_base;
                } else {
                    src0_pa = src0_base + src0_off;
                }
                cnt_A += REG_SIZE * REG_SIZE;

                PIM_RD_INSTR(&desc_idx, &next_desc, src1_pa, BRAM_DUMMY + (src1_off % 0x40), B_len, RD_B_ATTR | (OPCODE_MAT_MUL_DECOUPLED_8x4 << 4), fpga_id);
                PIM_RD_INSTR(&desc_idx, &next_desc, src0_pa, BRAM_DUMMY + (src0_off % 0x40), A_len, RD_A_ATTR | (OPCODE_MAT_MUL_DECOUPLED_8x4 << 4), fpga_id);
            }
            PIM_WR_INSTR(&desc_idx, &next_desc, BRAM_DUMMY + (offset % 0x40), dst_pa, C_len, WR_C_ATTR | (OPCODE_MAT_MUL_DECOUPLED_8x4 << 4), fpga_id);

        }
    }
    pim_args->desc_idx = desc_idx - 1;
    pim_args->desc_last = next_desc - 0x40;
    if (pim_exec(pim_args, fpga_id) < 0) {
        return -1;
    }
    return 0;
}
