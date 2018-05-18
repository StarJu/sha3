#include <memory.h>  
#include "memorypool.h"  
#include <stdio.h>
#include <cuda_runtime.h>
#include <cuda.h>

/************************************************************************/  
/* �ڴ����ʼ��ַ���뵽ADDR_ALIGN�ֽ� 
/************************************************************************/ 
size_t check_align_addr(void*& pBuf)  
{  
    size_t align = 0;  
    size_t addr = (size_t)pBuf;  
    align = (ADDR_ALIGN - addr % ADDR_ALIGN) % ADDR_ALIGN;  
    pBuf = (char*)pBuf + align;  
    return align;  
}  

/************************************************************************/  
/* �ڴ�block��С���뵽MINUNITSIZE�ֽ� 
/************************************************************************/  
size_t check_align_block(size_t size)  
{  
    size_t align = size % MINUNITSIZE;  
      
    return size - align;   
}

/************************************************************************/  
/* �����ڴ��С���뵽SIZE_ALIGN�ֽ� 
/************************************************************************/  
size_t check_align_size(size_t size)  
{  
    size = (size + SIZE_ALIGN - 1) / SIZE_ALIGN * SIZE_ALIGN;  
    return size;  
}

/************************************************************************/  
/* ���� MINUNITSIZE�ֽ� ��SIZE_ALIGN�ֽ� ����У���ڴ�������С
/************************************************************************/ 
size_t check_memory_size(size_t size)
{
	size_t endsize = size;
	
	endsize += (size%SIZE_ALIGN);
	endsize += (endsize%SIZE_ALIGN);
	
	printf("check_memory_size:size[%d], end_size[%d].\n",size, endsize);
    return endsize;
}

/************************************************************************/  
/* 
�ڴ�ӳ����е�����ת��Ϊ�ڴ���ʼ��ַ                                           
                          
/************************************************************************/ 
void* index2addr(PMEMORYPOOL mem_pool, size_t index)  
{  
    char* p = (char*)(mem_pool->memory);  
    void* ret = (void*)(p + index *MINUNITSIZE);  
      
    return ret;  
}  

/************************************************************************/  
/* 
�ڴ���ʼ��ַת��Ϊ�ڴ�ӳ����е�����                                           
                          
/************************************************************************/ 
size_t addr2index(PMEMORYPOOL mem_pool, void* addr)  
{  
    char* start = (char*)(mem_pool->memory);  
    char* p = (char*)addr;  
    size_t index = (p - start) / MINUNITSIZE;  
    return index;  
}

/************************************************************************/  
/* �����ڴ�� 
 * sBufSize: �ڴ�ؿ��ô�С
 * �������ɵ��ڴ��ָ�� 
/************************************************************************/ 
PMEMORYPOOL _CreateMemoryPool(int index, size_t sBufSize, int memoryType)
{
	
	printf("create memory pool \n");
    //�����ڴ�ֵָ��
	PMEMORYPOOL mem_pool = (PMEMORYPOOL)malloc(sizeof(MEMORYPOOL));
	if (NULL == mem_pool)
	{
		printf("[%s %d|Error] CreateMemoryPool failed. can not malloc mem_pool point. size[%d]\n",__FILE__,__LINE__,sizeof(MEMORYPOOL));
		return NULL;
	}

	//�����С
	mem_pool->size = check_memory_size(sBufSize);
    mem_pool->memory_type = memoryType;
    printf("_CreateMemoryPool:size[%ld], end size[%ld].\n",sBufSize, mem_pool->size);
	mem_pool->mem_block_count = mem_pool->size / MINUNITSIZE;
	mem_pool->mem_map_pool_count = mem_pool->mem_block_count;
	//mem_pool->mem_map_unit_count = mem_pool->mem_block_count;

	//��ʼ��pmem_map
	mem_pool->pmem_map = (memory_block*)malloc(sizeof(memory_block) * mem_pool->mem_block_count);   //�����ǰָ��
    if (NULL == mem_pool->pmem_map)
    {
        printf("[%s %d|Error] CreateMemoryPool failed. can not malloc mem_pool->pmem_map point. size[%d]\n",__FILE__,__LINE__, sizeof(memory_block) * mem_pool->mem_block_count);
        free(mem_pool);
		return NULL;
    }

    memset(mem_pool->pmem_map,0,sizeof(memory_block) * mem_pool->mem_block_count);
	mem_pool->pmem_map_end = &(mem_pool->pmem_map[mem_pool->mem_block_count-1]);     //block�����βָ��
    mem_pool->pmem_map[0].count = mem_pool->mem_block_count;             //chunk��block�ĸ���
    mem_pool->pmem_map[0].type = TYPE_IDLE;                                 //����
    //mem_pool->pmem_map[0].pos = 0;
    mem_pool->pmem_map[mem_pool->mem_block_count-1].start = 0;    //chunk�����һ��block�Ŀ�ʼλ��

    mem_pool->mem_chunk_count = 1;                           //���õ�chunk����
	mem_pool->mem_used_size = 0;                                  //�ڴ����Ѿ�ʹ�õ��ڴ��С

    
    // mem_pool->memory = (char *)malloc(mem_pool->size);           //������ʼָ��
    if (mem_pool->memory_type == TYPE_GPU)
    {
    	cudaSetDevice(index);
    	cudaError_t r = cudaMalloc((void**)&(mem_pool->memory), mem_pool->size);				//�����Դ�
    	if(r != cudaSuccess)
    	{
    	    free(mem_pool->pmem_map);
            free(mem_pool);
        	return NULL;
    	}
    }
    else
    {
        mem_pool->memory = (char *)malloc(mem_pool->size); 
		memset(mem_pool->memory, 0, mem_pool->size);
    }
	
    if (NULL == mem_pool->memory)
    {
        printf("[%s %d|Error] CreateMemoryPool failed. can not malloc mem_pool->memory point. size[%ld]\n",__FILE__,__LINE__, mem_pool->size);
        free(mem_pool->pmem_map);
        free(mem_pool);
		return NULL;
    }
	printf("create is over\n");
	
    return mem_pool;
}


void cleanMemoryPool(int num, PMEMORYPOOL mem_pool)
{
	memset(mem_pool->pmem_map, 0, sizeof(memory_block) * mem_pool->mem_block_count);
	mem_pool->pmem_map_end = &(mem_pool->pmem_map[mem_pool->mem_block_count - 1]);     //block�����βָ��
	mem_pool->pmem_map[0].count = mem_pool->mem_block_count;             //chunk��block�ĸ���
	mem_pool->pmem_map[0].type = TYPE_IDLE;                                 //����
	//mem_pool->pmem_map[0].pos = 0;
	mem_pool->pmem_map[mem_pool->mem_block_count - 1].start = 0;    //chunk�����һ��block�Ŀ�ʼλ��

	mem_pool->mem_chunk_count = 1;                           //���õ�chunk����
	mem_pool->mem_used_size = 0;                                  //�ڴ����Ѿ�ʹ�õ��ڴ��С
    if (mem_pool->memory_type == TYPE_GPU)
    {
    	cudaError_t r = cudaMemset((void**)&(mem_pool->memory),0x00,  mem_pool->size);				//�����Դ�
    	if (r != cudaSuccess)
    	{
    		return;
    	}
    }
    else
    {
        memset(mem_pool->memory, 0x00, mem_pool->size);
    }
}

/************************************************************************/  
/* ��ʱû�� 
/************************************************************************/  
void _ReleaseMemoryPool(PMEMORYPOOL ppMem)   
{  
}  

/************************************************************************/  
/* ���ڴ���з���ָ����С���ڴ�  
* pMem: �ڴ�� ָ�� 
* sMemorySize: Ҫ������ڴ��С 
* �ɹ�ʱ���ط�����ڴ���ʼ��ַ��ʧ�ܷ���NULL 
/************************************************************************/  
void* _GetMemory(size_t sMemorySize, PMEMORYPOOL pMem, TYPE_MEMORY type)
{
    if (NULL == pMem)
    {
        return NULL;
    }
    
    //У׼�ڴ�
    sMemorySize = check_align_size(sMemorySize);  
    
    size_t index = 0;
    //�����Ҳ���
    if (type == TYPE_COMMON)
    {
        memory_block* temp = pMem->pmem_map;
        for (index = 0; index < pMem->mem_chunk_count; index++)
        {
            if (temp->count * MINUNITSIZE >= sMemorySize && temp->type == TYPE_IDLE)  
            {             
                break;  
            }  

            //����������һ�����ƶ�����һ���ڴ��
            if (index < pMem->mem_chunk_count-1)
            {
                temp = (memory_block*)((char *)temp + (temp->count)*sizeof(memory_block));
            }
        }

        if (index == pMem->mem_chunk_count)  
        {  
            printf("GetMemory error. not enough memory. size[%ld]\n", sMemorySize);
            return NULL;  
        }

        //����ͳ�����
        pMem->mem_used_size += sMemorySize;
        size_t needcount = sMemorySize/MINUNITSIZE;
        pMem->mem_map_pool_count -= needcount;

        //����ƫ����
        size_t current_index = temp-pMem->pmem_map;
        
        //��Ҫ�и�����
        if (temp->count > needcount)
        {
            //�޸�β
            pMem->pmem_map[current_index+needcount-1].start = current_index;

            //�޸���һ��chunk
            pMem->pmem_map[current_index+needcount].count = temp->count-needcount;
            //pMem->pmem_map[current_index+needcount].type = TYPE_IDLE;   //�ʼ�Ѿ���ʼ������ʱ���ε�
            pMem->pmem_map[current_index+temp->count-1].start = current_index+needcount;

            //�޸ĵ�ǰtemp
            temp->count = needcount;

            //����ڴ�type
            temp->type = TYPE_COMMON;

            //�޸�chunk����
            pMem->mem_chunk_count++;
            
        //���и�����
        }
        else if (temp->count == needcount)
        {
            temp->type = TYPE_COMMON;
        }

        //�������ݿ������ǵ�ַ
        return index2addr(pMem, current_index);
        
    }
    //��������
    else if (type == TYPE_PERMANENT)
    {
        memory_block* temp = pMem->pmem_map_end;
        for (index = 0; index < pMem->mem_chunk_count; index++)
        {
            //��ȡchunkͷ
            memory_block* start = &(pMem->pmem_map[temp->start]);
            if (start->count * MINUNITSIZE >= sMemorySize && start->type == TYPE_IDLE)  
            {             
                break;  
            }  

            //���������ǰһ�����ƶ���ǰһ���ڴ��β��
            if (index < pMem->mem_chunk_count-1)
            {
                temp = (memory_block*)((char *)temp - (start->count)*sizeof(memory_block));
            }
        }

        if (index == pMem->mem_chunk_count)  
        {  
            printf("GetMemory error. not enough memory. size[%ld]\n", sMemorySize);
            return NULL;  
        }

        //��ȡ�ɷ���chunkͷ
        memory_block* start = &(pMem->pmem_map[temp->start]);

        //����ͳ�����
        pMem->mem_used_size += sMemorySize;
        size_t needcount = sMemorySize/MINUNITSIZE;
        pMem->mem_map_pool_count -= needcount;

        //����ƫ����
        size_t current_index = temp-pMem->pmem_map;
        
        //��Ҫ�и�����
        if (start->count > needcount)
        {
            //�޸�Ҫ�����count
            pMem->pmem_map[current_index-needcount+1].count = needcount;
            pMem->pmem_map[current_index-needcount+1].type = TYPE_PERMANENT;
            temp->start = current_index-needcount+1;

            //�޸�ǰһ��chunk�����һ��block
            pMem->pmem_map[current_index-needcount].start = current_index-start->count+1;

            //�޸�ǰһ��
            start->count = start->count - needcount;
            //start->type = TYPE_IDLE;
            //�޸�chunk����
            pMem->mem_chunk_count++;
        }
        //���и�����
        else if (start->count == needcount)
        {
            start->type = TYPE_PERMANENT;
        }

        //�������ݿ������ǵ�ַ
        return index2addr(pMem, current_index-needcount+1);
    }
    return NULL;
}

/************************************************************************/  
/* ���ڴ�����ͷ����뵽���ڴ� 
* pMem���ڴ��ָ�� 
* ptrMemoryBlock�����뵽���ڴ���ʼ��ַ 
/************************************************************************/  
int _FreeMemory(void *ptrMemoryBlock, PMEMORYPOOL pMem)   
{
    if (NULL == ptrMemoryBlock || NULL == pMem)
        return -1;

    size_t current_index = addr2index(pMem, ptrMemoryBlock);  
    if (current_index < 0 || current_index > pMem->mem_block_count-1)
    {
        return -1;
    }
    
    size_t count = pMem->pmem_map[current_index].count;
    size_t size = count * MINUNITSIZE; 

    bool merge_front = false;
    bool merge_back = false;

    //�ж���ǰ�ܲ��ܺϲ�
    if (current_index > 0)
    {
        size_t front_index = pMem->pmem_map[current_index-1].start;
        memory_block* front_block = &(pMem->pmem_map[front_index]);
        if (front_block->type == TYPE_IDLE)
        {
            merge_front = true;
            //printf("debug...  _FreeMemory front.\n");
            
            pMem->pmem_map[current_index-1].start = 0;          
            front_block->count = front_block->count+count;
            
            pMem->pmem_map[current_index].type = TYPE_IDLE;
            pMem->pmem_map[current_index].count = 0;
            pMem->pmem_map[current_index+count-1].start = front_index;

            //��currentָ����ǰ�ƶ�
            current_index = front_index;
            
            //�޸�chunk����
            pMem->mem_chunk_count--;
            
        }
    }

    //�ж�����ܲ��ܺϲ�
    if (current_index < pMem->mem_block_count-1)
    {
        size_t back_index = pMem->pmem_map[current_index].count + current_index;
        if (back_index < pMem->mem_block_count-1)
        {
            memory_block* back_block = &(pMem->pmem_map[back_index]);
            if (back_block->type == TYPE_IDLE)
            {
                merge_back = true;

                //printf("debug...  _FreeMemory back.\n");
                
                //����count 
                size_t  temp_count = pMem->pmem_map[current_index].count;

                //back_count
                size_t back_count = back_block->count;
                
                //�޸�
                pMem->pmem_map[back_index-1].start = 0;
                pMem->pmem_map[current_index].count = temp_count+back_count;
                pMem->pmem_map[current_index].type = TYPE_IDLE;

                pMem->pmem_map[back_index+back_count-1].start = current_index;
                
                //�޸�chunk����
                pMem->mem_chunk_count--;
            }
        }
    }


    //���ǰ�󶼲����Ժϲ�
    if (!merge_back && !merge_front)
    {
        pMem->pmem_map[current_index].type = TYPE_IDLE;
    }

    //�޸�ͳ��ֵ
    pMem->mem_used_size -= size;
    pMem->mem_map_pool_count += count;    

    return 0;
}


