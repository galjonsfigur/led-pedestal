#include <stdio.h>
#include <stdint.h>

uint8_t triwave8(uint8_t in) {
    if( in & 0x80) {
        in = 255 - in;
    }
    uint8_t out = (uint8_t) (in << 1);
    return out;
}

int main () {
	for ( uint16_t i = 0; i < 256; i++ ) {
		printf("%d, %d\n", i, triwave8((uint8_t)(i)));
	}
	return 0;
}
