#ifndef _PIM_MATH_LIB_
#define _PIM_MATH_LIB_

#include <sys/mman.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include "../drv_src/pim_mem_lib_user.h"
#include "../drv_src/addr_lib_user.h"
#include "../../include/pim.h"
#include "../drv_src/multi_dev_lib_user.h"

#define OP_ATTR_ADDR 0x6F00
#define BRAM_HIGH 0x2U
#define BRAM_LOW 0x0U
#define TEST_NOP
#define CODE_WR_ADDR 0x7000
/* For Operand Attribute */ 
#define CL_SIZE_OFFSET 		  1  // 1
#define CL_PER_TOT_DIM_OFFSET 13 // 1+12
#define CL_MODE_OFFSET 		  29 // 13+16
#define CODE_READ	0xdeadbea1
#define OPER_A_READ 0xdeadbea2
#define OPER_B_READ 0xdeadbea3
#define OPER_C_READ 0xdeadbea4


#define PREVENT_PIU_CONF  0x200000 	// Do not use this descriptor for PIU configuration
#define OPCODE_BCAST_CONF 0x200002
#define OPCODE_CL_CONF	  0x200001
#define OPCODE_NORM_COPY  0x200000
#define OPCODE_NORM_COPY_SV  0x200200
#define OPCODE_NORM_COPY_MatMul  0x200000




#define OPCODE_MAT_MUL 0x851F
#define OPCODE_MAT_MUL_DECOUPLED_8x4  0x1051F
#define OPCODE_ELE_ADD 0x42F
#define OPCODE_GEMM_ADD 0x1042F

#define OPCODE_ELE_SUB 0x44F
#define OPCODE_ELE_MUL 0x48F
#define OPCODE_PROF 0x100000

#define BURST_SIZE 32 /* Byte size */
#define TYPE_SIZE sizeof(short) /* Pre-defined bfloat16 type */
#define VECA_SIZE 16 /* vecA entry size */
#define REG_SIZE ((BURST_SIZE / TYPE_SIZE) * VECA_SIZE) /* vecA and vACC register size */
// #define REG_SIZE (BURST_SIZE / TYPE_SIZE) /* vecA and vACC register size */
#define vecA_SIZE 32
#define vACC_SIZE 32
#define NUM_BANKS 16
#define ALU_SIZE  16
#define PIM_WIDTH (ALU_SIZE * NUM_BANKS * TYPE_SIZE)
// #define PIM_WIDTH (REG_SIZE * NUM_BANKS * TYPE_SIZE)

#define RD_A_ATTR 0x2
#define WR_A_ATTR 0x3
#define RD_B_ATTR 0x4
#define WR_C_ATTR 0x9
#define RD_C_ATTR 0x8
#define BATCH1 0x0
#define BATCH2 0x20000
#define BATCH4 0x40000
#define BATCH8 0x60000

/* For Descriptor */
#define CDMA_NEXT_DE_L       0x00
#define CDMA_NEXT_DE_H       0x04
#define CDMA_DESC_SA_L       0x08
#define CDMA_DESC_SA_H       0x0C
#define CDMA_DESC_DA_L       0x10
#define CDMA_DESC_DA_H       0x14
#define CDMA_DESC_LEN        0x18
#define CDMA_DESC_STATUS     0x1C
#define CDMA_DESC_INFO       0x24 /* Use reserved bit for PIM opcode */

/* AXI CDMA Register Address Map */
#define CDMA_REG_CR          0x00
#define CDMA_REG_SR          0x04
#define CDMA_CURDESC_PNTR_L  0x08
#define CDMA_CURDESC_PNTR_H  0x0C
#define CDMA_TAILDESC_PNTR_L 0x10
#define CDMA_TAILDESC_PNTR_H 0x14
#define CDMA_REG_SA_L        0x18
#define CDMA_REG_SA_H        0x1C
#define CDMA_REG_DA_L        0x20
#define CDMA_REG_DA_H        0x24
#define CDMA_REG_BYTETRANS   0x28

#define BRAM_DUMMY 0x0
// #define BRAM_DUMMY 0xd0000000
//#define CONF_OFFSET_HPC_CLR 0x5000

typedef struct
{
	/* CDMA_NEXT_DE_L   */ uint32_t next_l;
	/* CDMA_NEXT_DE_H   */ uint32_t next_h;
	/* CDMA_DESC_SA_L   */ uint32_t src_l;
	/* CDMA_DESC_SA_H   */ uint32_t src_h;
	/* CDMA_DESC_DA_L   */ uint32_t dst_l;
	/* CDMA_DESC_DA_H   */ uint32_t dst_h;
	/* CDMA_DESC_LEN    */ uint32_t len;
	/* CDMA_DESC_STATUS */ uint32_t status;
} __attribute__((aligned(64))) pim_isa_t;

typedef struct
{
	/* CDMA_NEXT_DE_L   */ uint32_t next_l;
	/* CDMA_NEXT_DE_H   */ uint32_t next_h;
	/* CDMA_DESC_SA_L   */ uint32_t src_l;
	/* CDMA_DESC_SA_H   */ uint32_t src_h;
	/* CDMA_DESC_DA_L   */ uint32_t dst_l;
	/* CDMA_DESC_DA_H   */ uint32_t dst_h;
	/* CDMA_DESC_LEN    */ uint32_t len;
	/* CDMA_DESC_STATUS */ uint32_t status;
} __attribute__((aligned(32))) pim_isa_32_t;

#ifdef CLP
typedef struct
{
	uint64_t srcA_addr_size;
	uint16_t attrA;
	uint64_t srcB_addr_size;
	uint16_t attrB;
	uint64_t dstC_addr_size;
	uint16_t attrC;
	uint16_t reserved;
	// uint32_t srcA;
	// uint32_t attrA;
	// uint32_t srcB;
	// uint32_t attrB;
	// uint32_t dstC;
	// uint32_t attrC;
	// uint64_t status;
}  __attribute__ ((packed)) cl_data_t;
#endif


static inline char *decode_opcode(int opcode) {
	switch(opcode)
	{
		case 0: return "INIT";
		case (RD_A_ATTR | OPCODE_MAT_MUL << 0x4): return "MAT_MUL_SILENT_RD_A";
		case (RD_B_ATTR | OPCODE_MAT_MUL << 0x4): return "MAT_MUL_SILENT_RD_B";
		case (WR_C_ATTR | OPCODE_MAT_MUL << 0x4): return "MAT_MUL_SILENT_WR_C";
		case (RD_A_ATTR | OPCODE_MAT_MUL_DECOUPLED_8x4 << 0x4): return "MAT_MUL_DECOUPLED_RD_BANK_PRIVATE";
		case (RD_B_ATTR | OPCODE_MAT_MUL_DECOUPLED_8x4 << 0x4): return "MAT_MUL_DECOUPLED_RD_BANK_SHARED";
		case (WR_C_ATTR | OPCODE_MAT_MUL_DECOUPLED_8x4 << 0x4): return "MAT_MUL_DECOUPLED_WR_C";
		case (RD_A_ATTR | OPCODE_ELE_ADD<<0x4): return "ELE_ADD_RD_A";
		case (RD_B_ATTR | OPCODE_ELE_ADD<<0x4): return "ELE_ADD_RD_B";
		case (WR_C_ATTR | OPCODE_ELE_ADD<<0x4): return "ELE_ADD_WR_C";
		case (RD_A_ATTR | OPCODE_ELE_SUB<<0x4): return "ELE_SUB_RD_A";
		case (RD_B_ATTR | OPCODE_ELE_SUB<<0x4): return "ELE_SUB_RD_B";
		case (WR_C_ATTR | OPCODE_ELE_SUB<<0x4): return "ELE_SUB_WR_C";
		case (RD_A_ATTR | OPCODE_ELE_MUL<<0x4): return "ELE_MUL_RD_A";
		case (RD_B_ATTR | OPCODE_ELE_MUL<<0x4): return "ELE_MUL_RD_B";
		case (WR_C_ATTR | OPCODE_ELE_MUL<<0x4): return "ELE_MUL_WR_C";
		case (OPCODE_MAT_MUL): return "OpAttr_MAT_MUL";
		case (OPCODE_ELE_ADD): return "OpAttr_ELE_ADD";
		case (OPCODE_ELE_SUB): return "OpAttr_ELE_SUB";
		case (OPCODE_ELE_MUL): return "OpAttr_ELE_MUL";
		case (CODE_READ)  : return "CODE_READ";
		case (OPER_A_READ): return "OPERAND_A_READ";
		case (OPER_B_READ): return "OPERAND_B_READ";
		case (OPER_C_READ): return "OPERAND_C_READ";
		default: return "None";
	}
}

/*
 * For Multi-PIM operation
 */
#define pim_exec(...) _pim_exec((struct exec_args){__VA_ARGS__})
#define PIM_RD_INSTR(...) _PIM_RD_INSTR((struct instr_args){__VA_ARGS__})
#define PIM_WR_INSTR(...) _PIM_WR_INSTR((struct instr_args){__VA_ARGS__})

struct pl_dma_t {
    char dev_name[50];

	uint64_t desc_base;
	uint64_t dram_base; //
	int fd;
	pim_isa_t *pim_isa;
	
	pthread_mutex_t lock;		/* protects concurrent access */
};

struct exec_args {
	pim_args *pim_args;
	int fpga_id;
};

struct instr_args {
	uint32_t *idx;
	uint32_t *next;
	uint32_t src;
	uint32_t dst;
	uint32_t length;
	uint32_t opcode;
	int fpga_id;
};

extern struct pl_dma_t *pl_dma;

extern int mpim_exec(pim_args *pim_args, int fpga_id);
int _pim_exec(struct exec_args in);

static inline void mPIM_RD_INSTR(uint32_t *idx, uint32_t *next, 
							 uint32_t src, uint32_t dst, uint32_t length, uint32_t opcode, int fpga_id) 
{
	struct pl_dma_t *pl_dma_node = &pl_dma[fpga_id];
	
	pl_dma_node->pim_isa[*idx].next_l = *next;
	pl_dma_node->pim_isa[*idx].next_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].src_l = src;
	pl_dma_node->pim_isa[*idx].src_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].dst_l = dst;
	pl_dma_node->pim_isa[*idx].dst_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].len = length;
	pl_dma_node->pim_isa[*idx].status = 0x0U | opcode;
    PIM_MATH_LOG("   DESCR[idx:%3d] next:%x | src:%10x | dst:%10x | length: %6x | opcode: %s\n", 
    							*idx, pl_dma_node->pim_isa[*idx].next_l, pl_dma_node->pim_isa[*idx].src_l, pl_dma_node->pim_isa[*idx].dst_l, pl_dma_node->pim_isa[*idx].len, decode_opcode(opcode));
    (*idx)++;
    (*next)+=0x40;
}

static inline void mPIM_WR_INSTR(uint32_t *idx, uint32_t *next, 
							 uint32_t src, uint32_t dst, uint32_t length, uint32_t opcode, int fpga_id) 
{
	struct pl_dma_t *pl_dma_node = &pl_dma[fpga_id];

  	pl_dma_node->pim_isa[*idx].next_l = *next;
	pl_dma_node->pim_isa[*idx].next_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].src_l = src;
	pl_dma_node->pim_isa[*idx].src_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].dst_l = dst;
	pl_dma_node->pim_isa[*idx].dst_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].len = length;
	pl_dma_node->pim_isa[*idx].status = 0x0U | opcode;
    PIM_MATH_LOG("   DESCR[idx:%3d] next:%x | src:%10x | dst:%10x | length: %6x | opcode: %s\n", 
    							*idx, pl_dma_node->pim_isa[*idx].next_l, pl_dma_node->pim_isa[*idx].src_l, pl_dma_node->pim_isa[*idx].dst_l, pl_dma_node->pim_isa[*idx].len, decode_opcode(opcode));
    (*idx)++;
    (*next)+=0x40;
}

static inline void _PIM_RD_INSTR(struct instr_args in)
{
    int fpga_id_out = in.fpga_id? in.fpga_id : 0;
	mPIM_RD_INSTR(in.idx, in.next, in.src, in.dst, in.length, in.opcode, fpga_id_out);
}
static inline void _PIM_WR_INSTR(struct instr_args in)
{
    int fpga_id_out = in.fpga_id? in.fpga_id : 0;
	mPIM_WR_INSTR(in.idx, in.next, in.src, in.dst, in.length, in.opcode, fpga_id_out);
}
#ifdef QDMA
static inline void ConfigOpAttr(uint32_t *idx, uint32_t *next, cl_data_t* op_attr_data, cl_data_t* op_attr_data_pim, uint32_t srcA_pa, uint32_t srcB_pa, uint32_t dstC_pa,
#else
static inline void ConfigOpAttr(uint32_t *idx, uint32_t *next, cl_data_t* op_attr_data, uint32_t srcA_pa, uint32_t srcB_pa, uint32_t dstC_pa,
#endif
								uint32_t batch_col, uint32_t batch_size, uint32_t p_size, uint32_t q_loop_var, uint32_t r_loop_var, uint32_t opcode, int fpga_id){
	struct pl_dma_t *pl_dma_node = &pl_dma[fpga_id];
	
	uint64_t cl_size_A, cl_size_B, cl_size_C;
	uint32_t cl_per_total_dim_A, cl_per_total_dim_B, cl_per_total_dim_C;
	uint32_t cl_mode_A, cl_mode_B, cl_mode_C;
	switch(opcode)
	{
		case OPCODE_MAT_MUL:
			// cl_size_B = 0;
			// cl_per_total_dim_B = 0;
			// cl_mode_B = 0x0;	// 001: bcast, 010: cl
			// cl_size_A = REG_CHUNK*REG_CHUNK*TYPE_SIZE;
			// cl_per_total_dim_A = q_loop_var*r_loop_var;
			// cl_mode_A = 0x2;
			// cl_size_C = 0;
			// cl_per_total_dim_C = 0;
			// cl_mode_C = 0x0;
			cl_size_A = 1;
			cl_per_total_dim_A = NUM_BANKS*batch_size;
			cl_mode_A = 0x3;	// 001: bcast, 010: cl
			cl_size_B = batch_col*REG_CHUNK*REG_CHUNK*TYPE_SIZE/BURST_SIZE;
			// cl_size_B = (MAX_BATCH/batch_size)*REG_CHUNK*REG_CHUNK*TYPE_SIZE/BURST_SIZE;
			// cl_size_B = REG_CHUNK*REG_CHUNK*TYPE_SIZE;
			cl_per_total_dim_B = q_loop_var*r_loop_var;
			cl_mode_B = 0x2;
			// cl_size_B = 0;	// if zero, ffff...
			// cl_per_total_dim_B = 0;
			// cl_mode_B = 0x0;
			cl_size_C = 0;
			cl_per_total_dim_C = 0;
			cl_mode_C = 0x0;
			break;
		case OPCODE_ELE_ADD:
		case OPCODE_ELE_SUB:
		case OPCODE_ELE_MUL:
			cl_size_A = NUM_BANKS;
			cl_per_total_dim_A = p_size*r_loop_var;
			cl_mode_A = 0x2;
			cl_size_B = batch_size/(32*CH_NUM);
			cl_per_total_dim_B = batch_col;
			cl_mode_B = 0x2;
			cl_size_C = NUM_BANKS;
			cl_per_total_dim_C = p_size*r_loop_var;
			cl_mode_C = 0x2;
			break;
		default: 
			cl_size_A = 0x0;
			cl_per_total_dim_A = 0x0;
			cl_mode_A = 0x0;
			cl_size_B = 0x0;
			cl_per_total_dim_B = 0x0;
			cl_mode_B = 0x0;
			cl_size_C = 0x0;
			cl_per_total_dim_C = 0x0;
			cl_mode_C = 0x0;
			printf("case default\n");
			break;
	}
	op_attr_data[0].srcA_addr_size = (uint64_t)(srcA_pa) | (cl_size_A << 33);	// TODO: H/W에서 size bit field 31bit으로 맞추기(주소와의 합을 64bit로 하기 위함)
	op_attr_data[0].attrA  = (uint64_t)(cl_mode_A << 13) | (cl_per_total_dim_A);
	op_attr_data[0].srcB_addr_size = ((uint64_t)(srcB_pa) | (cl_size_B << 33));
	op_attr_data[0].attrB  = (uint64_t)(cl_mode_B << 13) | (cl_per_total_dim_B);
	op_attr_data[0].dstC_addr_size = (uint64_t)(dstC_pa) | (cl_size_C << 33);
	op_attr_data[0].attrC  = (uint64_t)(cl_mode_C << 13) | (cl_per_total_dim_C);
    PIM_MATH_LOG("op_attr_data A_base:%x | A_attr:%d/%d/%d | B_base:%x | B_attr:%d/%d/%d | C_base:%x | C_attr:%d/%d/%d\n", 
    				op_attr_data[0].srcA_addr_size&0x1ffffffff, (op_attr_data[0].srcA_addr_size>>33), (op_attr_data[0].attrA)&0xfff, (op_attr_data[0].attrA>>13)&0X7, 
					op_attr_data[0].srcB_addr_size&0x1ffffffff, (op_attr_data[0].srcB_addr_size>>33), (op_attr_data[0].attrB)&0xfff, (op_attr_data[0].attrB>>13)&0X7, 
					op_attr_data[0].dstC_addr_size&0x1ffffffff, (op_attr_data[0].dstC_addr_size>>33), (op_attr_data[0].attrC)&0xfff, (op_attr_data[0].attrC>>13)&0X7);


	// op_attr_data[0].srcA   = srcA_pa;
	// op_attr_data[0].attrA  = (cl_mode_A << CL_MODE_OFFSET) | (cl_per_total_dim_A << CL_PER_TOT_DIM_OFFSET) | (cl_size_A << CL_SIZE_OFFSET);
	// op_attr_data[0].srcB   = srcB_pa;
	// op_attr_data[0].attrB  = (cl_mode_B << CL_MODE_OFFSET) | (cl_per_total_dim_B << CL_PER_TOT_DIM_OFFSET) | (cl_size_B << CL_SIZE_OFFSET);
	// op_attr_data[0].dstC   = dstC_pa;
	// op_attr_data[0].attrC  = (cl_mode_C << CL_MODE_OFFSET) | (cl_per_total_dim_C << CL_PER_TOT_DIM_OFFSET) | (cl_size_C << CL_SIZE_OFFSET);
	// op_attr_data[0].status = 0x0UL;
    // PIM_MATH_LOG("op_attr_data A_base:%x | A_attr:%d/%d/%d | B_base:%x | B_attr:%d/%d/%d | C_base:%x | C_attr:%d/%d/%d\n", 
    // 				op_attr_data[0].srcA, (op_attr_data[0].attrA>>CL_MODE_OFFSET)&0x7, (op_attr_data[0].attrA>>CL_PER_TOT_DIM_OFFSET)&0xffff, (op_attr_data[0].attrA>>CL_SIZE_OFFSET)&0Xfff, 
	// 				op_attr_data[0].srcB, (op_attr_data[0].attrB>>CL_MODE_OFFSET)&0x7, (op_attr_data[0].attrB>>CL_PER_TOT_DIM_OFFSET)&0xffff, (op_attr_data[0].attrB>>CL_SIZE_OFFSET)&0Xfff, 
	// 				op_attr_data[0].dstC, (op_attr_data[0].attrC>>CL_MODE_OFFSET)&0x7, (op_attr_data[0].attrC>>CL_PER_TOT_DIM_OFFSET)&0xffff, (op_attr_data[0].attrC>>CL_SIZE_OFFSET)&0Xfff);


	pl_dma_node->pim_isa[*idx].next_l = *next;
	pl_dma_node->pim_isa[*idx].next_h = HIGH_ADDR;
#ifdef QDMA
	pl_dma_node->pim_isa[*idx].src_l  = VA2PA(op_attr_data_pim, fpga_id);
#else
	pl_dma_node->pim_isa[*idx].src_l  = VA2PA(op_attr_data, fpga_id);
#endif
	pl_dma_node->pim_isa[*idx].src_h  = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].dst_l  = OP_ATTR_ADDR;
	pl_dma_node->pim_isa[*idx].dst_h  = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].len    = BURST_SIZE;
	pl_dma_node->pim_isa[*idx].status = 0x0U | opcode;

    PIM_MATH_LOG("  OpAttr[idx:%3d] next:%x | src:%10x | dst:%10x | length: %6x | opcode: %s\n", 
    							*idx, pl_dma_node->pim_isa[*idx].next_l, pl_dma_node->pim_isa[*idx].src_l, pl_dma_node->pim_isa[*idx].dst_l, pl_dma_node->pim_isa[*idx].len, decode_opcode(opcode));
    (*idx)++;
    (*next)+=0x40;	
}
static inline void ConfigPimRdInstr(uint32_t *idx, uint32_t src, uint32_t dst, uint32_t length, uint32_t opcode, int fpga_id, pim_isa_32_t* code_list) 
{
	// struct pl_dma_t *pl_dma_node = &pl_dma[fpga_id];

  	code_list[*idx].next_l = 0x0U;
	code_list[*idx].next_h = 0x0U;
	code_list[*idx].src_l = src;
	code_list[*idx].src_h = HIGH_ADDR;
	code_list[*idx].dst_l = dst;
	code_list[*idx].dst_h = HIGH_ADDR;
	code_list[*idx].len = length;
	code_list[*idx].status = 0x0U | opcode;

	PIM_MATH_LOG("    	CODE[idx:%3d] src:%10x | dst:%10x | length: %6x | opcode: %x\n", 
									*idx, code_list[*idx].src_l, code_list[*idx].dst_l, code_list[*idx].len, opcode);
	// PIM_MATH_LOG("    	CODE[idx:%3d] src:%10x | dst:%10x | length: %6x | opcode: %s\n", 
	// 								*idx, code_list[*idx].src_l, code_list[*idx].dst_l, code_list[*idx].len, decode_opcode(opcode));
    (*idx)++;
}

static inline void ConfigPimWrInstr(uint32_t *idx, uint32_t src, uint32_t dst, uint32_t length, uint32_t opcode, int fpga_id, pim_isa_32_t* code_list) 
{
  	code_list[*idx].next_l = 0x0U;
	code_list[*idx].next_h = 0x0U;
	code_list[*idx].src_l = src;
	code_list[*idx].src_h = HIGH_ADDR;
	code_list[*idx].dst_l = dst;
	code_list[*idx].dst_h = HIGH_ADDR;
	code_list[*idx].len = length;
	code_list[*idx].status = 0x0U | opcode;

	PIM_MATH_LOG("    	CODE[idx:%3d] src:%10x | dst:%10x | length: %6x | opcode: %x\n", 
									*idx, code_list[*idx].src_l, code_list[*idx].dst_l, code_list[*idx].len, opcode);
	// PIM_MATH_LOG("    	CODE[idx:%3d] src:%10x | dst:%10x | length: %6x | opcode: %s\n", 
	// 								*idx, code_list[*idx].src_l, code_list[*idx].dst_l, code_list[*idx].len, decode_opcode(opcode));
    (*idx)++;
}

static inline void RdPimCode(uint32_t *idx, uint32_t *next, 
							 uint32_t src, uint32_t dst, uint32_t length, uint32_t opcode, int fpga_id) 
{
	struct pl_dma_t *pl_dma_node = &pl_dma[fpga_id];

  	pl_dma_node->pim_isa[*idx].next_l = *next;
	pl_dma_node->pim_isa[*idx].next_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].src_l = src;
	pl_dma_node->pim_isa[*idx].src_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].dst_l = dst;
	pl_dma_node->pim_isa[*idx].dst_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].len = length;
	pl_dma_node->pim_isa[*idx].status = 0x0U | opcode;
    PIM_MATH_LOG("   DESCR[idx:%3d] next:%x | src:%10x | dst:%10x | length: %6x | opcode: %s\n", 
    							*idx, pl_dma_node->pim_isa[*idx].next_l, pl_dma_node->pim_isa[*idx].src_l, pl_dma_node->pim_isa[*idx].dst_l, pl_dma_node->pim_isa[*idx].len, decode_opcode(opcode));
    (*idx)++;
    (*next)+=0x40;
}

static inline void RdPimReg(uint32_t *idx, uint32_t *next, 
							 uint32_t src, uint32_t dst, uint32_t length, uint32_t opcode, int fpga_id) 
{
	struct pl_dma_t *pl_dma_node = &pl_dma[fpga_id];

  	pl_dma_node->pim_isa[*idx].next_l = *next;
	pl_dma_node->pim_isa[*idx].next_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].src_l = src;
	pl_dma_node->pim_isa[*idx].src_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].dst_l = dst;
	pl_dma_node->pim_isa[*idx].dst_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].len = length;
	pl_dma_node->pim_isa[*idx].status = 0x0U | opcode;
    PIM_MATH_LOG("   DESCR[idx:%3d] next:%x | src:%10x | dst:%10x | length: %6x | opcode: %s\n", 
    							*idx, pl_dma_node->pim_isa[*idx].next_l, pl_dma_node->pim_isa[*idx].src_l, pl_dma_node->pim_isa[*idx].dst_l, pl_dma_node->pim_isa[*idx].len, decode_opcode(opcode));
    (*idx)++;
    (*next)+=0x40;
}
static inline void WrPimReg(uint32_t *idx, uint32_t *next, 
							 uint32_t src, uint32_t dst, uint32_t length, uint32_t opcode, int fpga_id) 
{
	struct pl_dma_t *pl_dma_node = &pl_dma[fpga_id];

  	pl_dma_node->pim_isa[*idx].next_l = *next;
	pl_dma_node->pim_isa[*idx].next_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].src_l = src;
	pl_dma_node->pim_isa[*idx].src_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].dst_l = dst;
	pl_dma_node->pim_isa[*idx].dst_h = HIGH_ADDR;
	pl_dma_node->pim_isa[*idx].len = length;
	pl_dma_node->pim_isa[*idx].status = 0x0U | opcode;
    PIM_MATH_LOG("   DESCR[idx:%3d] next:%x | src:%10x | dst:%10x | length: %6x | opcode: %s\n", 
    							*idx, pl_dma_node->pim_isa[*idx].next_l, pl_dma_node->pim_isa[*idx].src_l, pl_dma_node->pim_isa[*idx].dst_l, pl_dma_node->pim_isa[*idx].len, decode_opcode(opcode));
    (*idx)++;
    (*next)+=0x40;
}
#endif
