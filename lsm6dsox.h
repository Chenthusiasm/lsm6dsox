#ifndef LSM6DSOX_H
#define LSM6DSOX_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// === DEPENDENCIES ============================================================

#include <stdint.h>


// === TYPE DEFINES ============================================================

typedef enum
{
    lsm6dsox_err_none = 0,
    lsm6dsox_err_paramNull,
    lsm6dsox_err_paramInvalidLength,
    lsm6dsox_err_stopped,
    lsm6dsox_err_gyroPoweredOff,
    lsm6dsox_err_timedOut,
} lsm6dsox_err_t;

typedef enum
{
    lsm6dsox_gyroSelfTestMode_250_dps_positive = 0,
    lsm6dsox_gyroSelfTestMode_250_dps_negative,
    lsm6dsox_gyroSelfTestMode_2000_dps_positive,
    lsm6dsox_gyroSelfTestMode_2000_dps_negative,
} lsm6dsox_gyroSelfTestMode_t;

typedef enum
{
    lsm6dsox_gyroSelfTestResult_none = 0,
    lsm6dsox_gyroSelfTestResult_running,
    lsm6dsox_gyroSelfTestResult_passed,
    lsm6dsox_gyroSelfTestResult_failed,
} lsm6dsox_gyroSelfTestResult_t;

typedef enum
{
    lsm6dsox_gyroConfig_fs_125_dps = 0,
    lsm6dsox_gyroConfig_fs_250_dps,
    lsm6dsox_gyroConfig_fs_500_dps,
    lsm6dsox_gyroConfig_fs_1000_dps,
    lsm6dsox_gyroConfig_fs_2000_dps,
} lsm6dsox_gyroConfig_fs_t;

typedef enum
{
    lsm6dsox_gyroConfig_odr_off = 0,
    lsm6dsox_gyroConfig_odr_12p5_Hz = 1,
    lsm6dsox_gyroConfig_odr_26_Hz = 2,
    lsm6dsox_gyroConfig_odr_52_Hz = 3,
    lsm6dsox_gyroConfig_odr_104_Hz = 4,
    lsm6dsox_gyroConfig_odr_208_Hz = 5,
    lsm6dsox_gyroConfig_odr_416_Hz = 6,
    lsm6dsox_gyroConfig_odr_833_Hz = 7,
    lsm6dsox_gyroConfig_odr_1666_Hz = 8,
    lsm6dsox_gyroConfig_odr_3332_Hz = 9,
    lsm6dsox_gyroConfig_odr_6664_Hz = 10,
} lsm6dsox_gyroConfig_odr_t;

typedef struct
{
    lsm6dsox_gyroConfig_fs_t fs;
    lsm6dsox_gyroConfig_odr_t odr;
} lsm6dsox_gyroConfig_t;

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} lsm6dsox_rawData_t;

typedef struct
{
    float x;
    float y;
    float z;
} lsm6dsox_scaledData_t;


// === FUNCTION PROTOTYPES =====================================================

void lsm6dsox_start(void);
void lsm6dsox_stop(void);
lsm6dsox_err_t lsm6dsox_updateGyroConfig(lsm6dsox_gyroConfig_t config);
lsm6dsox_gyroConfig_fs_t lsm6dsox_getGyroConfigFS(void);
lsm6dsox_gyroConfig_odr_t lsm6dsox_getGyroConfigODR(void);
lsm6dsox_err_t lsm6dsox_getGyroDataRaw(lsm6dsox_rawData_t* const dataPtr);
lsm6dsox_err_t lsm6dsox_getGyroDataDPS(lsm6dsox_scaledData_t* const dataPtr);
lsm6dsox_scaledData_t lsm6dsox_convertGyroDataRawToDPS(lsm6dsox_rawData_t data);
lsm6dsox_err_t lsm6dsox_startGyroSelfTest(lsm6dsox_gyroSelfTestMode_t mode);
lsm6dsox_gyroSelfTestResult_t lsm6dsox_getGyroSelfTestResult(void);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LSM6DSOX_H
