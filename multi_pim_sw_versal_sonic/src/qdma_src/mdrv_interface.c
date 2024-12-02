#include "pim_qdma.h"

// static pthread_mutex_t qdma_lock = PTHREAD_MUTEX_INITIALIZER;

// char* devname;
// int qdma_fd;

extern struct pl_dma_t *pl_dma;

void close_qdma()
{
    if(qdma == NULL) return;
    
    atexit(qdma_env_cleanup);
    for (int i = 0; i < totalDevNum; i++) {
        pthread_mutex_destroy(&qdma[i].lock);
        close(qdma[i].fd);
    }    
}

void init_qdma()
{    
    if(qdma != NULL) return;    

    qdma = (struct qdma_t*)malloc(sizeof(struct qdma_t)*totalDevNum);
    printf("TOTAL FPGA NUM: %d\n", totalDevNum);
    get_qdma_bus_list(qdma);

    for (int i = 0; i < totalDevNum; i++) {
        pci_bus = qdma[i].bus_num;
        qdma_validate_qrange();

        q_count = 0;
        /* Addition and Starting of queues handled here */
        q_count = qdma_setup_queues(&q_info);
        if (q_count < 0) {
            printf("qdma_setup_queues failed, ret:%d\n", q_count);
            return q_count; 
        }

        /*Current queue status dump */
        printf("Queue Information:\n");
        printf("  Device Name: %s\n", q_info->q_name);
        // printf("  ID: %d\n", q_info->qid);
        // printf("  PF: %d\n", q_info->pf);

        /*First, H2C & C2H check*/
        qdma[i].devname = q_info->q_name;
        qdma[i].fd = open(qdma[i].devname, O_RDWR);
        if (qdma[i].fd < 0) {
	    	fprintf(stderr, "unable to open device %s, %d.\n",
	    		qdma[i].devname, qdma[i].fd);
	    	perror("open device");
            return;
	    }

        /* Mutual execlusion of QDMA in each FPGA board */
        struct qdma_t* qdma_inst;
        qdma_inst = &qdma[i];
        pthread_mutex_init(&qdma_inst->lock, NULL);
    }       
    
    atexit(close_qdma);
}

int datacopy_cpu2pim(void *pim_dst, const void *cpu_src, int size, int fpga_id)
{
    // struct qdma_t* qdma_inst;
    // qdma_inst = &qdma[fpga_id];
    // pthread_mutex_lock(&qdma_inst->lock);
    // struct pl_dma_t *pl_dma_node;
    // pl_dma_node = &pl_dma[fpga_id];
    // pthread_mutex_lock(&pl_dma_node->lock);
#ifdef DEBUG_PIM_THREAD
    printf("qdma cpu2pim start(fpga_id: %d, tid: %d) [%x ~ %x]\n", fpga_id, gettid(), VA2PA(pim_dst, fpga_id), VA2PA(pim_dst, fpga_id)+size);
#endif

#ifdef QDMA_SPLIT
    int Quotient = size / HUGE_PAGE_SIZE;
    int Remainder = size % HUGE_PAGE_SIZE;
    ssize_t ret;

    for (int i = 0; i < Quotient; i++) {
        ret = write_from_buffer(qdma[fpga_id].devname, qdma[fpga_id].fd, (void*)((char*)cpu_src + HUGE_PAGE_SIZE*i), HUGE_PAGE_SIZE, VA2PA(pim_dst + HUGE_PAGE_SIZE*i, fpga_id)); /*Host -> Card*/
        if (ret < 0) {
            printf("Data copy cpu2pim failed\n");
            close(qdma[fpga_id].fd);        
            // pthread_mutex_unlock(&qdma_inst->lock);
            pthread_mutex_unlock(&pl_dma_node->lock);
            return ret;
        }
    }
    if (Remainder > 0) {
        ret = write_from_buffer(qdma[fpga_id].devname, qdma[fpga_id].fd, (void*)((char*)cpu_src + HUGE_PAGE_SIZE*Quotient), Remainder, VA2PA(pim_dst + HUGE_PAGE_SIZE*Quotient, fpga_id)); /*Host -> Card*/
        if (ret < 0) {
            printf("Data copy cpu2pim failed\n");
            close(qdma[fpga_id].fd);        
            // pthread_mutex_unlock(&qdma_inst->lock);
            pthread_mutex_unlock(&pl_dma_node->lock);
            return ret;
        }
    }
#else
    ssize_t ret;
    ret = write_from_buffer(qdma[fpga_id].devname, qdma[fpga_id].fd, cpu_src, size, VA2PA(pim_dst, fpga_id)); /*Host -> Card*/
    if (ret < 0) {
        printf("Data copy cpu2pim failed\n");
        close(qdma[fpga_id].fd);        
    }
#endif

#ifdef DEBUG_PIM_THREAD
    printf("qdma cpu2pim end(fpga_id: %d, tid: %d) [%x ~ %x]\n", fpga_id, gettid(), VA2PA(pim_dst, fpga_id), VA2PA(pim_dst, fpga_id)+size);
#endif
    // pthread_mutex_unlock(&qdma_inst->lock);
    // pthread_mutex_unlock(&pl_dma_node->lock);
    return ret;
}

int datacopy_pim2cpu(void *cpu_dst, const void *pim_src, int size, int fpga_id)
{
    // struct qdma_t* qdma_inst;
    // qdma_inst = &qdma[fpga_id];
    // pthread_mutex_lock(&qdma_inst->lock);
    // struct pl_dma_t *pl_dma_node;
    // pl_dma_node = &pl_dma[fpga_id];
    // pthread_mutex_lock(&pl_dma_node->lock);
#ifdef DEBUG_PIM_THREAD
    printf("qdma pim2cpu start(fpga_id: %d, tid: %d) [%x ~ %x]\n", fpga_id, gettid(), VA2PA(pim_src, fpga_id), VA2PA(pim_src, fpga_id)+size);
#endif

#ifdef QDMA_SPLIT
    int Quotient = size / HUGE_PAGE_SIZE;
    int Remainder = size % HUGE_PAGE_SIZE;
    ssize_t ret;

    for (int i = 0; i < Quotient; i++) {
        ret = read_to_buffer(qdma[fpga_id].devname, qdma[fpga_id].fd, (void*)((char*)cpu_dst + HUGE_PAGE_SIZE*i), HUGE_PAGE_SIZE, VA2PA(pim_src + HUGE_PAGE_SIZE*i, fpga_id)); /*Host -> Card*/
        if (ret < 0) {
            printf("Data copy pim2cpu failed\n");
            close(qdma[fpga_id].fd);  
            // pthread_mutex_unlock(&qdma_inst->lock);
            pthread_mutex_unlock(&pl_dma_node->lock);
            return ret;
        }
    }
    if (Remainder > 0) {
        ret = read_to_buffer(qdma[fpga_id].devname, qdma[fpga_id].fd, (void*)((char*)cpu_dst + HUGE_PAGE_SIZE*Quotient), Remainder, VA2PA(pim_src + HUGE_PAGE_SIZE*Quotient, fpga_id)); /*Host -> Card*/
        if (ret < 0) {
            printf("Data copy pim2cpu failed\n");
            close(qdma[fpga_id].fd);     
            // pthread_mutex_unlock(&qdma_inst->lock);   
            pthread_mutex_unlock(&pl_dma_node->lock);   
            return ret;
        }
    }
#else
    ssize_t ret;
    ret = read_to_buffer(qdma[fpga_id].devname, qdma[fpga_id].fd, cpu_dst, size, VA2PA(pim_src, fpga_id)); /*Card -> Host*/
    if (ret < 0) {
        printf("Data copy pim2cpu failed\n");
        close(qdma[fpga_id].fd);
    }    
#endif

#ifdef DEBUG_PIM_THREAD
    printf("qdma pim2cpu end(fpga_id: %d, tid: %d) [%x ~ %x]\n", fpga_id, gettid(), VA2PA(pim_src, fpga_id), VA2PA(pim_src, fpga_id)+size);
#endif
    // pthread_mutex_unlock(&qdma_inst->lock);
    // pthread_mutex_unlock(&pl_dma_node->lock);
    return ret;
}

// int datacopy_pim2pim(void *pim_dst, const void *pim_src, int size, int dst_id) {
int datacopy_pim2pim(void *pim_dst, const void *pim_src, int size, int dst_id, int src_id) {    
    // printf("NO PIM 2 PIM COPY!!!!\n");
    // exit(-1);
    // void *buf = (void*)malloc(size);    
    void* buf = (void*)mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    ssize_t ret;
    // pthread_mutex_lock(&qdma[src_id].lock);
#ifdef DEBUG_PIM_THREAD
    printf("qdma pim2pim start(fpga_id: %d, tid: %d) [%x ~ %x]\n", src_id, gettid(), VA2PA(pim_src, src_id), VA2PA(pim_src, src_id)+size);
#endif

#ifdef QDMA_SPLIT
    ret = datacopy_pim2cpu(buf, pim_src, size, src_id);
#else
    ret = read_to_buffer(qdma[src_id].devname, qdma[src_id].fd, buf, size, VA2PA(pim_src, src_id));
#endif
    // pthread_mutex_unlock(&qdma[src_id].lock);
    if (ret < 0) {
        printf("Data copy pim2pim(1/2) failed\n");
        close(qdma[src_id].fd);        
        free(buf);
        return ret;
    }

    // pthread_mutex_lock(&qdma[dst_id].lock);
#ifdef QDMA_SPLIT
    ret = datacopy_cpu2pim(pim_dst, buf, size, dst_id);
#else
    ret = write_from_buffer(qdma[dst_id].devname, qdma[dst_id].fd, buf, size, VA2PA(pim_dst, dst_id));
#endif
#ifdef DEBUG_PIM_THREAD
    printf("qdma pim2pim end(fpga_id: %d, tid: %d) [%x ~ %x]\n", dst_id, gettid(), VA2PA(pim_dst, dst_id), VA2PA(pim_dst, dst_id)+size);
#endif    
    // pthread_mutex_unlock(&qdma[dst_id].lock);
    if (ret < 0) {
        printf("Data copy pim2pim(2/2) failed\n");
        close(qdma[dst_id].fd);        
    }

    // free(buf);
    munmap(buf, size);
    return ret;
}

#ifdef QDMA
int datacopy_desclist(const void *cpu_src, int size, int fpga_id)
{    
    ssize_t ret;
    ret = write_from_buffer(qdma[fpga_id].devname, qdma[fpga_id].fd, cpu_src, size, 0x800000); /*Host -> Card*/
    if (ret < 0) {
        printf("Data copy cpu2pim failed\n");
        close(qdma[fpga_id].fd);        
    }
    return ret;
}
#endif