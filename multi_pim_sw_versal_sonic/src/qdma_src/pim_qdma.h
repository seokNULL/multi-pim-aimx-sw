#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <getopt.h>
#include <err.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include "version.h"
#include "dmautils.h"
#include "qdma_nl.h"
#include "dmaxfer.h"

#include "../../include/pim.h"
#include "../pim_math_src/pim_math.h"
#include "../drv_src/pim_mem_lib_user.h"

struct qdma_t {
    char* devname;
    int fd;
	unsigned int bus_num;
	pthread_mutex_t lock;
};

static struct qdma_t* qdma;
static unsigned int pci_bus;
static unsigned int pci_dev = 0x00;
static int fun_id = 0x0;
static unsigned int num_q = 1; //Board number
static int is_vf = 0;

/*QDMA related information*/
#define QDMA_Q_NAME_LEN     100
struct queue_info *q_info;
int q_count;

enum qdma_q_dir {
	QDMA_Q_DIR_H2C,
	QDMA_Q_DIR_C2H,
	QDMA_Q_DIR_BIDI
};

enum qdma_q_mode {
	QDMA_Q_MODE_MM,
	QDMA_Q_MODE_ST
};

struct queue_info {
	char *q_name;
	int qid;
	int pf;
	enum qdmautils_io_dir dir;
};

enum qdma_q_mode mode = QDMA_Q_MODE_MM;
enum qdma_q_dir dir = QDMA_Q_DIR_BIDI;

int qdma_validate_qrange(void);
void qdma_q_prep_name(struct queue_info *q_info, int qid, int pf);

int qdma_prepare_queue(struct queue_info *q_info, enum qdmautils_io_dir dir, int qid, int pf);
int qdma_prepare_q_start(struct xcmd_info *xcmd, enum qdmautils_io_dir dir, int qid, int pf);
int qdma_prepare_q_del(struct xcmd_info *xcmd, enum qdmautils_io_dir dir, int qid, int pf);
int qdma_prepare_q_add(struct xcmd_info *xcmd, enum qdmautils_io_dir dir, int qid, int pf);
int qdma_prepare_q_stop(struct xcmd_info *xcmd, enum qdmautils_io_dir dir, int qid, int pf);
int qdma_create_queue(enum qdmautils_io_dir dir, int qid, int pf);
void qdma_env_cleanup();
void qdma_queues_cleanup(struct queue_info *q_info, int q_count);
int qdma_destroy_queue(enum qdmautils_io_dir dir, int qid, int pf);
int qdma_setup_queues(struct queue_info **pq_info);
int qdma_dump_queue(struct queue_info **pq_info);

#define MAX_QDMA 10 // 최대 QDMA 수

typedef struct {
    char id[3]; // QDMA ID (최대 2자리 + 널 종료)
} QDMA_Info;

static QDMA_Info qdma_list[MAX_QDMA];

static void extract_qdma_ids(const char *input) {
    int count = 0;
    char *line;
    char *input_copy = strdup(input); // 원본 문자열 복사

    // 문자열을 줄 단위로 나누기
    line = strtok(input_copy, "\n");
    while (line != NULL) {
        if (count >= MAX_QDMA) {
            break; // 최대 수치에 도달하면 종료
        }
        
        // "qdma" 다음의 2자리 숫자 추출
        char qdma_id[3]; // QDMA ID를 저장할 배열
        if (sscanf(line, "qdma%2s", qdma_id) == 1) {
            strncpy(qdma_list[count].id, qdma_id, 2);
            qdma_list[count].id[2] = '\0'; // null 종료
            count++;
        }

        line = strtok(NULL, "\n"); // 다음 줄로 이동
    }
    
    free(input_copy); // 메모리 해제
    // return count; // 추출된 QDMA 수 반환
}

void get_qdma_bus_list(struct qdma_t* qdma) {
	struct xcmd_info xcmd;
	memset(&xcmd, 0, sizeof(xcmd));
	xcmd.op = XNL_CMD_DEV_LIST;	
	xcmd.log_msg_dump = extract_qdma_ids;
	qdma_dev_list_dump(&xcmd);
	for (int i = 0; i < totalDevNum; i++) {    		
		// if (i==4) qdma[i].bus_num = strtoul(qdma_list[5].id, NULL, 16);
		// else if (i==5) qdma[i].bus_num = strtoul(qdma_list[6].id, NULL, 16);
		// else if (i==6) qdma[i].bus_num = strtoul(qdma_list[7].id, NULL, 16);
		// else qdma[i].bus_num = strtoul(qdma_list[i].id, NULL, 16);		
		// qdma[i].bus_num = strtoul(qdma_list[i+1].id, NULL, 16);		
		qdma[i].bus_num = strtoul(qdma_list[i].id, NULL, 16);		
    }
}

int qdma_validate_qrange(void){
	struct xcmd_info xcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	xcmd.op = XNL_CMD_DEV_INFO;
	xcmd.if_bdf = (pci_bus << 12) | (pci_dev << 4) | fun_id;	

	/* Get dev info from qdma driver */
	ret = qdma_dev_info(&xcmd);
	if (ret < 0) {
		printf("Failed to read qmax for PF: %d\n", fun_id);
		return ret;
	}
	if (!xcmd.resp.dev_info.qmax) {
		printf("Error: invalid qmax assigned to function :%d qmax :%u\n",
				fun_id, xcmd.resp.dev_info.qmax);
		return -EINVAL;
	}

	return 0;
}

int qdma_prepare_queue(struct queue_info *q_info, enum qdmautils_io_dir dir, int qid, int pf){
	int ret;

	if (!q_info) {
		printf("Error: Invalid queue info\n");
		return -EINVAL;
	}

	qdma_q_prep_name(q_info, qid, pf);
	q_info->dir = dir;
	ret = qdma_create_queue(q_info->dir, qid, pf);
	if (ret < 0) {
		printf("Q creation Failed PF:%d QID:%d\n",
				pf, qid);
		return ret;
	}
	q_info->qid = qid;
	q_info->pf = pf;

	return ret;
}

void qdma_q_prep_name(struct queue_info *q_info, int qid, int pf){
	q_info->q_name = calloc(QDMA_Q_NAME_LEN, 1);
	snprintf(q_info->q_name, QDMA_Q_NAME_LEN, "/dev/qdma%s%05x-%s-%d",
			(is_vf) ? "vf" : "",
			(pci_bus << 12) | (pci_dev << 4) | pf,
			(mode == QDMA_Q_MODE_MM) ? "MM" : "ST",
			qid);
}

int qdma_prepare_q_start(struct xcmd_info *xcmd, enum qdmautils_io_dir dir, int qid, int pf){
	struct xcmd_q_parm *qparm;


	if (!xcmd) {
		printf("Error: Invalid Input Param\n");
		return -EINVAL;
	}
	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_Q_START;
	xcmd->vf = is_vf; 
	xcmd->if_bdf = (pci_bus << 12) | (pci_dev << 4) | pf;
	qparm->idx = qid;
	qparm->num_q = 1;

	if (mode == QDMA_Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (mode == QDMA_Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else {
		printf("Error: Invalid mode\n");
		return -EINVAL;	
	}

	if (dir == DMAXFER_IO_WRITE)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (dir == DMAXFER_IO_READ)
		qparm->flags |= XNL_F_QDIR_C2H;
	else {
		printf("Error: Invalid Direction\n");
		return -EINVAL;	
	}

	qparm->flags |= (XNL_F_CMPL_STATUS_EN | XNL_F_CMPL_STATUS_ACC_EN |
			XNL_F_CMPL_STATUS_PEND_CHK | XNL_F_CMPL_STATUS_DESC_EN |
			XNL_F_FETCH_CREDIT);

	return 0;
}

int qdma_prepare_q_del(struct xcmd_info *xcmd, enum qdmautils_io_dir dir, int qid, int pf){
	struct xcmd_q_parm *qparm;

	if (!xcmd) {
		printf("Error: Invalid Input Param\n");
		return -EINVAL;
	}

	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_Q_DEL;
	xcmd->vf = is_vf; 
	xcmd->if_bdf = (pci_bus << 12) | (pci_dev << 4) | pf;
	qparm->idx = qid;
	qparm->num_q = 1;

	if (mode == QDMA_Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (mode == QDMA_Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else {
		printf("Error: Invalid mode\n");
		return -EINVAL;	
	}

	if (dir == DMAXFER_IO_WRITE)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (dir == DMAXFER_IO_READ)
		qparm->flags |= XNL_F_QDIR_C2H;
	else {
		printf("Error: Invalid Direction\n");
		return -EINVAL;	
	}

	return 0;
}

int qdma_create_queue(enum qdmautils_io_dir dir, int qid, int pf){
	struct xcmd_info xcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	ret = qdma_prepare_q_add(&xcmd, dir, qid, pf);
	if (ret < 0)
		return ret;

	ret = qdma_q_add(&xcmd);
	if (ret < 0) {
		printf("Q_ADD failed, ret :%d\n", ret);
		return ret;
	}

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	ret = qdma_prepare_q_start(&xcmd, dir, qid, pf);
	if (ret < 0)
		return ret;

	ret = qdma_q_start(&xcmd);
	if (ret < 0) {
		printf("Q_START failed, ret :%d\n", ret);
		qdma_prepare_q_del(&xcmd, dir, qid, pf);
		qdma_q_del(&xcmd);
	}

	return ret;
}

int qdma_prepare_q_add(struct xcmd_info *xcmd, enum qdmautils_io_dir dir, int qid, int pf){
	struct xcmd_q_parm *qparm;

	if (!xcmd) {
		printf("Error: Invalid Input Param\n");
		return -EINVAL;
	}

	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_Q_ADD;
	xcmd->vf = is_vf; 
	xcmd->if_bdf = (pci_bus << 12) | (pci_dev << 4) | pf;
	qparm->idx = qid;
	qparm->num_q = 1;

	if (mode == QDMA_Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (mode == QDMA_Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else {
		printf("Error: Invalid mode\n");
		return -EINVAL;	
	}
	if (dir == DMAXFER_IO_WRITE)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (dir == DMAXFER_IO_READ)
		qparm->flags |= XNL_F_QDIR_C2H;
	else {
		printf("Error: Invalid Direction\n");
		return -EINVAL;	
	}
	qparm->sflags = qparm->flags;

	return 0;
}

int qdma_setup_queues(struct queue_info **pq_info){
	struct queue_info *q_info;
	unsigned int qid;
	unsigned int q_count;
	unsigned int q_index;
	int ret;

	if (!pq_info) {
		printf("Error: Invalid queue info\n");
		return -EINVAL;
	}

	if (dir == QDMA_Q_DIR_BIDI)
		q_count = num_q * 2;
	else	
		q_count = num_q;

	*pq_info = q_info = (struct queue_info *)calloc(q_count, sizeof(struct queue_info));
	if (!q_info) {
		printf("Error: OOM\n");
		return -ENOMEM;
	}

	q_index = 0;
	for (qid = 0; qid < num_q; qid++) {
		if ((dir == QDMA_Q_DIR_BIDI) ||
				(dir == QDMA_Q_DIR_H2C)) {
			ret = qdma_prepare_queue(q_info + q_index,
					DMAXFER_IO_WRITE,
					qid,
					fun_id);
			if (ret < 0)
				break;
			q_index++;
		}
		if ((dir == QDMA_Q_DIR_BIDI) ||
				(dir == QDMA_Q_DIR_C2H)) {
			ret = qdma_prepare_queue(q_info + q_index,
				DMAXFER_IO_READ,
					qid,
					fun_id);
			if (ret < 0)
				break;
			q_index++;
		}
	}
	if (ret < 0) {
		// qdma_queues_cleanup(q_info, q_index);
		return ret;
	}

	return q_count; 
}

void qdma_env_cleanup(){
	for (int i = 0; i < totalDevNum; i++) {
		pci_bus = qdma[i].bus_num;
		qdma_queues_cleanup(q_info, q_count);
	}

	if (q_info)
		free(q_info);
	q_info = NULL;
	q_count = 0;
}

void qdma_queues_cleanup(struct queue_info *q_info, int q_count){
	unsigned int q_index;

	if (!q_info || q_count < 0)
		return;

	for (q_index = 0; q_index < q_count; q_index++) {
		qdma_destroy_queue(q_info[q_index].dir,
				q_info[q_index].qid,
				q_info[q_index].pf);
		// free(q_info[q_index].q_name);
	}
}

int qdma_destroy_queue(enum qdmautils_io_dir dir, int qid, int pf){
	struct xcmd_info xcmd;
	int ret;

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	ret = qdma_prepare_q_stop(&xcmd, dir, qid, pf);
	if (ret < 0)
		return ret;	

	ret = qdma_q_stop(&xcmd);
	if (ret < 0)
		printf("Q_STOP failed, ret :%d\n", ret);

	memset(&xcmd, 0, sizeof(struct xcmd_info));
	qdma_prepare_q_del(&xcmd, dir, qid, pf);
	ret = qdma_q_del(&xcmd);
	if (ret < 0)
		printf("Q_DEL failed, ret :%d\n", ret);

	return ret;
}

int qdma_prepare_q_stop(struct xcmd_info *xcmd, enum qdmautils_io_dir dir, int qid, int pf){
	struct xcmd_q_parm *qparm;

	if (!xcmd)
		return -EINVAL;

	qparm = &xcmd->req.qparm;

	xcmd->op = XNL_CMD_Q_STOP;
	xcmd->vf = is_vf; 
	xcmd->if_bdf = (pci_bus << 12) | (pci_dev << 4) | pf;
	qparm->idx = qid;
	qparm->num_q = 1;

	if (mode == QDMA_Q_MODE_MM)
		qparm->flags |= XNL_F_QMODE_MM;
	else if (mode == QDMA_Q_MODE_ST)
		qparm->flags |= XNL_F_QMODE_ST;
	else
		return -EINVAL;	

	if (dir == DMAXFER_IO_WRITE)
		qparm->flags |= XNL_F_QDIR_H2C;
	else if (dir == DMAXFER_IO_READ)
		qparm->flags |= XNL_F_QDIR_C2H;
	else
		return -EINVAL;	


	return 0;
}
