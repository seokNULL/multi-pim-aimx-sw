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
    int flush_size = 20*1024*1024;

    int srcA_size = p_size * q_size;
    int srcB_size = p_size * q_size;
    int dstC_size = p_size * q_size;

    int tmp=0;
    int fd_dma=0;
    int fd_conf=0;
    init_pim_drv();
    if ((fd_dma=open(PL_DMA_DRV, O_RDWR|O_SYNC)) < 0) {
        perror("PL DMA drvier open");
        exit(-1);
    }
        //For CPU verify
    float *src_A_DRAM = (float *)(mmap(NULL, srcA_size*sizeof(float), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    float *src_B_DRAM = (float *)(mmap(NULL, srcB_size*sizeof(float), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    float *dst_C_DRAM = (float *)(mmap(NULL, dstC_size*sizeof(float), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    double *flush_buffer = (double *)(mmap(NULL, flush_size, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

    short *PL_srcA_buf = (short *)pim_malloc(srcA_size*sizeof(short));
    short *PL_srcB_buf = (short *)pim_malloc(srcB_size*sizeof(short));
    short *PL_dstC_buf = (short *)pim_malloc(dstC_size*sizeof(short));
    short *PL_dstC_simul = (short *)malloc(dstC_size*sizeof(short));  //  Dedicated page for PIM SIMUL

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
#ifdef PERF
    // printf("Complete pim_malloc\n");
    uint64_t A_pa = VA2PA((uint64_t)&PL_srcA_buf[0]);    
    uint64_t B_pa = VA2PA((uint64_t)&PL_srcB_buf[0]);    
    uint64_t C_pa = VA2PA((uint64_t)&PL_dstC_buf[0]);    
    printf("A:%llx \n", A_pa);
    printf("B:%llx \n", B_pa);
    printf("C:%llx \n", C_pa);
    // getchar();    
    //zeroing
    for(size_t i=0; i<srcA_size; i++){
      PL_srcA_buf[i]=0;
    }
    for(size_t i=0; i<srcB_size; i++){
      PL_srcB_buf[i]=0;
    }
    for(size_t i=0; i<dstC_size; i++){
      PL_dstC_buf[i]=0;
    }

    
    // printf("dstC init\n");
    for(size_t i=0; i<dstC_size; i++){
      PL_dstC_buf[i] = 0;
      dst_C_DRAM[i]  = 0;
    }

    printf("srcA init\n");
    float tmp2 = 0;
    for(size_t i=0; i<srcA_size; i++){
      float tmp  = generate_random();
      tmp2  = 1;
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
      tmp3  = 1;
      // tmp3  = (i%16 == 0) ? 0 : tmp3+1;
      // float tmp  = 256;
      // short tmp0 = float_to_short(tmp3);
      short tmp0 = float_to_short(tmp);
      PL_srcB_buf[i]=tmp0;
      src_B_DRAM[i] = short_to_float(tmp0);
    }    
#endif
    // Input data
    srand((unsigned int)time(NULL));  // For reset random seed

    set_info->srcA_va   = (uint64_t)&PL_srcA_buf[0];
    set_info->srcB_va   = (uint64_t)&PL_srcB_buf[0];
    set_info->dstC_va   = (uint64_t)&PL_dstC_buf[0];
    set_info->p_size    = p_size;
    set_info->q_size    = q_size;
    set_info->r_size    = r_size;
    
    uint64_t dur_CPU = 0;
    uint64_t dur_PIM = 0;

    switch (cmd) {
      case 0:
        dur_CPU = 0;
        for(int i = 0; i < ITER; i++) {
          clock_gettime(CLOCK_MONOTONIC, &start_CPU);
          elewise_add_CPU(src_A_DRAM, src_B_DRAM, dst_C_DRAM, p_size, q_size);
          clock_gettime(CLOCK_MONOTONIC, &end_CPU);  
          diff_CPU = BILLION * (end_CPU.tv_sec - start_CPU.tv_sec) + (end_CPU.tv_nsec - start_CPU.tv_nsec);
          dur_CPU += diff_CPU;
          printf("CPU(system memory) elewise add latency %llu ns at iter #%d\t%llu\n", (long long unsigned int) diff_CPU, i, (long long unsigned int)diff_CPU);
        }
        printf("CPU average elewise add latency %llu ns %d %d\t%llu\n", (long long unsigned int) dur_CPU/ITER, p_size, q_size, (long long unsigned int) dur_CPU/ITER);

        elewise_op_pim_simul(PL_srcA_buf, PL_srcB_buf, PL_dstC_simul, p_size, q_size, cmd);
      
        dur_PIM = 0;
        for(int i = 0; i < ITER; i++) {
          clock_gettime(CLOCK_MONOTONIC, &start_PIM);
          elewise_add(set_info);
          clock_gettime(CLOCK_MONOTONIC, &end_PIM);  
          diff_PIM = BILLION * (end_PIM.tv_sec - start_PIM.tv_sec) + (end_PIM.tv_nsec - start_PIM.tv_nsec);
          dur_PIM += diff_PIM;
          printf("PIM elewise add latency %llu ns at iter #%d\t%llu\n", (long long unsigned int) diff_PIM, i, (long long unsigned int) diff_PIM);
        }
        printf("PIM average elewise add latency %llu ns %d %d\t%llu\n", (long long unsigned int) dur_PIM/ITER, p_size, q_size, (long long unsigned int) dur_PIM/ITER);
        break;

      case 1:
        dur_CPU = 0;
        for(int i = 0; i < ITER; i++) {
          clock_gettime(CLOCK_MONOTONIC, &start_CPU);
          elewise_mul_CPU(src_A_DRAM, src_B_DRAM, dst_C_DRAM, p_size, q_size);
          clock_gettime(CLOCK_MONOTONIC, &end_CPU);  
          diff_CPU = BILLION * (end_CPU.tv_sec - start_CPU.tv_sec) + (end_CPU.tv_nsec - start_CPU.tv_nsec);
          dur_CPU += diff_CPU;
          printf("CPU elewise mul latency %llu ns at iter #%d\t%llu\n", (long long unsigned int) diff_CPU, i, (long long unsigned int) diff_CPU);
        }
        printf("CPU average elewise mul latency %llu ns %d %d %d\t%llu\n", (long long unsigned int) dur_CPU/ITER, p_size, q_size, r_size, (long long unsigned int) dur_CPU/ITER);
      
        dur_PIM = 0;
        for(int i = 0; i < ITER; i++) {
          clock_gettime(CLOCK_MONOTONIC, &start_PIM);
          elewise_mul(set_info);
          clock_gettime(CLOCK_MONOTONIC, &end_PIM);  
          diff_PIM = BILLION * (end_PIM.tv_sec - start_PIM.tv_sec) + (end_PIM.tv_nsec - start_PIM.tv_nsec);
          dur_PIM += diff_PIM;
          printf("PIM elewise mul latency %llu ns at iter #%d\t%llu\n", (long long unsigned int) diff_PIM, i, (long long unsigned int) diff_PIM);
        }
        printf("PIM average elewise mul latency %llu ns %d %d %d\t%llu\n", (long long unsigned int) dur_PIM/ITER, p_size, q_size, r_size, (long long unsigned int) dur_PIM/ITER);
        
        for(int i = 0; i < ITER; i++) {          
          elewise_op_pim_simul(PL_srcA_buf, PL_srcB_buf, PL_dstC_simul, p_size, q_size, cmd);
        }
        break;
        
      case 2:
        dur_CPU = 0;
        for(int i = 0; i < ITER; i++) {
          clock_gettime(CLOCK_MONOTONIC, &start_CPU);
          elewise_sub_CPU(src_A_DRAM, src_B_DRAM, dst_C_DRAM, p_size, q_size);
          clock_gettime(CLOCK_MONOTONIC, &end_CPU);  
          diff_CPU = BILLION * (end_CPU.tv_sec - start_CPU.tv_sec) + (end_CPU.tv_nsec - start_CPU.tv_nsec);
          dur_CPU += diff_CPU;
          printf("CPU elewise sub latency %llu ns at iter #%d\t%llu\n", (long long unsigned int) diff_CPU, i, (long long unsigned int) diff_CPU);
        }
        printf("CPU average elewise sub latency %llu ns %d %d %d\t%llu\n", (long long unsigned int) dur_CPU/ITER, p_size, q_size, r_size, (long long unsigned int) dur_CPU/ITER);
      
        dur_PIM = 0;
        for(int i = 0; i < ITER; i++) {
          clock_gettime(CLOCK_MONOTONIC, &start_PIM);
          elewise_sub(set_info);
          clock_gettime(CLOCK_MONOTONIC, &end_PIM);  
          diff_PIM = BILLION * (end_PIM.tv_sec - start_PIM.tv_sec) + (end_PIM.tv_nsec - start_PIM.tv_nsec);
          dur_PIM += diff_PIM;
          printf("PIM elewise sub latency %llu ns at iter #%d\t%llu\n", (long long unsigned int) diff_PIM, i, (long long unsigned int) diff_PIM);
        }
        printf("PIM average elewise sub latency %llu ns %d %d %d\t%llu\n", (long long unsigned int) dur_PIM/ITER, p_size, q_size, r_size, (long long unsigned int) dur_PIM/ITER);
        
        for(int i = 0; i < ITER; i++) {          
          elewise_op_pim_simul(PL_srcA_buf, PL_srcB_buf, PL_dstC_simul, p_size, q_size, cmd);
        }
        break;
      // case 3:
      //   dur_CPU = 0;
      //   for(int i = 0; i < ITER; i++) {
      //     clock_gettime(CLOCK_MONOTONIC, &start_CPU);
      //     elewise_add_CPU(PL_srcA_buf, PL_srcB_buf, PL_dstC_buf, p_size, q_size);
      //     clock_gettime(CLOCK_MONOTONIC, &end_CPU);  
      //     diff_CPU = BILLION * (end_CPU.tv_sec - start_CPU.tv_sec) + (end_CPU.tv_nsec - start_CPU.tv_nsec);
      //     dur_CPU += diff_CPU;
      //     printf("CPU elewise add latency %llu ns at iter #%d\t%llu\n", (long long unsigned int) diff_CPU, i, (long long unsigned int)diff_CPU);
      //   }
      //   printf("CPU average elewise add latency %llu ns %d %d %d\t%llu\n", (long long unsigned int) dur_CPU/ITER, p_size, q_size, r_size, (long long unsigned int) dur_CPU/ITER);
      
      //   dur_PIM = 0;
      //   for(int i = 0; i < ITER; i++) {
      //     clock_gettime(CLOCK_MONOTONIC, &start_PIM);
      //     elewise_add(set_info);
      //     clock_gettime(CLOCK_MONOTONIC, &end_PIM);  
      //     diff_PIM = BILLION * (end_PIM.tv_sec - start_PIM.tv_sec) + (end_PIM.tv_nsec - start_PIM.tv_nsec);
      //     dur_PIM += diff_PIM;
      //     printf("PIM elewise add latency %llu ns at iter #%d\t%llu\n", (long long unsigned int) diff_PIM, i, (long long unsigned int) diff_PIM);
      //   }
      //   printf("PIM average elewise add latency %llu ns %d %d %d\t%llu\n", (long long unsigned int) dur_PIM/ITER, p_size, q_size, r_size, (long long unsigned int) dur_PIM/ITER);      
      //   break;

      default:
        printf("wrong cmd...");
        return 0;
    }
#ifdef PERF
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

        #ifdef DUMP_BANK0_ONLY
        if ((i%256)==0){ // Only Bank 0
          for(int j=0; j<16; j++){
            a.f_val = dst_C_DRAM[i+j];
            printf("idx[%4d] 0x%x  |  ", i, a.u_val);
            printf("0x%x ", PL_dstC_buf[i+j]);
            printf("\n");
          }
        }
        #else
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
        #endif
    }
    if (fail_flag) {
      printf("First fail at Idx[%d] \n", first_fail_idx);
      printf("PIM result and PIM SIMUL result always should be same.\n");
      printf("\nCorrectness check error !\n");
    }
    else {
      printf("Correctness check success !!!\n");
    }
#endif
    // result_check:
    // std::vector<float> cpu_output(p_size * r_size, 0.0f);
    // std::vector<float> pim_output(p_size * r_size, 0.0f);
    // std::vector<float> PIM_SIMUL_output(p_size * r_size, 0.0f);

    // for (int i = 0; i < p_size * r_size; i++) {
    //   // cpu_output[i] = dst_C_DRAM[i];
    //   pim_output[i] = short_to_float(PL_dstC_buf[i]);
    //   PIM_SIMUL_output[i] = short_to_float(PL_dstC_simul[i]);      
    // }    

    // printf("\n===================== PIM VS PIM_SIMUL ======================\n");
    // ResultChecker::compare_results(pim_output, PIM_SIMUL_output);
    pim_free(PL_srcA_buf);
    pim_free(PL_srcB_buf);
    pim_free(PL_dstC_buf);
    free(PL_dstC_simul);
    return 0;
}




