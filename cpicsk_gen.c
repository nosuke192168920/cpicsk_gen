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

#define KEY_LEN 11
#define CONFIG_LEN (KEY_LEN + 3)

#define KEY_INTERVAL 2

#define TEMPLATE_EXPECT 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x11, 0x33, 0x55, 0x77, 0x99, 0xBB

#define TEMPLATE_START 0x1e4

#define PIC_PROGRAM_SIZE_MAX (0x400 * 2)

#define CONFIG_DEFAULT ":021FFE00EAFFF8"
#define CONFIG_XGPRO   ":020FFE00EA0FF8"

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
patch_hex(char *line, unsigned char config[], int offset, int interval, int length)
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
            && address < offset + interval * length
            && (address - offset) % interval == 0) {

            int idx = (address - offset) / interval;

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
parse_key(char *buf, unsigned char config[])
{
    char *p = buf;

    // parse key file
    for (int i = 0; i < CONFIG_LEN; i++) {
        char *p2;
        long val;
        p2 = strchr(p, ',');
        if (p2 != NULL) {
            *p2 = '\0';
        } else if (i < KEY_LEN - 1) {
            // ',' not found and i is not last value
            fprintf(stderr, "Invalid key (key should be %d Byte or larger)\n", KEY_LEN);
            return -1;
        }
        
        errno = 0;
        val = strtol(p, NULL, 0);
        if (errno != 0) {
            perror("strtol");
            fprintf(stderr, "Invalid key (key should be hexadecimal value array)\n");
            return -1;
        }

        if (val < 0 || val > 256) {
            fprintf(stderr, "Invalid key (each value of key should be a single byte data)\n");
            return -1;
        }

        config[i] = val;

        if (p2 != NULL) {
            p = p2 + 1;

/*
            // process debug flag
            if (i == KEY_LEN - 1) {
                errno = 0;
                config[KEY_LEN] = strtol(p, &p2, 0); // last entry is used for debug flag
            }
*/
        } else {
            // last entry
            break;
        }
    }
    return 1;
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
#define KEY_FILE "key.txt"
#define OUT_FILE "cpicskprg.hex"
#define OUT_FILE2 "cpicskprg_xgpro.hex"


int
main(int argc, char **argv)
{
    FILE *fpi, *fpo, *fpo2;
    char buf[512];

    char *templatefile;
    char *keyfile;
    char *outfile;
    char *outfile2;
    unsigned char config[CONFIG_LEN];
    int ret = -1;
//    int key_offset;

    if (argc == 5) {
        templatefile = argv[1];
        keyfile = argv[2];
        outfile = argv[3];
        outfile2 = argv[4];
    } else if (argc == 1) {
        templatefile = TEMPLATE_FILE;
        keyfile = KEY_FILE;
        outfile = OUT_FILE;
        outfile2 = OUT_FILE2;
    } else {
        fprintf(stderr, "Invalid argument\n");
        fprintf(stderr, "USAGE: cpicsk_gen\n");
        fprintf(stderr, "       cpicsk_gen <templatefile> <keyfile> <outfile> <outfile2>\n");
        goto FIN;
    }

    // read key file
    printf("Read key file \"%s\" ... ", keyfile);
    fflush(stdout);
    
    fpi = fopen(keyfile, "rb");
    if (fpi == NULL) {
        perror("fopen");
        fprintf(stderr, "Key file \"%s\" open failed\n", keyfile);
        goto FIN;
    }

    printf("OK\n");
    
    fgets(buf, sizeof(buf) - 1, fpi);

    fclose(fpi);


    memset(config, 0, CONFIG_LEN);

    if (parse_key(buf, config) < 0) {
        goto FIN;
    }

   
    printf("================================\n");
    printf("Key:\t");

    dump(config, KEY_LEN, 1);
    printf("Start delay:\t%d msec\n", config[KEY_LEN] * 10);
    printf("Blink:\t%s\n", config[KEY_LEN + 1] ? "Yes" : "No");
    printf("Slow:\t%s\n", config[KEY_LEN + 2] ? "Yes" : "No");
  
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

    // search template key location in template hex file

    printf("OK\n");

/*
    key_offset = search_template_in_hex(fpi);
    if (key_offset < 0) {
        fclose(fpi);
        goto FIN;
    } else if (key_offset == PIC_PROGRAM_SIZE_MAX) {
        // no expect data
        fprintf(stderr, "Tempalte hex file error: no expected data found\n");
        fclose(fpi);
        goto FIN; 
    } else {
        printf("OK (key template offset: 0x%03x)\n", key_offset);
    }
*/

    printf("Write to output file \"%s\" & \"%s\" ... ", outfile, outfile2);
    fflush(stdout);
    

    fpo = fopen(outfile, "wb");
    if (fpo == NULL) {
        perror("fopen");
        fprintf(stderr, "Output file \"%s\" open failed\n", outfile);
        goto FIN;
    }


    fpo2 = fopen(outfile2, "wb");
    if (fpo2 == NULL) {
        perror("fopen");
        fprintf(stderr, "Output file \"%s\" open failed\n", outfile2);
        goto FIN;
    }


    rewind(fpi);        
    while (fgets(buf, sizeof(buf) - 1, fpi) > 0) {
        patch_hex(buf, config, TEMPLATE_START, KEY_INTERVAL, CONFIG_LEN);
        fprintf(fpo, "%s\r\n", buf);

        if (strncmp(buf, CONFIG_DEFAULT, strlen(CONFIG_DEFAULT)) == 0) {
            memcpy(buf, CONFIG_XGPRO, strlen(CONFIG_XGPRO));
        }

        fprintf(fpo2, "%s\r\n", buf);

    }

    printf("OK\n");
    
    fclose(fpo);
    fclose(fpo2);

    fclose(fpi);

    ret = 0;
    
 FIN:
    fflush(stderr);

    printf("Press any key to exit.\n");

    fflush(stdout);
    
    getchar();
    
    return ret;
}
