#ifndef LSM6DSOX_H
#define LSM6DSOX_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// === DEPENDENCIES ============================================================

#include <stdint.h>


// === TYPE DEFINES ============================================================

/// @brief Enumerations of all the possible causes of errors when doing
///        something on the LSM6DSOX. These are typically used as return values
///        for functions.
typedef enum
{
    /// @brief No error occurred.
    lsm6dsox_err_none = 0,

    /// @brief Error caused by a parameter being NULL which is not valid.
    lsm6dsox_err_paramNull,

    /// @brief Error caused by a length parameter having an invalid length
    ///        value.
    lsm6dsox_err_paramInvalidLength,

    /// @brief Error caused by performing an action that requires functioning
    ///        communication with the LSM6DSOX but it has been stopped so it's
    ///        not possible to communicate with it.
    lsm6dsox_err_stopped,

    /// @brief Error caused by attempting to perform a gyro-related function but
    ///        the gyro is current powered down.
    lsm6dsox_err_gyroPoweredOff,

    /// @brief Error caused by attempting to get gyro data, but the gyro data is
    ///        not ready yet.
    lsm6dsox_err_gyroDataNotReady,

    /// @brief Error caused by attempting an action that timed out.
    lsm6dsox_err_timedOut,

} lsm6dsox_err_t;

/// @brief Enumeration of the different possible gyro self-test configs.
typedef enum
{
    /// @brief Full-scale = 250 dps; positive sign self test.
    lsm6dsox_gyroSelfTestMode_250_dps_positive = 0,

    /// @brief Full-scale = 250 dps; negative sign self test.
    lsm6dsox_gyroSelfTestMode_250_dps_negative,

    /// @brief Full-scale = 2000 dps; positive sign self test.
    lsm6dsox_gyroSelfTestMode_2000_dps_positive,

    /// @brief Full-scale = 2000 dps; negative sign self test.
    lsm6dsox_gyroSelfTestMode_2000_dps_negative,

} lsm6dsox_gyroSelfTestConfig_t;

/// @brief Enumeration of the different possible results from performing a gyro
///        self-test.
typedef enum
{
    /// @brief A gyro self-test has not been performed or the last self-test
    ///        failed to complete.
    lsm6dsox_gyroSelfTestResult_none = 0,

    /// @brief A gyro self-test is currently running; this option can occur
    ///        if the self-test result is requested from another thread or an
    ///        interrupt.
    lsm6dsox_gyroSelfTestResult_running,

    /// @brief The gyro self-test passed successfully.
    lsm6dsox_gyroSelfTestResult_passed,

    /// @brief The gyro self-test failed.
    lsm6dsox_gyroSelfTestResult_failed,

} lsm6dsox_gyroSelfTestResult_t;

/// @brief Enumeration of the different gyro full-scale options for the gyro
///        config.
typedef enum
{
    /// The gyro's angular rate full range is from -125 dps to +125 dps.
    lsm6dsox_gyroConfig_fs_125_dps = 0,

    /// The gyro's angular rate full range is from -250 dps to +250 dps.
    lsm6dsox_gyroConfig_fs_250_dps,

    /// The gyro's angular rate full range is from -500 dps to +500 dps.
    lsm6dsox_gyroConfig_fs_500_dps,

    /// The gyro's angular rate full range is from -1000 dps to +1000 dps.
    lsm6dsox_gyroConfig_fs_1000_dps,

    /// The gyro's angular rate full range is from -2000 dps to +2000 dps.
    lsm6dsox_gyroConfig_fs_2000_dps,

} lsm6dsox_gyroConfig_fs_t;

/// @brief Enumeration of the different output data rates for the gyro config.
typedef enum
{
    /// @brief The gyro is powered off (not running).
    lsm6dsox_gyroConfig_odr_off = 0,

    /// @brief The gyro output data rate is 12.5 Hz.
    lsm6dsox_gyroConfig_odr_12p5_Hz = 1,

    /// @brief The gyro output data rate is 26 Hz.
    lsm6dsox_gyroConfig_odr_26_Hz = 2,

    /// @brief The gyro output data rate is 52 Hz.
    lsm6dsox_gyroConfig_odr_52_Hz = 3,

    /// @brief The gyro output data rate is 104 Hz.
    lsm6dsox_gyroConfig_odr_104_Hz = 4,

    /// @brief The gyro output data rate is 208 Hz.
    lsm6dsox_gyroConfig_odr_208_Hz = 5,

    /// @brief The gyro output data rate is 416 Hz.
    lsm6dsox_gyroConfig_odr_416_Hz = 6,

    /// @brief The gyro output data rate is 833 Hz.
    lsm6dsox_gyroConfig_odr_833_Hz = 7,

    /// @brief The gyro output data rate is 1666 Hz.
    lsm6dsox_gyroConfig_odr_1666_Hz = 8,

    /// @brief The gyro output data rate is 3332 Hz.
    lsm6dsox_gyroConfig_odr_3332_Hz = 9,

    /// @brief The gyro output data rate is 6664 Hz.
    lsm6dsox_gyroConfig_odr_6664_Hz = 10,

} lsm6dsox_gyroConfig_odr_t;

/// @brief Struct that aggregates the gyro configuration parameters.
typedef struct
{
    /// @brief The full-scale selection.
    lsm6dsox_gyroConfig_fs_t fullScale;

    /// @brief The output data rate.
    lsm6dsox_gyroConfig_odr_t outputDataRate;

} lsm6dsox_gyroConfig_t;

/// @brief Struct that aggregates the raw data values for each axis of the
///        accelerometer or gyro measurements. Note that this can be used for
///        both accelerometer and gyroscope data.
typedef struct
{
    /// @brief X-axis raw data value (units of LSB).
    int16_t x;

    /// @brief Y-axis raw data value (units of LSB).
    int16_t y;

    /// @brief Z-axis raw data value (units of LSB).
    int16_t z;

} lsm6dsox_rawData_t;

/// @brief Struct that aggregates the scaled data values for each axis of the
///        accelerometer or gyro measurements. Note that this can be used for
///        both accelerometer and gyroscope data; but depending on the sensor,
///        the unit changes.
///        accelerometer = mg (where g = gravity)
///        gyroscope = dps (degrees-per-second)
typedef struct
{
    /// @brief X-axis scaled value.
    float x;

    /// @brief Y-axis scaled value.
    float y;

    /// @brief Z-axis scaled value.
    float z;
    
} lsm6dsox_scaledData_t;


// === FUNCTION PROTOTYPES =====================================================

void lsm6dsox_start(void);
void lsm6dsox_stop(void);
lsm6dsox_err_t lsm6dsox_updateGyroConfig(lsm6dsox_gyroConfig_t config);
lsm6dsox_gyroConfig_fs_t lsm6dsox_getGyroConfigFullScale(void);
lsm6dsox_gyroConfig_odr_t lsm6dsox_getGyroConfigOutputDataRate(void);
lsm6dsox_err_t lsm6dsox_getGyroDataRaw(lsm6dsox_rawData_t* const dataPtr);
lsm6dsox_err_t lsm6dsox_getGyroDataDPS(lsm6dsox_scaledData_t* const dataPtr);
lsm6dsox_scaledData_t lsm6dsox_convertGyroDataRawToDPS(lsm6dsox_rawData_t data);
lsm6dsox_err_t lsm6dsox_startGyroSelfTest(lsm6dsox_gyroSelfTestConfig_t config);
lsm6dsox_gyroSelfTestResult_t lsm6dsox_getGyroSelfTestResult(void);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LSM6DSOX_H
