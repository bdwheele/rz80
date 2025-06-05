typedef struct {
    int (*con_ready)();
    char (*con_read)();
    void (*con_write)();
    

} backend_ops_t;




