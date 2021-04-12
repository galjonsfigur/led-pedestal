#include <stdio.h>
#include <stdint.h>

uint8_t ease8InOutApprox(uint8_t i) {
    if (i < 64) {
        // start with slope 0.5
        i /= 2;
    } else if (i > (255 - 64)) {
        // end with slope 0.5
        i = 255 - i;
        i /= 2;
        i = 255 - i;
    } else {
        // in the middle, use slope 192/128 = 1.5
        i -= 64;
        i += (i / 2);
        i += 32;
    }

    return i;
}


int main () {
	for ( uint16_t i = 0; i < 256; i++ ) {
		printf("%d, %d\n", i, ease8InOutApprox((uint8_t)(i)));
	}
	return 0;
}
