//Name: LessTanker
//loginID: wobudaoa 
#define _POSIX_C_SOURCE 200809L
#include "cachelab.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int hit_count = 0, miss_count = 0, eviction_count = 0; 
int s, b;
int S, E, B;
char* tracefile;
int time_counter = 0;

struct Line
{
    int valid;
    unsigned tag;
    int lru_count;
};

struct Set
{
    struct Line* bb;
};

struct Cache
{
    struct Set* aa;
}cache;

// void update_lru()
// {
//     for(int i=0;i<S;i++)
//         for(int j=0;j<E;j++)
//             cache.aa[i].bb[j].lru_count++;
// }

void cache_process(unsigned addr)
{
    time_counter++;
    //update_lru();
    unsigned tag = addr >> (s+b);
    unsigned set_index = (addr >> b) & (S - 1);
    struct Set* selected_set = &cache.aa[set_index];
    int available_index = -1;
    for(int i=0;i<E;i++)
    {
        if(selected_set->bb[i].tag == tag && selected_set->bb[i].valid == 1)
        {
            available_index = i;
            break;
        } 
    }
    struct Line* line = &cache.aa[set_index].bb[available_index];
    if(available_index != -1)
    {
        hit_count++;
        //line->lru_count = 0;
        line->lru_count = time_counter;
    }
    else
    {   
        miss_count++;
        int empty_index = -1;
        for(int i=0;i<E;i++)
        {
            if(selected_set->bb[i].valid == 0)
            {
                empty_index = i;
                break;
            }
        }
        if(empty_index != -1)
        {
            struct Line to_insert = selected_set->bb[empty_index];
            to_insert.valid = 1;
            to_insert.tag = tag;
            to_insert.lru_count = time_counter;
            selected_set->bb[empty_index] = to_insert;
        }
        else
        {
            eviction_count++;
            int victim_index = 0;
            for(int i=1;i<E;i++)
            {
                if(selected_set->bb[i].lru_count < selected_set->bb[victim_index].lru_count)
                    victim_index = i;
            }
            struct Line to_victim = selected_set->bb[victim_index];
            to_victim.valid = 1;
            to_victim.tag = tag;
            to_victim.lru_count = time_counter;
            selected_set->bb[victim_index] = to_victim;
        }
    }
}

void parse_opt(char* line)
{
    char op;
    unsigned addr;
    int size;
    if(sscanf(line, " %c %x,%d", &op, &addr, &size) != 3)
        printf("Parse error: %s\n", line);
    if(op == 'I') 
        return;
    if(op == 'M')
        cache_process(addr);
    cache_process(addr);
}

int main(int argc, char* argv[])
{
    int opt;
    while((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch(opt)
        {
            case 'h': case 'v': break;
            case 's':
                s = atoi(optarg);
                S = pow(2, s);
                break; 
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                B = pow(2, b);
                break;
            case 't':
                tracefile = optarg;
                break;
        }
    }

    cache.aa = malloc(S * sizeof(struct Set));
    for(int i=0;i<S;i++)
    {
        cache.aa[i].bb = malloc(E * sizeof(struct Line));
        for(int j=0;j<E;j++)
        {
            cache.aa[i].bb[j].lru_count = 0;
            cache.aa[i].bb[j].tag = 0;
            cache.aa[i].bb[j].valid = 0;
        }
    }
        
    FILE* fp = fopen(tracefile, "r");
    if(!fp)
    {
        perror("Failed to open file");
        return 1;
    }
    char* line = NULL;
    size_t len = 0;
    while(getline(&line, &len, fp) != -1)
        parse_opt(line);
    free(line);
    fclose(fp);

    for(int i=0;i<S;i++)
        free(cache.aa[i].bb);
    free(cache.aa);

    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
