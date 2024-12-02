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
#include <vector>
// #define  DUMP_BANK0_ONLY

#define FPGA_ID 0
#define ITER  1
// #define PERF
// #define DECOUP
// #define INPUT_COPY_BY_CPU

struct timespec start_PIM, end_PIM;
struct timespec start_CPU, end_CPU;
uint64_t diff_PIM;
uint64_t diff_CPU;
int iter;

double get_current_time() {
  struct timespec tv;
  clock_gettime(CLOCK_MONOTONIC, &tv);
  return tv.tv_sec + tv.tv_nsec * 1e-9;
}

void mat_mul_CPU(float *src_A_DRAM, float * src_B_DRAM, float * dst_C_DRAM, int p_size, int q_size, int r_size)
{
  
  union converter{
  float f_val;
  unsigned int u_val;
  };
  union converter a;
  union converter b;
  union converter c;
  union converter d;

  for(size_t c_r=0; c_r < p_size; c_r++)
  {
    for(size_t c_c=0; c_c<r_size; c_c++)
    {
      float tmp=0.0f;
      float mul_tmp=0.0f;
      for(size_t k=0; k<q_size; k++)
      {
        unsigned residual=0;
        if(q_size>32) residual= (q_size-32)*16;

        unsigned row=((k/32)*512)+(c_r*512);
        unsigned col=k%32;

        mul_tmp = (src_A_DRAM[ k + c_r*q_size ] * src_B_DRAM[(k*r_size) + c_c]);

        tmp+=(src_A_DRAM[ k + c_r*q_size ] * src_B_DRAM[(k*r_size) + c_c]);

        a.f_val = src_A_DRAM[ k + c_r*q_size ];
        b.f_val = src_B_DRAM[(k*r_size) + c_c];
        c.f_val = mul_tmp;
        d.f_val = tmp;

        //if(c_c==0) printf("idx[%lu] a(%x) x b(%x) = c(%x) || acc=%x \n", k, a.u_val, b.u_val, c.u_val, d.u_val);
      }
      dst_C_DRAM[c_c+c_r*r_size]=tmp;
    }
  }
  
  // Naive CPU matrix multiplication(ubench)
  // int M = p_size;
  // int K = q_size;
  // int N = r_size;
  // for (int i = 0; i < M; ++i) {
  //   for (int k = 0; k < K; ++k) {
  //     for (int j = 0; j < N; ++j) {
  //       dst_C_DRAM[i * N + j] += src_A_DRAM[i * K + k] * src_B_DRAM[k * N + j];
  //     }
  //   }
  // }
}
void pim_batch_align_chunk(const short *src, short *dst, int batch_size, int row_dim, int col_dim) {
    int batch_chunk  = (batch_size < MAX_BATCH) ? batch_size : MAX_BATCH;
    int col_chunk = (MAX_BATCH/batch_chunk)*REG_CHUNK*CH_NUM;
    int batch_col = (col_dim<col_chunk)? col_dim/(REG_CHUNK*CH_NUM) : MAX_BATCH/batch_chunk;
    int CHUNK = REG_CHUNK * CH_NUM * batch_col; // suppose col_dim is multiple of (REG_CHUNK*CH_NUM)
    // int CHUNK = 256 * CH_NUM;
    int col_chunk_num = 0;
    if (col_dim % CHUNK != 0) {
        col_chunk_num = CHUNK * (col_dim / CHUNK + 1) / CHUNK;
    } else {
      col_chunk_num = col_dim / CHUNK;
    }
    
    // int col_chunk_num =  512 * (col_dim / 512 + 1) / CHUNK; // CHUNK = 512
    int dest_idx = 0;
    if (col_dim > CHUNK) {
        for (int i = 0; i < col_chunk_num; i++) {
            for (int j = 0; j < row_dim; j++) {
                if (i < col_chunk_num - 1) {
                    for (int k = 0; k < CHUNK; k++) {
                        dst[dest_idx] = (src[(i*CHUNK)+(j*col_dim)+k]);
                        // std::cout << "dest_idx: "<< dest_idx << "\tsrc_index: " << (i*CHUNK)+(j*col_dim)+k << std::endl;
                        dest_idx++;
                    }
                }
                // LAST COL NUM
                else {
                    for (int k = 0; k < CHUNK; k++) {
                        if (i * CHUNK + k < col_dim) {
                            dst[dest_idx] = (src[(i*CHUNK)+(j*col_dim)+k]);
                            // std::cout << "dest_idx: "<< dest_idx << "\tsrc_index: " << (i*CHUNK)+(j*col_dim)+k << std::endl;
                        } else {
                            // printf("ZERO padding");
                            dst[dest_idx] = (0);
                        }
                        dest_idx++;
                    }
                }
            }
        }
        // std::cout << "dest_idx: " << dest_idx << std::endl;
    } else if (col_dim == CHUNK) {
        for (int i = 0; i < row_dim * col_dim; i++) {
            dst[i] = (src[i]);
        }
    } else {
        printf("NOT SUPPORTED, NEED ALIGNMENT");
    }
}

void pim_align_chunk(const short *src, short *dst, int row_dim, int col_dim) {
    int CHUNK = 256 * CH_NUM * B_CHUNK;
    // int CHUNK = 256 * CH_NUM;
    int col_chunk_num = 0;
    if (col_dim % CHUNK != 0) {
        col_chunk_num = CHUNK * (col_dim / CHUNK + 1) / CHUNK;
    } else {
      col_chunk_num = col_dim / CHUNK;
    }
    
    // int col_chunk_num =  512 * (col_dim / 512 + 1) / CHUNK; // CHUNK = 512
    int dest_idx = 0;
    if (col_dim > CHUNK) {
        for (int i = 0; i < col_chunk_num; i++) {
            for (int j = 0; j < row_dim; j++) {
                if (i < col_chunk_num - 1) {
                    for (int k = 0; k < CHUNK; k++) {
                        dst[dest_idx] = (src[(i*CHUNK)+(j*col_dim)+k]);
                        // std::cout << "dest_idx: "<< dest_idx << "\tsrc_index: " << (i*CHUNK)+(j*col_dim)+k << std::endl;
                        dest_idx++;
                    }
                }
                // LAST COL NUM
                else {
                    for (int k = 0; k < CHUNK; k++) {
                        if (i * CHUNK + k < col_dim) {
                            dst[dest_idx] = (src[(i*CHUNK)+(j*col_dim)+k]);
                            // std::cout << "dest_idx: "<< dest_idx << "\tsrc_index: " << (i*CHUNK)+(j*col_dim)+k << std::endl;
                        } else {
                            // printf("ZERO padding");
                            dst[dest_idx] = (0);
                        }
                        dest_idx++;
                    }
                }
            }
        }
        // std::cout << "dest_idx: " << dest_idx << std::endl;
    } else if (col_dim == CHUNK) {
        for (int i = 0; i < row_dim * col_dim; i++) {
            dst[i] = (src[i]);
        }
    } else {
        printf("NOT SUPPORTED, NEED ALIGNMENT");
    }
}

void batch_align_srcA(const short *src, short *dst, int row_dim, int col_dim) {
  // chunk 배수가 아닌 입력에 대한 예외 처리 필요, 일단 배수로 가정
  int dst_idx = 0;
  int row_chunk_size  = row_dim/MAX_BATCH;
  int col_chunk_size  = col_dim/REG_CHUNK;
  int aligned_col_dim = col_dim; // 예외 처리 필요
  for (int io = 0; io < row_chunk_size; io++) {
    for (int j = 0; j < col_chunk_size; j++) {
      for (int ii = 0; ii < MAX_BATCH; ii++) {
        for (int k = 0; k < REG_CHUNK; k++) {
          dst[dst_idx] = src[aligned_col_dim*MAX_BATCH*io + aligned_col_dim*ii + REG_CHUNK*j + k];
          dst_idx++;
        }
      }
    }
  }
}

void batch_align_dstC(const short *src, short *dst, int row_dim, int col_dim) {
  // chunk 배수가 아닌 입력에 대한 예외 처리 필요, 일단 배수로 가정
  int src_idx = 0;
  int row_chunk_size  = row_dim/MAX_BATCH;
  int col_chunk_size  = col_dim/REG_CHUNK;
  int aligned_col_dim = col_dim; // 예외 처리 필요
  for (int io = 0; io < row_chunk_size; io++) {
    for (int j = 0; j < col_chunk_size; j++) {
      for (int ii = 0; ii < MAX_BATCH; ii++) {
        for (int k = 0; k < REG_CHUNK; k++) {
          dst[aligned_col_dim*MAX_BATCH*io + aligned_col_dim*ii + REG_CHUNK*j + k] = src[src_idx];
          src_idx++;
        }
      }
    }
  }
}

void pim_matmul_simul(short *A, short *B, short *C, int m, int n, int k) {
    // Silent-PIM based data layout considered
    // C = A * B
    // A는 m x n 행렬
    // B는 n x k 행렬
    // C는 m x k 행렬
    // assert(k % 2048 == 0);

    // int col_chunk_size = 256*CH_NUM*CLUSTER_SIZE;
    int col_chunk_size = 256*CH_NUM*B_CHUNK;
    // int col_chunk_size = 256*CH_NUM;
    // int col_chunk_size = 2048;
    int n_col_chunk = k/col_chunk_size;    
    
    for (int i = 0; i < m; i++) {
      for (int col_chunk_idx = 0; col_chunk_idx < n_col_chunk; col_chunk_idx++) {
        for (int j = 0; j < col_chunk_size; j++) {            
            AccumulatorData vAcc;
            vAcc.sign = false;
            vAcc.exp = 0;
            vAcc.mant = 0;            
            for (int p = 0; p < n; p++) {                
                BF16 bf16_a(reinterpret_cast<uint16_t&>(A[i * n + p]));
                BF16 bf16_b(reinterpret_cast<uint16_t&>(B[n * col_chunk_size * col_chunk_idx + col_chunk_size * p + j]));
                MultiplierData temp = multiplyBfloat16(bf16_a, bf16_b);
                AccumulatorData vAcc_buf;
                vAcc_buf.sign = vAcc.sign;
                vAcc_buf.exp = vAcc.exp;
                vAcc_buf.mant = vAcc.mant;             
                vAcc = addBfloat16(temp, vAcc);              
            }
            C[i * k + col_chunk_size * col_chunk_idx + j] = normalize(vAcc);
        }
      }
    }
}

void matmul_bfloat16_ref(short *A, short *B, short *C, int m, int n, int k) {
    // Silent-PIM based data layout considered
    // C = A * B
    // A는 m x n 행렬
    // B는 n x k 행렬
    // C는 m x k 행렬
    assert(k % 256 == 0);

    int col_chunk_size = 256*CH_NUM;
    int n_col_chunk = k/256;    
    
    for (int i = 0; i < m; i++) {
      for (int col_chunk_idx = 0; col_chunk_idx < n_col_chunk; col_chunk_idx++) {
        for (int j = 0; j < col_chunk_size; j++) {
            float sum = 0.0f;            
            for (int p = 0; p < n; p++) {
                BF16 bf16_a(reinterpret_cast<uint16_t&>(A[i * n + p]));
                BF16 bf16_b(reinterpret_cast<uint16_t&>(B[n * col_chunk_size * col_chunk_idx + col_chunk_size * p + j]));                
                MultiplierData mul_result = multiplyBfloat16(bf16_a, bf16_b);
                float temp = convertToFP32(mul_result);
                float sum_buf = sum;
                sum += temp;        
                uint32_t tmp = reinterpret_cast<uint32_t&>(sum);   
                tmp = tmp & 0xFFFFFF00;
                sum = reinterpret_cast<float&>(tmp);                
#ifdef DEBUG_Llama_MatMul
                if((col_chunk_size * col_chunk_idx + j) == target_idx) {                                    
                  bf16_ref_partial_sum[p] = sum;
                  input_A_BF16_REF[p] = temp;
                  input_B_BF16_REF[p] = sum_buf;
                }         
#endif
            }
            C[i * k + col_chunk_size * col_chunk_idx + j] = float_to_short(sum);
        }
      }
    }
}
/* ======================================= PIM MatMul simul & BF16 op reference ======================================= */

int main(int argc, char *argv[])
{

    if(argc<3)
    {
        printf("Check matrix param p,q,r (pxq) x (qxr) = (pxr)\n");
        exit(1);
    }
    int p_size=atoi(argv[1]);
    int q_size=atoi(argv[2]);
    int r_size=atoi(argv[3]);
    int srcA_size=p_size*q_size;
    int srcB_size=q_size*r_size;
    int dstC_size=p_size*r_size;
    int fpga_id=atoi(argv[4]);
    // const int fpga_id = 0;

    bool is_need_align=false;
    printf("TEST FPGA NUM: %d\n", fpga_id);
    init_pim_drv();


    uint64_t* pim_reg_map = (uint64_t*)pim_malloc(PMM_SIZE);
    if (pim_reg_map == NULL) {
        printf("PIM code section allocation faield.\n");
        return -1;
    }
    
    //For CPU verify
    float *src_A_DRAM = (float *)(mmap(NULL, srcA_size*sizeof(float), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    float *src_B_DRAM = (float *)(mmap(NULL, srcB_size*sizeof(float), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    float *dst_C_DRAM = (float *)(mmap(NULL, dstC_size*sizeof(float), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

    srand((unsigned int)time(NULL));  // For reset random seed

    void *srcA;
    void *srcB;
    void *dstC;

    uint64_t srcA_va;
    uint64_t srcB_va;
    uint64_t dstC_va;

/* ======= Operand Allocation ======= */
#ifdef INPUT_COPY_BY_CPU
    short *PL_srcA_buf = (short *)pim_malloc(srcA_size*sizeof(short)*CH_NUM, fpga_id);
#else
    short *PL_srcA_buf = (short *)pim_malloc(srcA_size*sizeof(short), fpga_id);
#endif
    short *PL_srcB_buf = (short *)pim_malloc(srcB_size*sizeof(short), fpga_id);
    short *PL_dstC_buf = (short *)pim_malloc(dstC_size*sizeof(short), fpga_id);

    short *PL_srcA_simul = (short *)malloc(srcA_size*sizeof(short));
    short *PL_srcB_simul = (short *)malloc(srcB_size*sizeof(short));
    short *PL_dstC_simul = (short *)malloc(dstC_size*sizeof(short));

    if (PL_srcA_buf == NULL) {
        printf("PL srcA call failure.\n");
        return -1;
    }
    if (PL_srcB_buf == NULL) {
        printf("PL srcB call failure.\n");
        return -1;
    }
    if (PL_dstC_buf == NULL) { 
        printf("PL dstC call failure.\n");
        return -1;
    }
    uint64_t A_pa = VA2PA((uint64_t)&PL_srcA_buf[0], fpga_id);    
    uint64_t B_pa = VA2PA((uint64_t)&PL_srcB_buf[0], fpga_id);    
    uint64_t C_pa = VA2PA((uint64_t)&PL_dstC_buf[0], fpga_id); 



    printf("A_pa:%llx \n", A_pa);
    printf("B_pa:%llx \n", B_pa);
    printf("C_pa:%llx \n", C_pa);

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

    printf("srcA init\n");
    float constant_value = 1;
    for(size_t i=0; i<srcA_size; i++){
      float ramdom_value  = generate_random();
      // constant_value  = (i%(256) == 0)? constant_value + 1 : constant_value;
      constant_value  = 1;
      // float ramdom_value  = 0.25;
      // short tmp0 = float_to_short(constant_value);
      short tmp0 = float_to_short(ramdom_value);
  #ifdef INPUT_COPY_BY_CPU
      for (int j=0; j<CH_NUM; j++) PL_srcA_buf[16*CH_NUM*(i/16) + (16*j+i)%(16*CH_NUM)] = tmp0;
  #else
      PL_srcA_buf[i]   = tmp0;
  #endif
      PL_srcA_simul[i] = tmp0;
      src_A_DRAM[i]    = short_to_float(tmp0);
    }

    printf("srcB init\n");
    for(size_t i=0; i<srcB_size; i++){
      float ramdom_value  = generate_random_255();
      // constant_value  = i%(256)+1;
      // constant_value  = i%(r_size)+1;
      // constant_value  = i;
      // constant_value  = i%(CLUSTER_SIZE*256)+1;
      constant_value  = 1;
      // constant_value  = (i%16 == 0) ? 0 : constant_value+1;
      // float ramdom_value  = 256;
      // short tmp0 = float_to_short(constant_value);
      short tmp0 = float_to_short(ramdom_value);
      PL_srcB_buf[i]   = tmp0;
      PL_srcB_simul[i] = tmp0;
      src_B_DRAM[i]    = short_to_float(tmp0);
    }    

    for(size_t i=0; i<dstC_size; i++){
      PL_dstC_buf[i] = 0;
      dst_C_DRAM[i] = 0;
    }

    short *PL_srcB_aligned_buf   = (short *)pim_malloc(srcB_size*sizeof(short), fpga_id);
    short *PL_srcB_aligned_simul = (short *)malloc(srcB_size*sizeof(short));
      if (PL_srcB_aligned_buf == NULL) {
        printf("PL srcB call failure.\n");
        return -1;
      }
      
    if(r_size > 256 * CH_NUM){ // TODO: Batch에 따라 B layout 변경
      is_need_align = true;
      for(size_t i=0; i<srcB_size; i++){
        PL_srcB_aligned_buf[i]=0;
      }
      pim_batch_align_chunk(PL_srcB_buf, PL_srcB_aligned_buf, p_size, q_size, r_size);
      pim_align_chunk(PL_srcB_simul, PL_srcB_aligned_simul, q_size, r_size);
    }
    short *simul_PL_srcB_buf = is_need_align? PL_srcB_aligned_simul : PL_srcB_simul;


/* ======= Argument parsing & set memory-mapping address  ======= */
    pim_args *set_info;
    int size = sizeof(pim_args);
    set_info = (pim_args *)malloc(sizeof(pim_args));

    set_info->srcA_va   = (uint64_t)&PL_srcA_buf[0];
    set_info->srcB_va   = is_need_align? (uint64_t)&PL_srcB_aligned_buf[0]:(uint64_t)&PL_srcB_buf[0];
    set_info->dstC_va   = (uint64_t)&PL_dstC_buf[0];
    set_info->p_size    = p_size;
    set_info->q_size    = q_size;
    set_info->r_size    = r_size;
    set_info->pmm_base_addr = (uint64_t)&pim_reg_map[0];


    mat_mul_CPU(src_A_DRAM, src_B_DRAM, dst_C_DRAM, p_size, q_size, r_size);
    matmul(set_info, fpga_id);  
    pim_matmul_simul(PL_srcA_simul, simul_PL_srcB_buf, PL_dstC_simul, p_size, q_size, r_size);

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
        //  printf("idx[%4d] 0x%x  |  ", i, a.u_val);
        //  printf("0x%x  |  ", (uint16_t)PL_dstC_buf[i]);
        //  printf("0x%x   |", (uint16_t)PL_dstC_simul[i]);
        //  printf("\n");        
        if(PL_dstC_buf[i] != PL_dstC_simul[i]) {          
          printf("idx[%4d] 0x%x  |  ", i, a.u_val);
          printf("0x%x  |  ", (uint16_t)PL_dstC_buf[i]);
          printf("0x%x   |", (uint16_t)PL_dstC_simul[i]);
          printf("\n"); 
          if (fail_flag == 0) first_fail_idx = i;       
          fail_flag = 1;
          break;
        }
        #endif
        //if (i == 32) // Only Bank 0
        //    break;        
    }

    // printf("             srcA      |    aligned srcA      \n");
    // printf("-------------------------------------------------\n");     
    // for(int i=0; i<srcA_size; i++){        
    //   printf("idx[%4d] 0x%x  |  ", i, (uint16_t)PL_srcA_buf[i]);
    //   printf("0x%x  |  ", (uint16_t)srcA_aligned_buf[i]);
    //   printf("\n"); 
    // }

    // printf("             srcA      |    vecA      \n");
    // printf("-------------------------------------------------\n");     
    // for(int i=0; i<srcA_size; i++){        
    //   printf("idx[%4d] 0x%x  |  ", i, (uint16_t)PL_srcA_buf[i]);
    //   printf("0x%x  |  ", (uint16_t)vecA_buf[i]);
    //   printf("\n"); 
    // }

    if (fail_flag) {
      printf("First fail at Idx[%d] \n", first_fail_idx);
      printf("PIM result and PIM SIMUL result always should be same.\n");
      printf("\nCorrectness check error !\n");
    }
    else {
      printf("Correctness check success !!!\n");
    }
    pim_free(PL_srcA_buf, fpga_id);
    pim_free(PL_srcB_buf, fpga_id);
    pim_free(PL_dstC_buf, fpga_id);
    pim_free(PL_srcB_aligned_buf, fpga_id);
    
    free(PL_srcA_simul);
    free(PL_srcB_simul);
    free(PL_srcB_aligned_simul);
    free(PL_dstC_simul);
    return 0;
}



