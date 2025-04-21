/*
 * cpicsk_gen
 * Copyright (c) 2024 nosuke <sasugaanija@gmail.com>
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <unistd.h>
extern char *optarg;
extern int optind, opterr, optopt;
#include <getopt.h>

#ifdef __MINGW32__
#include <windows.h>
#endif

#define COPY "2024 nosuke <sasugaanija@gmail.com>"
#define VERSION "1.01"

#define CODE_LEN 11
#define CONFIG_LEN (CODE_LEN + 3)

#define TITLE_LEN 256

#define HEX_INTERVAL 2

#define TEMPLATE_EXPECT 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x11, 0x33, 0x55, 0x77, 0x99, 0xBB

#define TEMPLATE_START 0x1e4

#define CONFIG_DEFAULT ":021FFE00EAFFF8"
#define CONFIG_XGPRO   ":020FFE00EA0FF8"


#ifdef __MINGW32__
int cp_org_valid = 0;
UINT cp_org;

BOOL WINAPI CtrlHandler(DWORD dwCtrlType)
{
    switch(dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        if (cp_org_valid) {
            SetConsoleOutputCP(cp_org);
        }
        return FALSE;
        break;
    default:
        break;
    }
    return TRUE;
}
#endif


enum ihex_record_type {
    DATA = 0,
    END_OF_FILE,
    EXT_SEG_ADDR,
    START_SEG_ADDR,
    EXT_LINEAR_ADDR,
    START_LINEAR_ADDR
};


int
hexstr2val(char *hex)
{
    char *endptr;
    unsigned char ret;
    char buf[5] = "0x";
    buf[2] = hex[0];
    buf[3] = hex[1];
    buf[4] = '\0';
    ret = strtol(buf, &endptr, 16);

    if (*endptr != '\0') {
        // invalid hexadecimal string
        return -1;
    }

    return ret;
}


void
remove_newline_code(char *line)
{
    int len = strlen(line);
    if (len == 0)
        return;

    // remove 'LF'
    if ((line[len - 1]) == '\n') {
        line[len - 1] = '\0';
        len--;
        if (len == 0)
            return;
    }


    // remove 'CR'
    if ((line[len - 1]) == '\r') {
        line[len - 1] = '\0';
    }

    return;
}




int
patch_hex(char *line, unsigned char config[], int offset, int length)
{
    char *p = line;
    int byte_count;
    unsigned short address = 0;
    int addr_low, addr_high;
    enum ihex_record_type type;
    int sum = 0;
    char tmp[3];
    int len;
    unsigned char expect[CONFIG_LEN] = {TEMPLATE_EXPECT};
    int ret = 0;

    remove_newline_code(line);
    len = strlen(line);

    // length must be larger than 11
    // 11 = start code (1) + byte count (2) + address (4) + record type (2)
    //      + check sum (2)
    if (len < 11) {
        fprintf(stderr, "Hex file error: line \"%s\" is too short (%d)\n",
                line, len);
        return -1;
    }

    // start code
    if (p == NULL || *p != ':') {
        fprintf(stderr, "Hex file error: 1st character is not ':'\n");
        return -1;
    }
    p += 1;
    len --;


    // byte count
    byte_count = hexstr2val(p);
    if (byte_count < 0) {
        fprintf(stderr, "Hex file error: non hexadecimal characters\n");
        return -1;
    }
    sum += byte_count;
    p += 2;
    len -= 2;
   
    // address
    addr_high = hexstr2val(p);
    addr_low = hexstr2val(p + 2);
    if (addr_high < 0 || addr_low < 0) {
        fprintf(stderr, "Hex file error: non hexadecimal characters\n");
        return -1;
    }
    sum += addr_high + addr_low;
    
    address = (unsigned short)((addr_high << 8) | addr_low);
    p += 4;
    len -= 4;

    // record type
    type = hexstr2val(p);
    if (type < 0) {
        fprintf(stderr, "Hex file error: non hexadecimal characters\n");
        return -1;
    }

    if (type != DATA)
        return 0;

    sum += type;
    p += 2;
    len -= 2;

    
    if (len < byte_count * 2) {
        fprintf(stderr, "invalid format\n");
        return -1;
    }
    
    for (int i = 0; i < byte_count; i++) {
        int d = hexstr2val(p);

        if (d < 0) {
            fprintf(stderr, "Hex file error: non hexadecimal characters\n");
            return -1;
        }

        if (type == DATA
            && address >= offset
            && address < offset + HEX_INTERVAL * length
            && (address - offset) % HEX_INTERVAL == 0) {

            int idx = (address - offset) / HEX_INTERVAL;

            if (d != expect[idx]) {
                fprintf(stderr, "Template file error: "
                        "address: 0x%04x, expected: 0x%02x, original: 0x%02x\n",
                        address, expect[idx], d);
                ret = -1;
            }

            sprintf(tmp, "%02X", config[idx]);
            *p = tmp[0];
            *(p + 1) = tmp[1];
            ret++; // replace count
            sum += config[idx];
        } else {
            sum += d;
        }
        p += 2;
        len -= 2;
        
        address++;
    }

    if (len < 2) {
        fprintf(stderr, "invalid format\n");
        return -1;
    }
    
    sprintf(p, "%02X", -sum & 0xff);

    return ret;
}



int
read_config(char *buf, unsigned char *config, char *title)
{
    char *p = buf;
    char *p_next;
    int idx = 0;
    int val;

    // config file format:
    // ( *(VAL) *,){CODE_LEN-1,CONFIG_LEN-1} *(VAL) *# *(.*)$
    // VAL is integer value from 0 to 255

    for (int i = 0; i < CONFIG_LEN; i++) {
        // skip space
        while (isspace(*p))
            p++;

        errno = 0;
        val = strtol(p, &p_next, 0);
        if (errno != 0) {
            perror("strtol");
            fprintf(stderr, "Invalid config\n");
            //return -1;
            goto POINT_ERROR;
        }

        if (p == p_next) {
            fprintf(stderr, "Invalid config format\n");
            //return -1;
            goto POINT_ERROR;
        }

        if (val < 0 || val > 256) {
            fprintf(stderr, "Invalid config (each value of config must be a single byte data)\n");
            //return -1;
            goto POINT_ERROR;
        }

        config[idx++] = val;
        p = p_next;

        while (isspace(*p))
            p++;

        if (*p != ',') {
            break;
        }

        p++;
    }

    if (idx < CODE_LEN) {
        fprintf(stderr, "Invalid config format (too short)\n");
        goto POINT_ERROR;
    }

    // title

    // skip space
    while (isspace(*p))
        p++;

    if (*p == '#') {
        p++;
        // skip space
        while (isspace(*p))
            p++;
        strncpy(title, p, TITLE_LEN);
        title[TITLE_LEN - 1] = '\0';
    }

    return 1;

POINT_ERROR:
    printf("idx=%d\n", idx);
    printf("%s\n", buf);
    for (char *p1 = buf; p1 != p; p1++) {
        printf(" ");
    }
    printf("^\n");

    return -1;
}


void
dump(unsigned char *p, int len, int noaddr)
{
    for (int i = 0; i < len; i++) {
        if (!noaddr && i % 16 == 0)
            printf("%08x  ", i);

        printf("%02x", p[i]);
        
        if (i % 16 < 16 - 1) {
            printf(" ");
            if (i % 16 == 7)
                printf(" ");
        } else {
            printf("\n");
        }
    }
    if (len % 16 != 0) {
        printf("\n");
    }
}


#define TEMPLATE_FILE "template.hex"
#define CONFIG_FILE "config.txt"
#define OUT_FILE "cpicskprg.hex"
#define OUT_FILE2 "cpicskprgx.hex"


void
version()
{
    printf("CPicSK program generator (ver %s)\n", VERSION);
    printf("Copyright (c) %s\n", COPY);
}

void
usage(char **argv)
{
    fprintf(stderr, "USAGE: %s [OPTION]\n", argv[0]);
    fprintf(stderr, "       %s [OPTION] <template> <config> <out1> <out2>\n\n", argv[0]);

    fprintf(stderr, "  template: template PIC program file (default: %s)\n", TEMPLATE_FILE);
    fprintf(stderr, "  config:   KABUKI configuration file (default: %s)\n", CONFIG_FILE);
    fprintf(stderr, "  out1:     PIC program file for general programmer (default: %s)\n", OUT_FILE);
    fprintf(stderr, "  out2:     PIC program file for Xgpro (default: %s)\n\n", OUT_FILE2);

    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v         show tool version\n");   
    fprintf(stderr, "  -h         show this message\n");   
}

int
main(int argc, char **argv)
{
    FILE *fpi, *fpo;
    char buf[512];

    char *templatefile;
    char *configfile;
    char *outfile;
    char *outfile2;
    unsigned char config[CONFIG_LEN];
    char title[TITLE_LEN] = "";
    int ret = -1;

    int opt;

#ifdef __MINGW32__
    cp_org = GetConsoleOutputCP();
    cp_org_valid = 1;
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
    SetConsoleOutputCP(65001);
#endif

    while ((opt = getopt(argc, argv, "vh")) != -1) {
        switch (opt) {
        case 'v':
            ret = 0;
            version();
            goto FIN;
        case 'h':
            usage(argv);
            goto FIN;
        default:
            fprintf(stderr, "Invalid options\n");
            usage(argv);
            goto FIN;
        }
    }

    if (argc - optind == 4) {
        templatefile = argv[optind];
        configfile = argv[optind + 1];
        outfile = argv[optind + 2];
        outfile2 = argv[optind + 3];
    } else if (argc - optind == 0) {
        templatefile = TEMPLATE_FILE;
        configfile = CONFIG_FILE;
        outfile = OUT_FILE;
        outfile2 = OUT_FILE2;
    } else {
        fprintf(stderr, "Invalid argument\n");
        usage(argv);
        goto FIN;
    }

    // read config file
    printf("Read config file \"%s\" ... ", configfile);
    fflush(stdout);
    
    fpi = fopen(configfile, "rb");
    if (fpi == NULL) {
        perror("fopen");
        fprintf(stderr, "Config file \"%s\" open failed\n", configfile);
        goto FIN;
    }

    printf("OK\n");
    
    fgets(buf, sizeof(buf) - 1, fpi);

    fclose(fpi);

    remove_newline_code(buf);

    memset(config, 0, CONFIG_LEN);

    if (read_config(buf, config, title) < 0) {
        goto FIN;
    }

   
    printf("================================\n");
    printf("Title: %s\n", title);
    printf("Code:  ");
    dump(config, CODE_LEN, 1);
    printf("Delay: %d msec\n", config[CODE_LEN] * 10);
    printf("Blink: %s\n", config[CODE_LEN + 1] ? "Yes" : "No");
    printf("Slow:  %s\n", config[CODE_LEN + 2] ? "Yes" : "No");
  
    printf("================================\n");
    
    // open template hex file

    printf("Read template hex file \"%s\" ... ", templatefile);
    fflush(stdout);
    
    fpi = fopen(templatefile, "rb");
    if (fpi == NULL) {
        perror("fopen");
        fprintf(stderr, "Template hex file \"%s\" open failed\n", templatefile);
        goto FIN;
    }

    // search template code location in template hex file

    printf("OK\n");

    printf("Generate hex file for regular programmer \"%s\" ... ", outfile);
    fflush(stdout);

    fpo = fopen(outfile, "wb");
    if (fpo == NULL) {
        perror("fopen");
        fprintf(stderr, "Output file \"%s\" open failed\n", outfile);
        goto FIN;
    }
    
    rewind(fpi);
    while (fgets(buf, sizeof(buf) - 1, fpi) > 0) {
        patch_hex(buf, config, TEMPLATE_START, CONFIG_LEN);
        fprintf(fpo, "%s\r\n", buf);
    }
    fclose(fpo);

    printf("OK\n");


    printf("Generate hex file for Xgpro \"%s\" ... ", outfile2);
    fflush(stdout);
    
    fpo = fopen(outfile2, "wb");
    if (fpo == NULL) {
        perror("fopen");
        fprintf(stderr, "Output file \"%s\" open failed\n", outfile2);
        goto FIN;
    }


    rewind(fpi);
    while (fgets(buf, sizeof(buf) - 1, fpi) > 0) {
        patch_hex(buf, config, TEMPLATE_START, CONFIG_LEN);
        if (strncmp(buf, CONFIG_DEFAULT, strlen(CONFIG_DEFAULT)) == 0) {
            memcpy(buf, CONFIG_XGPRO, strlen(CONFIG_XGPRO));
        }
        fprintf(fpo, "%s\r\n", buf);
    }

    printf("OK\n");
    
    fclose(fpo);

    fclose(fpi);

    ret = 0;
    
 FIN:
    fflush(stderr);

    printf("Press enter key to exit.\n");

    fflush(stdout);
    
    getchar();

#ifdef __MINGW32__
    SetConsoleOutputCP(cp_org);
#endif
    
    return ret;
}
