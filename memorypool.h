/**************************************************
description:�ڴ������
author:zhoubin
time:2017-6-28
filename:MemoryPool.h
**************************************************/
#ifndef _MEMORYPOOL_H_  
#define _MEMORYPOOL_H_  
#include <stdlib.h>  

#ifdef __cplusplus
extern "C" {
#endif 

#define MINUNITSIZE 64  
#define ADDR_ALIGN 8  
#define SIZE_ALIGN MINUNITSIZE 

enum TYPE_MEMORY
{
    TYPE_IDLE = 0,
    TYPE_COMMON,
    TYPE_PERMANENT
};

enum MEMORYPOOL_TYPE
{
    TYPE_GPU = 1,
    TYPE_CPU = 2
};


typedef struct memory_block  
{  
    size_t count;  
    size_t start;
    short type;
}memory_block;   


// �ڴ�ؽṹ��   
typedef struct MEMORYPOOL  
{  
    void *memory;  
    size_t size; 
    int memory_type;                //memory_type
    memory_block* pmem_map;         //block����ͷָ��
    memory_block* pmem_map_end;     //block����βָ��
    size_t mem_used_size; // ��¼�ڴ�����Ѿ�������û����ڴ�Ĵ�С  
    size_t mem_map_pool_count; // ��¼����Ԫ�������ʣ��ĵ�Ԫ�ĸ���������Ϊ0ʱ���ܷ��䵥Ԫ��pfree_mem_chunk  
    size_t mem_chunk_count; // ��¼ pfree_mem_chunk�����еĵ�Ԫ����  
    size_t mem_block_count; // һ�� mem_unit ��СΪ MINUNITSIZE  
}MEMORYPOOL, *PMEMORYPOOL;

/************************************************************************/  
/* �����ڴ�� 
 * sBufSize: �ڴ�ؿ��ô�С
 * �������ɵ��ڴ��ָ�� 
/************************************************************************/ 
PMEMORYPOOL _CreateMemoryPool(int index, size_t sBufSize, int memoryType);  


/************************************************************************/  
/* ��ʱû�� 
/************************************************************************/   
void _ReleaseMemoryPool(PMEMORYPOOL ppMem);   

/************************************************************************/  
/* ���ڴ���з���ָ����С���ڴ�  
 * pMem: �ڴ�� ָ�� 
 * sMemorySize: Ҫ������ڴ��С 
 * �ɹ�ʱ���ط�����ڴ���ʼ��ַ��ʧ�ܷ���NULL 
/************************************************************************/  
//void* GetMemory(size_t sMemorySize, PMEMORYPOOL pMem);  
void* _GetMemory(size_t sMemorySize, PMEMORYPOOL pMem, TYPE_MEMORY type = TYPE_COMMON);
/************************************************************************/  
/* ���ڴ�����ͷ����뵽���ڴ� 
 * pMem���ڴ��ָ�� 
 * ptrMemoryBlock�����뵽���ڴ���ʼ��ַ 
/************************************************************************/  
int _FreeMemory(void *ptrMemoryBlock, PMEMORYPOOL pMem);  

/************************************************************************/  
/* 
�ڴ�ӳ����е�����ת��Ϊ�ڴ���ʼ��ַ                                           
                          
/************************************************************************/ 
void* index2addr(PMEMORYPOOL mem_pool, size_t index);

void cleanMemoryPool(int ,PMEMORYPOOL mem_pool);


#define    POOL_WALK(pool, iterator) \
    for(iterator = pool->pmem_map; (iterator != NULL && (iterator-pool->pmem_map)+(iterator->count) < pool->mem_block_count); (iterator = &(pool->pmem_map[((iterator-pool->pmem_map)+(iterator->count))])))


#ifdef __cplusplus
}
#endif 

#endif //_MEMORYPOOL_H 
