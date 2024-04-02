#include <stdint.h>
#include <stdio.h>

#include "lsm6dsox.h"

// === STUB FUNCTIONS ==========================================================

void lsm6dsoxComm_start(void)
{
    // do nothing (stub function)
}

void lsm6dsoxComm_stop(void)
{
    // do nothing (stub function)
}

uint16_t lsm6dsoxComm_read(uint8_t* data, uint16_t dataLength)
{
    // do nothing (stub function)
    return dataLength;
}

uint16_t lsm6dsoxComm_write(uint8_t const* data, uint16_t dataLength)
{
    // do nothing (stub function)
    return dataLength;
}

void delay_ms(uint32_t time_ms)
{
    // do nothing (stub function)
}


// === MAIN ====================================================================

int main(void)
{
    printf("Starting LSM6DSOX driver test...\n");

    lsm6dsox_start();
    delay_ms(1000u);
    lsm6dsox_stop();
    
    return 0;
}