#include "config.h"
#include "sys/stat.h"
#include "malloc.h"
#include "unistd.h"
#include "stdio.h"

void *rawbuf = NULL;
struct conf* read_config(char *filename) {
    /* load the file into memory */
    struct stat *statbuf = malloc(sizeof(struct stat));
    stat(filename, statbuf);
    rawbuf = malloc(statbuf->st_size + 1);
    FILE * h = fopen(filename, 'r');
    fread(rawbuf, statbuf->st_size, 1, h);
    fclose(h);
    
    /* parse the key/val data */
    struct conf *head = calloc(1, sizeof(struct conf));
    struct conf *here = head;
    char *p = rawbuf;
    here->key = p;
    


}


int _find_key(char *key) {}

int get_int(char *key, int val) {

}

char *get_string(char *key, char *val) {

}