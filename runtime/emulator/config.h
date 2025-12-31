#define MAX_CONF_KEYS 30

struct conf {
    char *key;
    void *val;
    struct conf *next;
};


struct conf* read_config(char *filename);
int get_int(char *key, int val);
char *get_string(char *key, char *val);
