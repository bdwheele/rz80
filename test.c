#include <stdio.h>
#include <string.h>

#define SECTOR "7a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f137a3b8c43b0791ba6ce01f5696fd36f13"
#define NL "\n"
#define READCMD "r000101" NL
#define READMSG "K" SECTOR NL
#define WRITECMD "w000101" SECTOR NL
#define WRITEMSG "K" NL
#define FORMATCMD "fdeadbeef" NL
#define FORMATMSG "K" NL
#define MOTORONCMD "m01" NL
#define MOTORONMSG "K" NL
#define MOTOROFFCMD "m00" NL
#define MOTOROFFMSG "K" NL

char databuffer[516];

int main(int argc, char *argv[]) {
    unsigned int cyl, head, sector, d, r;
    char *cmd = WRITECMD;
    printf("cmd length: %d\n", strlen(cmd));
    if(3 == sscanf(cmd, "r%02x%02x%02x", &cyl, &head, &sector)) {
        printf("c=%d, h=%d, s=%d\n", cyl, head, sector);
        printf(READMSG);    
    } else if(3 == sscanf(cmd, "w%02x%02x%02x", &cyl, &head, &sector)) {
        printf("c=%d, h=%d, s=%d\n", cyl, head, sector);
        int i;
        for(i = 0; i < 512; i++) {
            r = sscanf(cmd + i + 7, "%02x", &d);
            if(r != 1) {
                printf("EInvalid Sector");
                break;
            } else {
                databuffer[i + 4] = d;
            }            
        }
        if(i == 512) {
            printf("KData written\n");
        }
    } else {
        printf("EUnknown command\n");
    }
    
    
}

