#include <stdio.h>
#include <stdint.h>

uint8_t quadwave8(uint8_t in) {
    if( in & 0x80) {
        in = 255 - in;
    }
    uint8_t j = (uint8_t)(in << 1);
    if( j & 0x80 ) {
        j = 255 - j;
    }
    uint8_t jj  =  (((uint16_t) j) * (1+(uint16_t)(j))) >> 8;
    jj = (uint8_t)(jj << 1);
    if( in << 1 & 0x80 ) {
        jj = 255 - jj;
    }
    return jj;
}

int main () {
	for ( uint16_t i = 0; i < 256; i++ ) {
		printf("%d, %d\n", i, quadwave8((uint8_t)(i)));
	}
	return 0;
}
