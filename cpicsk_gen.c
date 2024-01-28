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
//#define KEY_OFFSET 0x1e8
#define KEY_INTERVAL 2

#define TEMPLATE_EXPECT 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x11, 0x33, 0x55, 0x77

#define PIC_PROGRAM_SIZE_MAX (0x400 * 2)

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


enum record_type {
    DATA = 0,
    END_OF_FILE,
    EXT_SEG_ADDR,
    START_SEG_ADDR,
    EXT_LINEAR_ADDR,
    START_LINEAR_ADDR
};


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
parse_hex_line(char *line, unsigned short *offset, int *byte_count, unsigned char *data)
{
    char *p = line; 
    int len;
    int addr_low, addr_high;
    enum record_type type;
    int sum_org, sum = 0;
    int ret = 1;

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
    *byte_count = hexstr2val(p);
    if (*byte_count < 0) {
        fprintf(stderr, "Hex file error: non hexadecimal characters\n");
        return -1;
    }
    sum += *byte_count;
    p += 2;
    len -= 2;

    // check remaining data length
    if (len != 4 + 2 + *byte_count * 2 + 2) {
        fprintf(stderr, "Hex file error: line length is unmatched with byte count field (%d)\n", *byte_count);
        return -1;
    }
    

    // address
    addr_high = hexstr2val(p);
    addr_low = hexstr2val(p + 2);

    if (addr_high < 0 || addr_low < 0) {
        fprintf(stderr, "Hex file error: non hexadecimal characters\n");
        return -1;
    }

    sum += addr_high + addr_low;
    
    *offset = (unsigned short)((addr_high << 8) | addr_low);
    p += 4;
    len -= 4;

    // record type
    type = hexstr2val(p);
    if (type < 0) {
        fprintf(stderr, "Hex file error: non hexadecimal characters\n");
        return -1;
    }
    if (type == END_OF_FILE)
        ret = 2;

    sum += type;
    p += 2;
    len -= 2;
   
    // data
    for (int i = 0; i < *byte_count; i++) {

        int d = hexstr2val(p);
        if (d < 0) {
            fprintf(stderr, "Hex file error: non hexadecimal characters\n");
            return -1;
        }
        data[i] = (unsigned char)d;

        sum += d;
        p += 2;
        len -= 2;
    }

    // check sum
    sum_org = hexstr2val(p);
    if (sum_org < 0) {
        fprintf(stderr, "Hex file error: non hexadecimal characters\n");
        return -1;
    }

    if ((unsigned char)(sum_org + sum) != 0) {
        fprintf(stderr, "Hex file error: check sum is unmatch (0x%02X, 0x%02X)\n",
                sum_org, sum);
        return -1;
    }

    return ret;
}


int
search_template_in_hex(FILE *fpi)
{
    unsigned char program[PIC_PROGRAM_SIZE_MAX];
    char buf[512];
    unsigned short offset;
    unsigned char data[255]; // each line has up to 255Byte data
    int count;
    unsigned char expect[KEY_LEN + 1] = {TEMPLATE_EXPECT};
    int ret = PIC_PROGRAM_SIZE_MAX; // PIC_PROGRAM_SIZE_MAX means "no error but no expected data"

    memset(program, 0xFF, PIC_PROGRAM_SIZE_MAX);

    while (fgets(buf, sizeof(buf) - 1, fpi) > 0) {

        int ret = parse_hex_line(buf, &offset, &count, data);
        if (ret < 0) {
            return -1;
        }
        if (ret == 2) {
            // end of file
            break;
        }

        for (int i = 0; i < count; i++) {
            if (offset + i < PIC_PROGRAM_SIZE_MAX) {
                program[offset + i] = data[i];
            }
        }
    }

    // search template
    for (int i = 0; i < PIC_PROGRAM_SIZE_MAX - (KEY_LEN + 1) * 2; i += 2) {
        int found = 1;
        for (int j = 0; j < KEY_LEN + 1; j++) {
            if (program[i + j * 2] != expect[j] || program[i + j * 2 + 1] != 8) {
                // data has 8 in the upper 4bit
                found = 0;
                break;
            }
        }
        if (found) {
            ret = i;
        }
    }

    rewind(fpi);
    return ret;
}


int
patch_hex(char *line, unsigned char config[], int offset, int interval, int length)
{
    int byte_count = 0;
    char *p = line;
    unsigned short address = 0;
    unsigned char addr_low, addr_high;
    enum record_type type;
    unsigned char sum = 0;
    char tmp[3];
    int len = strlen(line);
    unsigned char expect[KEY_LEN + 1] = {TEMPLATE_EXPECT};
    int ret = 1;

    // start code
    if (p == NULL || *p != ':') {
        fprintf(stderr, "invalid format\n");
        return -1;
    }
    p += 1;
    len --;

    if (len < 2) {
        fprintf(stderr, "invalid format\n");
        return -1;
    }

    // byte count
    byte_count = hexstr2val(p);
    sum += byte_count;
    p += 2;
    len -= 2;

    if (len < 4) {
        fprintf(stderr, "invalid format\n");
        return -1;
    }
    
    // address
    addr_high = (unsigned short)hexstr2val(p);
    addr_low = (unsigned short)hexstr2val(p + 2);
    sum += addr_high + addr_low;
    
    address = (unsigned short)(addr_high << 8)
        | (unsigned short)hexstr2val(p + 2);
    p += 4;
    len -= 4;

    if (len < 2) {
        fprintf(stderr, "invalid format\n");
        return -1;
    }

    // record type
    type = hexstr2val(p);
    sum += type;
    p += 2;
    len -= 2;

    
    if (len < byte_count * 2) {
        fprintf(stderr, "invalid format\n");
        return -1;
    }
    
    for (int i = 0; i < byte_count; i++) {
        unsigned char d;

        if (type == DATA
            && address >= offset
            && address < offset + interval * length
            && (address - offset) % interval == 0) {

            int idx = (address - offset) / interval;

            unsigned char org = hexstr2val(p);
            if (org != expect[idx]) {
                fprintf(stderr, "Template file error: "
                        "address: 0x%04x, expected: 0x%02x, original: 0x%02x\n",
                        address, expect[idx], org);
                ret = -1;
            }

            d = config[idx];
            sprintf(tmp, "%02X", d);
            *p = tmp[0];
            *(p + 1) = tmp[1];

        } else {
            d = hexstr2val(p);
        }

        sum += d;
        p += 2;
        len -= 2;
        
        address++;
    }

    
    if (len < 2) {
        fprintf(stderr, "invalid format\n");
        return -1;
    }

    sum = 0 - sum;

    sprintf(p, "%02X", sum);

    return ret;
}



int
parse_key(char *buf, unsigned char config[])
{
    char *p = buf;


    // NOTE: size of config is KEY_LEN + 1 and the last entry is used for blink flag
    config[KEY_LEN] = 0;

    // parse key file
    for (int i = 0; i < KEY_LEN; i++) {
        char *p2;
        long val;
        p2 = strchr(p, ',');
        if (p2 != NULL) {
            *p2 = '\0';
        } else if (i < KEY_LEN - 1) {
            // ',' not found and i is not last value
            fprintf(stderr, "Invalid key (key should be %d Byte)\n", KEY_LEN);
            return -1;
        }
        
        errno = 0;
        val = strtol(p, NULL, 16);
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

            // process blink flag
            if (i == KEY_LEN - 1) {
                errno = 0;
                val = strtol(p, &p2, 0);
                if (val != 0)
                    config[KEY_LEN] = 1; // last entry is used for debug flag
            }
        }
    }
    return 1;
}


void
dump(unsigned char *p, int len)
{
    for (int i = 0; i < len; i++) {
        if (i % 16 == 0)
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


int
main(int argc, char **argv)
{
    FILE *fpi, *fpo;
    char buf[256];

    char *templatefile;
    char *keyfile;
    char *outfile;
    unsigned char config[KEY_LEN + 1];
    int ret = -1;
    int key_offset;


    if (!(argc == 1 || argc == 4)) {
        fprintf(stderr, "Invalid argument\n");
        fprintf(stderr, "USAGE: cpicsk_gen\n");
        fprintf(stderr, "USAGE: cpicsk_gen <templatefile> <keyfile> <outfile>\n");
        goto FIN;
    }

    if (argc == 4) {
        templatefile = argv[1];
        keyfile = argv[2];
        outfile = argv[3];
    } else {
        templatefile = TEMPLATE_FILE;
        keyfile = KEY_FILE;
        outfile = OUT_FILE;
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

    if (parse_key(buf, config) < 0) {
        goto FIN;
    }

   
    printf("================================\n");
    printf("Key data:\n");

    dump(config, KEY_LEN);
    printf("Debug flag: %s\n", config[KEY_LEN] == 0 ? "Disabled" : "Enabled");
  
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
        printf("OK\n");
    }

    printf("Write to output file \"%s\" ... ", outfile);
    fflush(stdout);
    

    fpo = fopen(outfile, "wb");
    if (fpo == NULL) {
        perror("fopen");
        fprintf(stderr, "Output file \"%s\" open failed\n", outfile);
        goto FIN;
    }
        
    while (fgets(buf, sizeof(buf) - 1, fpi) > 0) {
        patch_hex(buf, config, key_offset, KEY_INTERVAL, KEY_LEN + 1);
        fprintf(fpo, "%s\r\n", buf);
    }

    printf("OK\n");
    
    fclose(fpo);

    fclose(fpi);

    ret = 0;
    
 FIN:
    fflush(stderr);

    printf("Press any key to exit.\n");

    fflush(stdout);
    
    getchar();
    
    return ret;
}
