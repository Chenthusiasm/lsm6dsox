#ifndef LSM6DSOX_REG_H
#define LSM6DSOX_REG_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// === DEPENDENCIES ============================================================

#include <stdint.h>


// === TYPE DEFINES: REGISTER ADDRESSES ========================================

typedef enum
{
    lsm6dsox_reg_WHO_AM_I = 0x0f,
    lsm6dsox_reg_CTRL2_G = 0x11,
    lsm6dsox_reg_CTRL5_C = 0x14,
    lsm6dsox_reg_STATUS_REG = 0x1e,
    lsm6dsox_reg_OUTX_L_G = 0x22,
    lsm6dsox_reg_OUTX_H_G = 0x23,
    lsm6dsox_reg_OUTY_L_G = 0x24,
    lsm6dsox_reg_OUTY_H_G = 0x25,
    lsm6dsox_reg_OUTZ_L_G = 0x26,
    lsm6dsox_reg_OUTZ_H_G = 0x27,
} lsm6dsox_reg_t;


// === TYPE DEFINES: REGISTER STRUCTURE ========================================

typedef union
{
    uint8_t value;
    struct
    {
        uint8_t unused : 1;
        uint8_t FS_125 : 1;
        uint8_t FS_G : 2;
        uint8_t ODR_G : 4;
    };
} lsm6dsox_reg_CTRL2_G_t;

typedef union
{
    uint8_t value;
    struct
    {
        uint8_t ST_XL : 2;
        uint8_t ST_G : 2;
        uint8_t ROUNDING_STATUS : 1;
        uint8_t ROUNDING : 2;
        uint8_t XL_ULP_EN : 1;
    };
} lsm6dsox_reg_CTRL5_C_t;

typedef union
{
    uint8_t value;
    struct
    {
        uint8_t XLDA : 1;
        uint8_t GDA : 1;
        uint8_t TDA : 1;
        uint8_t unused : 5;
    };
} lsm6dsox_reg_STATUS_REG_t;


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LSM6DSOX_REG_H
