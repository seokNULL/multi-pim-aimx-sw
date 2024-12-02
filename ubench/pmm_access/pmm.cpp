#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h> 
#include <stdint.h>

#include <sys/ioctl.h>
#include <stdbool.h>

#include <pim.h>
#include "convert_numeric.h"
#include "pim_mac_simul.h"
#include "result_checker.h"
#include <vector>

// #define CACHE_FLUSH
#define ITER 1
#define PERF
struct timespec start_PIM, end_PIM;
struct timespec start_CPU, end_CPU;
uint64_t diff_PIM;
uint64_t diff_CPU;
int iter;
// #define TEST_BULK
// #define TEST_SCHED

void elewise_add_CPU(float *src_A_DRAM, float * src_B_DRAM, float * dst_C_DRAM, int p_size, int q_size)
{
  
  for(int i=0; i<p_size*q_size;i++)
  {
    dst_C_DRAM[i] = src_A_DRAM[i] + src_B_DRAM[i];
  }
}

void elewise_sub_CPU(float *src_A_DRAM, float * src_B_DRAM, float * dst_C_DRAM, int p_size, int q_size)
{
  
  for(int i=0; i<p_size*q_size;i++)
  {
    dst_C_DRAM[i] = src_A_DRAM[i] - src_B_DRAM[i];
  }
}

void elewise_mul_CPU(float *src_A_DRAM, float * src_B_DRAM, float * dst_C_DRAM, int p_size, int q_size)
{
  
  for(int i=0; i<p_size*q_size;i++)
  {
    dst_C_DRAM[i] = src_A_DRAM[i] * src_B_DRAM[i];
  }
}

void elewise_op_pim_simul(short* A, short* B, short* C, int M, int N, int opcode) {  
  switch (opcode) {
    case 0:
    {
      short one = 16256;
      AccumulatorData zero;
      zero.sign = false;
      zero.exp = 0;
      zero.mant = 0;
      for (int i = 0; i < M * N; i++) {
        BF16 bf16_a(reinterpret_cast<uint16_t&>(A[i]));
        BF16 bf16_b(reinterpret_cast<uint16_t&>(B[i]));        
        BF16 bf16_one(reinterpret_cast<uint16_t&>(one)); //  '1.0'
        MultiplierData mul_out_a = multiplyBfloat16(bf16_a, one);
        MultiplierData mul_out_b = multiplyBfloat16(bf16_b, one);
        AccumulatorData add_out_a = addBfloat16(mul_out_a, zero);
        C[i] = normalize(addBfloat16(mul_out_b, add_out_a));
      }
      break;
    }
    case 1:
    {
      short one = 16256;
      AccumulatorData zero;
      zero.sign = false;
      zero.exp = 0;
      zero.mant = 0;
      for (int i = 0; i < M * N; i++) {
        BF16 bf16_a(reinterpret_cast<uint16_t&>(A[i]));
        BF16 bf16_b(reinterpret_cast<uint16_t&>(B[i]));        
        MultiplierData mul_result = multiplyBfloat16(bf16_a, bf16_b);        
        C[i] = float_to_short(convertToFP32(mul_result));
      }
      break;
    }
    case 2:
    {
      short one = 16256;
      AccumulatorData zero;
      zero.sign = false;
      zero.exp = 0;
      zero.mant = 0;
      for (int i = 0; i < M * N; i++) {
        BF16 bf16_a(reinterpret_cast<uint16_t&>(A[i]));
        BF16 bf16_b(reinterpret_cast<uint16_t&>(B[i]));        
        BF16 bf16_one(reinterpret_cast<uint16_t&>(one)); //  '1.0'
        MultiplierData mul_out_a = multiplyBfloat16(bf16_a, one);        
        MultiplierData mul_out_b = multiplyBfloat16(bf16_b, one);
        if (mul_out_b.exp == 0) mul_out_b.sign = 0;
        else mul_out_b.sign = mul_out_b.sign ^ 1;
        AccumulatorData add_out_a = addBfloat16(mul_out_a, zero);        
        C[i] = normalize(addBfloat16(mul_out_b, add_out_a));
      }
      break;
    }
  }
}

int main(int argc, char *argv[])
{

    if(argc<3)
    {
        printf("Check vector param p,q,r (pxq) +-x (pxq) = (pxq)\n");
        exit(1);
    }
    
    int p_size = atoi(argv[1]);
    int q_size = atoi(argv[2]);
    int r_size = q_size;
    int cmd = atoi(argv[3]);

    int srcA_size = p_size * q_size;
    int srcB_size = p_size * q_size;
    int dstC_size = p_size * q_size;

    int fd_dma=0;
    init_pim_drv();
    if ((fd_dma=open(PL_DMA_DRV, O_RDWR|O_SYNC)) < 0) {
        perror("PL DMA drvier open");
        exit(-1);
    }
        //For CPU verify
    float *src_A_DRAM = (float *)(mmap(NULL, srcA_size*sizeof(float), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    float *src_B_DRAM = (float *)(mmap(NULL, srcB_size*sizeof(float), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    float *dst_C_DRAM = (float *)(mmap(NULL, dstC_size*sizeof(float), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

  #ifdef PMM
    // Alloc PMM (PIM Memory Map) and Set PIU base register as base address of PMM
    uint64_t* pmm_va = (uint64_t*)pim_malloc(PMM_SIZE);
    uint64_t pmm_pa = VA2PA((uint64_t)&pmm_va[0], 0);
    set_piu_base_addr(pmm_pa);
    for(size_t i=0; i<srcA_size; i++){
      unsigned long long tmp  = 0x3f80;
      pmm_va[i] = tmp;
    }
  #endif

    short *PL_srcA_buf = (short *)pim_malloc(srcA_size*sizeof(short));
    short *PL_srcB_buf = (short *)pim_malloc(srcB_size*sizeof(short));
    short *PL_dstC_buf = (short *)pim_malloc(dstC_size*sizeof(short));
    short *PL_dstC_simul = (short *)malloc(dstC_size*sizeof(short));  //  Dedicated page for PIM SIMUL

    short *vecA_buf = (short *)pim_malloc(srcA_size*sizeof(short));
    short *vACC_buf = (short *)pim_malloc(dstC_size*sizeof(short));

    pim_args *set_info;
    int size = sizeof(pim_args);
    set_info = (pim_args *)malloc(1024*1024*size);

    if (PL_srcA_buf == MAP_FAILED) {
        printf("PL srcA call failure.\n");
        return -1;
    }
    if (PL_srcB_buf == MAP_FAILED) {
        printf("PL srcB call failure.\n");
        return -1;
    }
    if (PL_dstC_buf == MAP_FAILED) {
        printf("PL dstC call failure.\n");
        return -1;
    } 
    for(size_t i=0; i<srcA_size; i++){
      PL_srcA_buf[i]=0;
    }
    for(size_t i=0; i<srcB_size; i++){
      PL_srcB_buf[i]=0;
    }
    for(size_t i=0; i<dstC_size; i++){
      PL_dstC_buf[i]=0;
      dst_C_DRAM[i]  = 0;
    }

    printf("srcA init\n");
    float tmp2 = 0;
    for(size_t i=0; i<srcA_size; i++){
      float tmp  = generate_random();
      tmp2  = 2;
      // tmp2  = i;
      // float tmp  = i;
      // float tmp  = 0.25;
      // short tmp0 = float_to_short(tmp2);
      short tmp0 = float_to_short(tmp);
      PL_srcA_buf[i] = tmp0;
      src_A_DRAM[i] = short_to_float(tmp0);
    }

    printf("srcB init\n");
    float tmp3 = -1;
    for(size_t i=0; i<srcB_size; i++){
      float tmp  = generate_random_255();
      // tmp3  = 0;
      tmp3  = 3;
      // tmp3  = (i%16 == 0) ? 0 : tmp3+1;
      // float tmp  = 256;
      short tmp0 = float_to_short(tmp3);
      // short tmp0 = float_to_short(tmp);
      PL_srcB_buf[i]=tmp0;
      src_B_DRAM[i] = short_to_float(tmp0);
    }    
    // Input data
    srand((unsigned int)time(NULL));  // For reset random seed

    set_info->srcA_va   = (uint64_t)&PL_srcA_buf[0];
    set_info->srcB_va   = (uint64_t)&PL_srcB_buf[0];
    set_info->dstC_va   = (uint64_t)&PL_dstC_buf[0];
    set_info->p_size    = p_size;
    set_info->q_size    = q_size;
    set_info->r_size    = r_size;

  #ifdef PMM
    set_info->pmm_base_addr = (uint64_t)&pmm_va[0];
    set_info->vecA_buf = (uint64_t)&vecA_buf[0];
    set_info->vACC_buf = (uint64_t)&vACC_buf[0];
  #endif
    
    elewise_add_CPU(src_A_DRAM, src_B_DRAM, dst_C_DRAM, p_size, q_size);
    // elewise_add(set_info);
    pmm_access(set_info);
    elewise_op_pim_simul(PL_srcA_buf, PL_srcB_buf, PL_dstC_simul, p_size, q_size, cmd);
      
    union converter{
    float f_val;
    unsigned int u_val;
    };
    union converter a;
    union converter b;
    int fail_flag = 0;
    int first_fail_idx = 0;

    printf("Correctness check!\n\n");
    printf("             CPU      |    PIM   | PIM SIMUL |   \n");
    printf("-------------------------------------------------\n");     

    for(int i=0; i<dstC_size; i++){ 
        a.f_val = dst_C_DRAM[i];

        // printf("idx[%4d] 0x%x  |  ", i, a.u_val);
        // printf("0x%x  |  ", (uint16_t)PL_dstC_buf[i]);
        // printf("0x%x   |", (uint16_t)PL_dstC_simul[i]);
        // printf("\n");        
        if(PL_dstC_buf[i] != PL_dstC_simul[i]) {          
          printf("idx[%4d] 0x%x  |  ", i, a.u_val);
          printf("0x%x  |  ", (uint16_t)PL_dstC_buf[i]);
          printf("0x%x   |", (uint16_t)PL_dstC_simul[i]);
          printf("\n"); 
          if (fail_flag == 0) first_fail_idx = i;       
          fail_flag = 1;
          if (i == 16) break;
        }
    }

    printf("             srcA      |    vecA      \n");
    printf("-------------------------------------------------\n");     
    for(int i=0; i<srcA_size; i++){        
      printf("idx[%4d] 0x%x  |  ", i, (uint16_t)PL_srcA_buf[i]);
      printf("0x%x  |  ", (uint16_t)vecA_buf[i]);
      printf("\n"); 
    }

    if (fail_flag) {
      printf("First fail at Idx[%d] \n", first_fail_idx);
      printf("PIM result and PIM SIMUL result always should be same.\n");
      printf("\nCorrectness check error !\n");
    }
    else {
      printf("Correctness check success !!!\n");
    }
    printf("vecA_buf: %llx\n", vecA_buf);
    pim_free(PL_srcA_buf);
    pim_free(PL_srcB_buf);
    pim_free(PL_dstC_buf);
    free(PL_dstC_simul);
    return 0;
}




