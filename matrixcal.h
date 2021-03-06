#ifndef MATRIXCAL_H
#define MATRIXCAL_H

#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "device_functions.h"
#include "algri.h"

#include "memorypool.h"
#include "memorypoolmanager.h"
#include "seed.h"
#include "RequestQueue.h"

#include <cublas_v2.h> //cuda�Դ��⺯��
//#include <cublas.h> 
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

#define DEVICENUM		12
#define DEVICEMEMORY		(1024*1024*1024)			 //1G

extern cublasHandle_t g_handle[6];
extern int8_t* g_device_matList[6];
extern uint8_t g_seed[32];
extern uint8_t g_results[32];
//extern SafeQueue<pTaskST> g_tskQueue[DEVICENUM];
// extern cTaskQueue g_tskQueue[DEVICENUM];
// extern cTaskQueue g_retQueue;
extern SafeQueue g_tskQueue[DEVICENUM];
extern SafeQueue g_retQueue;
extern SafeQueue g_tskQueue1;
extern SafeQueue g_tskQueue2;
extern SafeQueue g_tskQueue3;
extern SafeQueue g_tskQueue4;
extern SafeQueue g_tskQueue5;
extern SafeQueue g_tskQueue0;
#define LOOP_COUNT		2
#define SEQUENCE_COUNT	32

#define BLOCK_SIZE		256
#define THREAD_SIZE		256

extern void ret_callback(pTaskST tmpTask);

cudaError_t matrixMul(Mat256x256i8& sourceMatrix, const Mat256x256i8* tmpMatrix, const AlgriMatList* matList_int8, uint8_t *sequence, int8_t* threadID);

//void iter(const uint8_t *msg, uint32_t len, uint8_t result[32], uint32_t deviceID, Mat256x256i8 *res, Mat256x256i8 *mat, sha3_ctx *ctx);
void iter(uint8_t *msg,uint8_t *seed, uint32_t len, uint8_t result[32], uint32_t deviceID);

Words32 extSeedCreate(uint8_t *seed);
double GetMillsec();

void* calculate_Thread(void *arg);

#endif