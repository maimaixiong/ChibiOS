/*
 * Usage:
 *
 *  gcc vw_crc.c -o a
 *  ./a
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t crc8_lut_8h2f[256];

void gen_crc_lookup_table(uint8_t poly, uint8_t crc_lut[]) {
    uint8_t crc;
    int i, j;

    for (i = 0; i < 256; i++) {
        crc = i;
        for (j = 0; j < 8; j++) {
        if ((crc & 0x80) != 0)
            crc = (uint8_t)((crc << 1) ^ poly);
        else
            crc <<= 1;
        }
        crc_lut[i] = crc;
    }
}

/*
 * Exsample:
 *  uint8_t data=[0x01, 0x02, 0x03 ,x04 ,0x05, 0x06, 0x07, 0x08]
 *
 *  return :
 *  uint64_t d = 0x0807060504030201
 */

uint64_t read_u64_le(const uint8_t* v) {

      return (  (uint64_t)v[0]
              | ((uint64_t)v[1] << 8)
              | ((uint64_t)v[2] << 16)
              | ((uint64_t)v[3] << 24)
              | ((uint64_t)v[4] << 32)
              | ((uint64_t)v[5] << 40)
              | ((uint64_t)v[6] << 48)
              | ((uint64_t)v[7] << 56));
}



unsigned int vw_crc(unsigned int address, uint8_t *data, int l) {

    uint64_t d;
    uint8_t crc = 0xFF; // Standard init value for CRC8 8H2F/AUTOSAR

    d = read_u64_le(data);

    // CRC the payload first, skipping over the first byte where the CRC lives.
    for (int i = 1; i < l; i++) {
        crc ^= (d >> (i*8)) & 0xFF;
        crc = crc8_lut_8h2f[crc];
    }

    // Look up and apply the magic final CRC padding byte, which permutes by CAN
    // address, and additionally (for SOME addresses) by the message counter.
    uint8_t counter = ((d >> 8) & 0xFF) & 0x0F;
    crc ^= (uint8_t[]){0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA,0xDA}[counter];
    crc = crc8_lut_8h2f[crc];

    return crc ^ 0xFF; // Return after standard final XOR for CRC8 8H2F/AUTOSAR
}


int main(void){
    
    gen_crc_lookup_table(0x2f, crc8_lut_8h2f);
    uint8_t data1[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
    uint8_t data2[] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
    
    printf("0x126 %016llx crc: %x\n", read_u64_le(data1), vw_crc(0x126, data1, 8));
    printf("0x166 %016llx crc: %x\n", read_u64_le(data2), vw_crc(0x166, data2, 8));

    return 0;
}
