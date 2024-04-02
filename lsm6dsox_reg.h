#ifndef LSM6DSOX_REG_H
#define LSM6DSOX_REG_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// === DEPENDENCIES ============================================================

#include <stdint.h>


// === TYPE DEFINES: REGISTER ADDRESSES ========================================

/// @brief Enumeration of the different registers of the LSM6DSOX. Note that the
///        enum value corresponds to the register address.
typedef enum
{
    /// @brief WHO_AM_I register (r). This is a read-only register. Its value is
    ///        fixed at 6Ch.
    lsm6dsox_reg_WHO_AM_I = 0x0f,

    /// @brief Gyroscope control register 2 (r/w).
    lsm6dsox_reg_CTRL2_G = 0x11,

    /// @brief Control register 5 (r/w).
    lsm6dsox_reg_CTRL5_C = 0x14,

    /// @brief The STATUS_REG register is read by the primary interface SPI/I²C
    /// & MIPI I3CSM (r).
    lsm6dsox_reg_STATUS_REG = 0x1e,

    /// @brief Angular rate sensor pitch axis (X) angular rate output register
    ///        (r). The value is expressed as a 16-bit word in two’s complement.
    ///        Low-byte.
    lsm6dsox_reg_OUTX_L_G = 0x22,

    /// @brief Angular rate sensor pitch axis (X) angular rate output register
    ///        (r). The value is expressed as a 16-bit word in two’s complement.
    ///        High-byte.
    lsm6dsox_reg_OUTX_H_G = 0x23,

    /// @brief Angular rate sensor pitch axis (Y) angular rate output register
    ///        (r). The value is expressed as a 16-bit word in two’s complement.
    ///        Low-byte.
    lsm6dsox_reg_OUTY_L_G = 0x24,

    /// @brief Angular rate sensor pitch axis (Y) angular rate output register
    ///        (r). The value is expressed as a 16-bit word in two’s complement.
    ///        High-byte.
    lsm6dsox_reg_OUTY_H_G = 0x25,

    /// @brief Angular rate sensor pitch axis (Z) angular rate output register
    ///        (r). The value is expressed as a 16-bit word in two’s complement.
    ///        Low-byte.
    lsm6dsox_reg_OUTZ_L_G = 0x26,

    /// @brief Angular rate sensor pitch axis (Z) angular rate output register
    ///        (r). The value is expressed as a 16-bit word in two’s complement.
    ///        High-byte.
    lsm6dsox_reg_OUTZ_H_G = 0x27,

} lsm6dsox_reg_t;


// === TYPE DEFINES: REGISTER STRUCTURE ========================================

/// @brief Union that describes each of the fields in the CTRL2_G register.
typedef union
{
    /// @brief 8-bit representation of the register.
    uint8_t value;

    /// @brief Register broken down into the different fields.
    struct
    {
        uint8_t unused : 1; // unused
        uint8_t FS_125 : 1; // Selects gyro UI chain full-scale 125 dps.
        uint8_t FS_G : 2;   // Gyroscope UI chain full-scale selection.
        uint8_t ODR_G : 4;  // Gyroscope output data rate selection.
    };

} lsm6dsox_reg_CTRL2_G_t;

/// @brief Union that describes each of the fields in the CTRL5_C register.
typedef union
{
    /// @brief 8-bit representation of the register.
    uint8_t value;

    struct
    {
        uint8_t ST_XL : 2;              // Linear acceleration sensor self-test enable.
        uint8_t ST_G : 2;               // Angular rate sensor self-test enable.
        uint8_t ROUNDING_STATUS : 1;    // Source register rounding function.
        uint8_t ROUNDING : 2;           // Circular burst-mode (rounding) read from the output registers.
        uint8_t XL_ULP_EN : 1;          // Accelerometer ultra-low-power mode enable.
    };

} lsm6dsox_reg_CTRL5_C_t;

/// @brief Union that describes each of the fields in the STATUS_REG register.
typedef union
{
    /// @brief 8-bit representation of the register.
    uint8_t value;

    struct
    {
        uint8_t XLDA : 1;   // Accelerometer new data available.
        uint8_t GDA : 1;    // Gyroscope new data available.
        uint8_t TDA : 1;    // Temperature new data available.
        uint8_t unused : 5; // unused
    };

} lsm6dsox_reg_STATUS_REG_t;


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LSM6DSOX_REG_H
