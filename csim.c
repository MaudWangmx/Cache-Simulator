#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Block{
    int tag;
    int valid;
    struct Block* nextBlock;
}CacheRow;

int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;
int hit;
int miss;
int eviction;
int indexbits = 0;  //bits of group index
int indexnum = 0; //number of indexes
int relevance = 0; //rows in each group
int offset = 0; //bits of a block address
int TracePrinter = 0; // if 1, print trace 
char *TestFileName; 

void HelpInfo(){
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\nOptions:\n");
	printf("  -h      \tPrint this help message.\n");
	printf("  -v      \tOptional verbose flag.\n");
	printf("  -s <num>\tNumber of set index bits.\n");
	printf("  -E <num>\tNumber of lines per set.\n");
	printf("  -b <num>\tNumber of block offset bits.\n");
	printf("  -t <file>\tTrace file.\n");
	printf("Examples:\n");
	printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
	printf("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void TraceInfoPrinter(char operation){
    
    if (hit == 1)
        printf(" hit");
    if (miss == 1)
        printf(" miss");
    if(eviction == 1)
        printf(" eviction");
    if (operation == 'M'){

        printf(" hit");
    } 
    printf("\n");

    return;
        
}

void CreateCache(CacheRow **Cache){
    for (int i = 0; i < indexnum; i++)
    {
        if (relevance == 1)
        {
            Cache[i] = (CacheRow*)malloc(sizeof(CacheRow));
            Cache[i]->nextBlock = NULL;
            Cache[i]->tag = 0;
            Cache[i]->valid = 0;
        }
        else{
            Cache[i] = (CacheRow*)malloc(sizeof(CacheRow));
            Cache[i]->nextBlock = NULL;
            Cache[i]->tag = 0;
            Cache[i]->valid = 0;
            CacheRow *BlockPointer = Cache[i];
            for(int j = 1; j < relevance; j++){
                BlockPointer->nextBlock = (CacheRow*)malloc(sizeof(CacheRow));
                BlockPointer = BlockPointer->nextBlock;
                BlockPointer->tag = 0;
                BlockPointer->valid = 0;
            }
            BlockPointer->nextBlock = NULL;
        }   
    }
    return;
}

void UpdateCache(CacheRow **Cache,char operation, int Index_Access, long long Tag_Access){
    CacheRow *BlockPointer = Cache[Index_Access];
    CacheRow *LastEmptyBlock = NULL, *LastBlock = Cache[Index_Access];
    CacheRow *Pre_Block = Cache[Index_Access], *Pre_LastBlock = Cache[Index_Access];

    while (1)
    {
        if(BlockPointer->valid == 0){   //find the last empty block in the index accessed
            LastEmptyBlock = BlockPointer;
        }
        if (BlockPointer->valid == 1 && BlockPointer->tag == Tag_Access)    //hit
        {
            hit = 1;
            hit_count++;
            break;
        }
        if(BlockPointer->nextBlock == NULL){    //reach the last block, missed
            miss = 1;
            miss_count++;
            LastBlock = BlockPointer;
            Pre_LastBlock = Pre_Block;
            break;
        }
        Pre_Block = BlockPointer;
        BlockPointer = BlockPointer->nextBlock;   
    }
    if(hit){
        if(Pre_Block != BlockPointer){  //hit block is not the first block in the group
            Pre_Block->nextBlock = BlockPointer->nextBlock;
            if(LastEmptyBlock != NULL){
                BlockPointer->nextBlock = LastEmptyBlock->nextBlock;
                LastEmptyBlock->nextBlock = BlockPointer;
            }else{
                BlockPointer->nextBlock = Cache[Index_Access];
                Cache[Index_Access] = BlockPointer;
            }
        }
    }
    else if(LastEmptyBlock != NULL){//access missed and there is an empty block in the index group
        LastEmptyBlock->tag = Tag_Access;
        LastEmptyBlock->valid = 1;
    }else{  //access missed and all blocks in the group are occupied
        eviction = 1;
        eviction_count++;
        LastBlock->tag = Tag_Access;
	if(Pre_LastBlock != LastBlock){
            Pre_LastBlock->nextBlock = NULL;
            LastBlock->nextBlock = Cache[Index_Access];
            Cache[Index_Access] = LastBlock;
        }
    }
	if(operation == 'M')
		hit_count++;
   if(TracePrinter)
      TraceInfoPrinter(operation);

    return;
}

void ReadTestFile(CacheRow **Cache){
    FILE *fp;
    fp = fopen(TestFileName, "r");
    if(fp == NULL){
        printf("ERROR: File open failed.\n");
        return;
    }
    char operation; // access type
    long long unsigned address; 
    int size;   // number of bytes accessed in memory
    while ((fscanf(fp, "%s %llx,%d", &operation, &address, &size)) != EOF)
    {	

        if (operation == 'I')
            continue;   // skip all instruction access
        int Index_Access = (address & (0xFFFFFFFFFFFFFFFF >> offset << (64 - indexbits) >> (64 - indexbits - offset))) >> offset;
        long long unsigned Tag_Access = (address & (0xFFFFFFFFFFFFFFFF << (indexbits + offset))) >> (indexbits + offset);
	if (TracePrinter)
		printf("%c %llx, %d", operation, address, size);

        hit = 0;
        miss = 0;
        eviction = 0;
	
       UpdateCache(Cache, operation, Index_Access, Tag_Access);
        
    }
	fclose(fp);
    
}


int main(int argc, char** argv)
{  

 if (argv[1][1] == 'h') {
        HelpInfo();
        return 0;
    }
    if (argc == 10) {
        if(argv[1][1] == 'v'){
            TracePrinter = 1;
        }
        indexbits = atoi(argv[3]);
        relevance = atoi(argv[5]);
        offset = atoi(argv[7]);
        TestFileName = argv[9];
    } else {
        indexbits = atoi(argv[2]);
        relevance = atoi(argv[4]);
        offset = atoi(argv[6]);
        TestFileName = argv[8];
    }
 
    indexnum = 1 << indexbits;
    CacheRow *Cache[indexnum];
    CreateCache(Cache);
    ReadTestFile(Cache);
    

    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}

