#include <ngx_config.h>
#include <ngx_core.h>
#include "blockrequestqueue.h"



int isBlockQueueEmpty(){
	return (blockRequestQueue.head == blockRequestQueue.tail);
}

int isBlockQueueFull(){
	return ((blockRequestQueue.tail + 1) % blockRequestQueue.size == blockRequestQueue.head);
}

int blockQueueOffer(httpParams_pool* e){
	int head;
	if(isBlockQueueFull()){
		head = (blockRequestQueue.tail + 1) % blockRequestQueue.size;
		if(__sync_bool_compare_and_swap(&blockRequestQueue.head, head, (head + 1)%blockRequestQueue.size)){
			httpParams_pool_list* l = alpaca_memory_pool_malloc(blockRequestQueue.CircularQueue[head]->pool, sizeof(httpParams_pool_list));
			if(!l){
				return 0;
			}
			l->value = blockRequestQueue.CircularQueue[head];
			httpParams_pool_list* freelist_head;
			do{
				freelist_head = freelist;
				l->next = freelist_head;
			}while(!__sync_bool_compare_and_swap(&freelist, freelist_head, l));
		}
		//freePairP(buf, PUSH_BLOCK_ARGS_NUM);
	}
	blockRequestQueue.CircularQueue[blockRequestQueue.tail] = e;
	blockRequestQueue.tail = (blockRequestQueue.tail + 1) % blockRequestQueue.size;
	return 1;
}

httpParams_pool* blockQueuePoll(){
	if(isBlockQueueEmpty()){
		return NULL;
	}
	int head;
	do{
		head = blockRequestQueue.head;
	}while(!__sync_bool_compare_and_swap(&blockRequestQueue.head, head, (head + 1)%blockRequestQueue.size));
	httpParams_pool* result = blockRequestQueue.CircularQueue[head];
	httpParams_pool_list* l = alpaca_memory_pool_malloc(result->pool, sizeof(httpParams_pool_list));
	if(!l){
		return NULL;
	}
	l->value = result;
	httpParams_pool_list* freelist_head;
	do{
		freelist_head = freelist;
		l->next = freelist_head;//TODO check
	}while(!__sync_bool_compare_and_swap(&freelist, freelist_head, l));
	//freePairP(*blockRequestQueue.CircularQueue[blockRequestQueue.head], 8);
	return result;
}

void freePairP(Pair* pair, int len){
	int i;
	if(!pair){
		return;
	}
	for(i = 0; i < len; i++){
		if(pair[i].key){
			free(pair[i].key);
		}
		if(pair[i].value){
			free(pair[i].value);
		}
	}
	free(pair);
}
void httpParams_pool_free(httpParams_pool* p){
	alpaca_memory_pool_destroy(p->pool);
}
