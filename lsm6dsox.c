// === INCLUDES ================================================================

#include "lsm6dsox.h"
#include "lsm6dsox_reg.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>


// === DEFINES =================================================================

/// Number of samples to collect during the gyro self-test.
#define GYRO_SELF_TEST_SAMPLES                              (5u)

/// Amount of time to wait during the gyro self-test after changing gyro config
/// registers.
#define GYRO_SELF_TEST_WAIT_MS                              (100u)

/// The minimum angular rate self-test output change at 250 dps.
#define GRYO_SELF_TEST_250_DPS_OUTPUT_CHANGE_MIN            (20u)

/// The maximum angular rate self-test output change at 250 dps.
#define GRYO_SELF_TEST_250_DPS_OUTPUT_CHANGE_MAX            (80u)

/// The minimum angular rate self-test output change at 2000 dps.
#define GRYO_SELF_TEST_2000_DPS_OUTPUT_CHANGE_MIN           (150u)

/// The maximum angular rate self-test output change at 2000 dps.
#define GRYO_SELF_TEST_2000_DPS_OUTPUT_CHANGE_MAX           (700u)

/// Mask to enable gyro wraparound via the ROUNDING field in the CTRL5_C
/// register.
#define CTRL5_C_ROUNDING_EN_GYRO_MASK                       (0x02)

/// Mask to apply to the register address when performing a SPI read.
#define SPI_READ_REG_MASK                                   (0x80)


// === TYPE DEFINES ============================================================

/// @brief Enum of the different gyro self test modes.
typedef enum
{
    /// @brief Positive-sign self test.
    gyroSelfTestMode_positiveSign = 1,

    /// @brief Negative-sign self test.
    gyroSelfTestMode_negativeSign = 3,

} gyroSelfTestMode_t;

/// @brief Enum of the different gyro operation states.
typedef enum
{
    /// @brief The gyro is powered off.
    gyroState_poweredOff,

    /// @brief The gyro is powered on, but data is not ready.
    gyroState_onDataNotReady,

    /// @brief The gyro is powered on and the the data is ready.
    gyroState_onDataReady,

} gyroState_t;

/// @brief Union of the LSm6DSOX raw data to account for the x, y, and z axes.
///        This allows for convenient conversion of retrieving individual data
///        bytes and combining low and high bytes into the different axes values
///        without having to perform shift and mask operations.
typedef union
{
    /// @brief Raw data representation of the x, y, and z axes (as int16_t).
    lsm6dsox_rawData_t rawData;

    /// @brief The rawData field broken down a byte array.
    uint8_t array[sizeof(lsm6dsox_rawData_t)];

} imuData_t;

/// @brief Struct that aggregates the LSM6DSOX configuration settings.
typedef struct
{
    /// @brief The current gyro state.
    gyroState_t gyroState;

    /// @brief The gyro configuration (full-scale selection and output data
    ///        rate).
    lsm6dsox_gyroConfig_t gyroConfig;

} imuConfig_t;

/// @brief Struct that aggregates the different settings pertaining to the
///        different gyro self test configurations.
typedef struct
{
    /// @brief The gyro's self-test mode.
    gyroSelfTestMode_t mode;

    /// @brief The full-scale gyro configuration.
    lsm6dsox_gyroConfig_fs_t fullScale;

    /// @brief The minimum self-test output change value (in dps).
    uint16_t outputChangeMin;

    /// @brief The maximum self-test output change value (in dps).
    uint16_t outputChangeMax;

} settingsGyroSelfTest_t;


// === STATIC CONSTANTS ========================================================

/// @brief Lookup table for gyro self-test settings based on the gyro self-test
///        config.
static settingsGyroSelfTest_t const G_SettingsGyroSelfTest[] =
{
    [lsm6dsox_gyroSelfTestMode_250_dps_positive] =
    {
        .mode = gyroSelfTestMode_positiveSign,
        .fullScale = lsm6dsox_gyroConfig_fs_250_dps,
        .outputChangeMin = GRYO_SELF_TEST_250_DPS_OUTPUT_CHANGE_MIN,
        .outputChangeMax = GRYO_SELF_TEST_250_DPS_OUTPUT_CHANGE_MAX,
    },
    [lsm6dsox_gyroSelfTestMode_250_dps_negative] =
    {
        .mode = gyroSelfTestMode_negativeSign,
        .fullScale = lsm6dsox_gyroConfig_fs_250_dps,
        .outputChangeMin = GRYO_SELF_TEST_250_DPS_OUTPUT_CHANGE_MIN,
        .outputChangeMax = GRYO_SELF_TEST_250_DPS_OUTPUT_CHANGE_MAX,
    },
    [lsm6dsox_gyroSelfTestMode_2000_dps_positive] =
    {
        .mode = gyroSelfTestMode_positiveSign,
        .fullScale = lsm6dsox_gyroConfig_fs_2000_dps,
        .outputChangeMin = GRYO_SELF_TEST_2000_DPS_OUTPUT_CHANGE_MIN,
        .outputChangeMax = GRYO_SELF_TEST_2000_DPS_OUTPUT_CHANGE_MAX,
    },
    [lsm6dsox_gyroSelfTestMode_2000_dps_negative] =
    {
        .mode = gyroSelfTestMode_negativeSign,
        .fullScale = lsm6dsox_gyroConfig_fs_2000_dps,
        .outputChangeMin = GRYO_SELF_TEST_2000_DPS_OUTPUT_CHANGE_MIN,
        .outputChangeMax = GRYO_SELF_TEST_2000_DPS_OUTPUT_CHANGE_MAX,
    },
};


// === GLOBAL VARIABLES (STATIC SCOPE) =========================================

/// @brief Flag indicating if the LSM6DSOX has started, specifically the comm
///        interface to the LSM6DSOX is active (true).
static bool g_started = false;

/// @brief The local configuration of the LSM6DSOX.
static imuConfig_t g_config =
{
    .gyroState = gyroState_poweredOff,
    .gyroConfig =
    {
        .fullScale = lsm6dsox_gyroConfig_fs_125_dps,
        .outputDataRate = lsm6dsox_gyroConfig_odr_12p5_Hz,
    },
};

/// @brief The gyro self-test result of the last performed gyro self-test.
static lsm6dsox_gyroSelfTestResult_t g_gyroSelfTestResult = lsm6dsox_gyroSelfTestResult_none;


// === DEPENDENCIES FUNCTION PROTOTYPES ========================================

/// @brief Start and power on the communication interface to the LSM6DSOX.
/// @param  
extern void lsm6dsoxComm_start(void);

/// @brief Stop and power off the communication interface to the LSM6DSOX.
/// @param  
extern void lsm6dsoxComm_stop(void);

/// @brief Perform a read operation from the communication interface to the
///        LSM6DSOX.
/// @param dataPtr Pointer to location to store the read data.
/// @param dataLength Number of bytes to read.
/// @return The number of bytes that were read.
extern uint16_t lsm6dsoxComm_read(uint8_t* dataPtr, uint16_t dataLength);

/// @brief Perform a write operation to the communication interface to the
///        LSM6DSOX.
/// @param dataPtr Pointer to the data to write.
/// @param dataLength Number of bytes to write.
/// @return The number of bytes that were written.
extern uint16_t lsm6dsoxComm_write(uint8_t const* dataPtr, uint16_t dataLength);

/// @brief Perform a blocking wait for the specified amount of time.
/// @param time_ms The amount of time to wait in milliseconds.
extern void delay_ms(uint32_t time_ms);


// === PRIVATE FUNCTIONS =======================================================

/// @brief Write an 8-bit value to the specified LSM6DSOX register.
/// @param reg Address of the register to write to.
/// @param val The 8-bit value to write.
static void writeRegister(lsm6dsox_reg_t reg, uint8_t val)
{
    uint8_t temp[2] =
    {
        [0] = reg,
        [1] = val,
    };
    lsm6dsoxComm_write(temp, sizeof(temp));
}

/// @brief Read the 8-bit value at the specified LSM6DSOX register.
/// @param reg Address of the register to read from.
/// @return The 8-bit value at the register.
static uint8_t readRegister(lsm6dsox_reg_t reg)
{
    uint8_t temp = reg | SPI_READ_REG_MASK;
    lsm6dsoxComm_write(&temp, sizeof(temp));
    uint8_t value = 0u;
    lsm6dsoxComm_read(&value, sizeof(value));
    return value;
}

/// @brief Read multiple bytes at the specified LSM6DSOX register.
/// @param reg Address of the register to read from.
/// @param dataPtr Pointer to location to store the read data.
/// @param dataLength Number of bytes to read.
/// @return The number of bytes that were read.
static uint16_t readRegisterMulti(lsm6dsox_reg_t reg, uint8_t* dataPtr, uint16_t dataLength)
{
    if ((dataPtr == NULL) || (dataLength == 0u))
        return 0u;

    uint8_t temp = reg | SPI_READ_REG_MASK;
    lsm6dsoxComm_write(&temp, sizeof(temp));
    return lsm6dsoxComm_read(dataPtr, dataLength);
}

/// @brief Check the WHO_AM_I register of the LSM6DSOX to confirm communication.
/// @param  
/// @return True if the value at WHO_AM_I matches the expected value; otherwise,
///         false.
static bool confirmWhoAmI(void)
{
    uint8_t value = readRegister(lsm6dsox_reg_WHO_AM_I);
    return (value == 0x6c);
}

/// @brief Update the CTRL2_G register value in order to configure the specified
///        full-scale selection.
/// @param ctrl2_g_ptr Pointer to the lms6dsox_reg_CTRL2_G_t structure that
///                    represents the value in the CTRL2_G register. 
/// @param fullScale The full-scale selection value.
static void updateGyroCtrlRegFullScale(lsm6dsox_reg_CTRL2_G_t* const ctrl2_g_ptr, lsm6dsox_gyroConfig_fs_t fullScale)
{
    // modify the full-scale selection
    switch (fullScale)
    {
        case lsm6dsox_gyroConfig_fs_125_dps:
            ctrl2_g_ptr->FS_125 = 1u;
            ctrl2_g_ptr->FS_G = 0u;
            break;
        case lsm6dsox_gyroConfig_fs_250_dps:
            ctrl2_g_ptr->FS_125 = 0u;
            ctrl2_g_ptr->FS_G = 0u;
            break;
        case lsm6dsox_gyroConfig_fs_500_dps:
            ctrl2_g_ptr->FS_125 = 0u;
            ctrl2_g_ptr->FS_G = 1u;
            break;
        case lsm6dsox_gyroConfig_fs_1000_dps:
            ctrl2_g_ptr->FS_125 = 0u;
            ctrl2_g_ptr->FS_G = 2u;
            break;
        case lsm6dsox_gyroConfig_fs_2000_dps:
            ctrl2_g_ptr->FS_125 = 0u;
            ctrl2_g_ptr->FS_G = 3u;
            break;
    }
}

/// @brief Converts the raw gyro value (in LSB) to angular rate (in dps).
/// @param data The raw gyro value (in LSB).
/// @return The angular rate (in dps).
static float convertGyroDataRawToDPS(int16_t data)
{
    static float const Divisor = 1000.0f;
    float factorDPS = 0.0f;
    switch (g_config.gyroConfig.fullScale)
    {
        case lsm6dsox_gyroConfig_fs_125_dps:
            factorDPS = 4.375f / Divisor;
            break;
        case lsm6dsox_gyroConfig_fs_250_dps:
            factorDPS = 8.75f / Divisor;
            break;
        case lsm6dsox_gyroConfig_fs_500_dps:
            factorDPS = 17.5f / Divisor;
            break;
        case lsm6dsox_gyroConfig_fs_1000_dps:
            factorDPS = 35.0f / Divisor;
            break;
        case lsm6dsox_gyroConfig_fs_2000_dps:
            factorDPS = 70.0f / Divisor;
            break;
    }
    float dataDPS = data * factorDPS;
    return dataDPS;
}

/// @brief Checks the STATUS_REG to see if LSM6DSOX has sampled new gyro data.
/// @param  
/// @return True if the data is ready; otherwise, false if the data is not
///         ready.
static bool isGyroDataReady(void)
{
    lsm6dsox_reg_STATUS_REG_t status_reg;
    status_reg.value = readRegister(lsm6dsox_reg_STATUS_REG);
    return (status_reg.GDA == 1u);
}

/// @brief Checks the STATUS_REG to see if LSM6DSOX sampled new gyro data within
///        the specified timeout.
/// @param timeout_ms The max amount of time to check STATUS_REG for the data to
///                   be ready before giving up. If this value is 0, then
///                   function only checks the STATUS_REG once.
/// @return True if the data is ready; otherwise, false if the data is not ready
///         and the function timed out.
static bool isGyroDataReadyWithTimeout(uint16_t timeout_ms)
{
    // The time (in milliseconds) to wait in each iteration of the loop before
    // checking the STATUS_REG again.
    uint16_t wait_ms = (timeout_ms / 4u) + 1u;
    // The total time spent waiting in the function to check if the gyro data is
    // ready.
    uint32_t time_ms = 0u;
    do
    {
        if (isGyroDataReady() == true)
            return true;
        delay_ms(wait_ms);
        time_ms += wait_ms;
    } while (time_ms <= timeout_ms);

    return false;
}

/// @brief Checks the STATUS_REG to see if the IMU has sampled new gyro data.
///        This function differs from isGyroDataReady by not needing the
///        timeout_ms parameter; this function instead uses a recommended
///        timeout given the current gyro's output data rate.
/// @param  
/// @return True if the data is ready; otherwise, false if the data is not ready
///         and the function timed out.
static bool isGyroDataReadyForCurrentConfig(void)
{
    // Table of timeouts (in milliseconds) for all possible output data rate
    // (ODR) selections. These are calculated by taking the taking the period in
    // milliseconds of the ODR and adding 25%. For example, for an ODR of 12.5:
    // timeout = 1000 * (1/ODR) * 1.25
    // timeout = 1000 * (1/12.5) * 1.25 = 100 ms
    static uint16_t const G_GyroDataReadyTimeout_ms[] =
    {
        [lsm6dsox_gyroConfig_odr_off] = 0u,
        [lsm6dsox_gyroConfig_odr_12p5_Hz] = 100u,
        [lsm6dsox_gyroConfig_odr_26_Hz] = 48u,
        [lsm6dsox_gyroConfig_odr_52_Hz] = 24u,
        [lsm6dsox_gyroConfig_odr_104_Hz] = 12u,
        [lsm6dsox_gyroConfig_odr_208_Hz] = 6u,
        [lsm6dsox_gyroConfig_odr_416_Hz] = 3u,
        [lsm6dsox_gyroConfig_odr_833_Hz] = 2u,
        [lsm6dsox_gyroConfig_odr_1666_Hz] = 1u,
        [lsm6dsox_gyroConfig_odr_3332_Hz] = 1u,
        [lsm6dsox_gyroConfig_odr_6664_Hz] = 1u,
    };

    return isGyroDataReadyWithTimeout(G_GyroDataReadyTimeout_ms[g_config.gyroConfig.outputDataRate]);
}

/// @brief Calculate the average raw gyro data for the specified number of
///        samples.
/// @param samples The number of samples to calculate the average raw gyro data
///                of.
/// @param dataPtr Pointer of the lsm6dsox_rawData_t structure to store the
///                average results.
/// @return True if the average was successfully calculated; otherwise, false if
///         the system timed out while acquiring sample data.
static bool getAverageGyroDataRaw(uint16_t samples, lsm6dsox_rawData_t *const dataPtr)
{
    int32_t xTotal = 0u;
    int32_t yTotal = 0u;
    int32_t zTotal = 0u;
    for (uint8_t index = 0u; index < samples; ++index)
    {
        if (isGyroDataReadyForCurrentConfig() == false)
            return false;
        lsm6dsox_rawData_t data;
        lsm6dsox_getGyroDataRaw(&data);
        xTotal += data.x;
        yTotal += data.y;
        zTotal += data.z;
    }
    dataPtr->x = xTotal / samples;
    dataPtr->y = yTotal / samples;
    dataPtr->z = zTotal / samples;

    return true;
}


// === PUBLIC FUNCTIONS ========================================================

/// @brief Enable communication to the LSM6DSOX and configure it based on the
///        last configuration settings before stopping it.
/// @param  
void lsm6dsox_start(void)
{
    if (g_started == true)
        return;

    // Start and power on the communication to the LSM6DSOX.
    lsm6dsoxComm_start();
    // Verify communication to LSM6DSOX by checking the WHO_AM_I register.
    if (confirmWhoAmI() == false)
    {
        // Since communication to the LSM6DSOX could not be confirmed, stop and
        // power off the communication to the LSM6DSOX.
        lsm6dsoxComm_stop();
        return;
    }
    g_started = true;

    g_config.gyroState = gyroState_onDataNotReady;
    lsm6dsox_updateGyroConfig(g_config.gyroConfig);
}

/// @brief Disable communication to the LSM6DSOX and disable all functionality
///        (specifically the gyro).
/// @param  
void lsm6dsox_stop(void)
{
    if (g_started == false)
        return;

    // Power off the gyro. Directly change the CTRL2_G register and do not
    // modify the g_config global so that the next call to lsm6dsox_start uses
    // the last configuration.
    lsm6dsox_reg_CTRL2_G_t ctrl2_g;
    ctrl2_g.value = readRegister(lsm6dsox_reg_CTRL2_G);
    ctrl2_g.ODR_G = (uint8_t)lsm6dsox_gyroConfig_odr_off;
    writeRegister(lsm6dsox_reg_CTRL2_G, ctrl2_g.value);
    g_config.gyroState = gyroState_poweredOff;
    // Stop and power off the communication to the LSM6DSOX.
    lsm6dsoxComm_stop();
    g_started = false;
}

/// @brief Update the gyro configuration and commit it to the LSM6DSOX
/// @param config The gyro configuration (full-scale selection and output data
///               rate).
/// @return The LSM6DSOX-specific error if the gyro config cannot be updated;
///         if the config was updated, lsm6dsox_err_none.
lsm6dsox_err_t lsm6dsox_updateGyroConfig(lsm6dsox_gyroConfig_t config)
{
    if (g_started == false)
        return lsm6dsox_err_stopped;
    
    // Update the CTRL2_G register (full-scale and output data rate).
    lsm6dsox_reg_CTRL2_G_t ctrl2_g;
    ctrl2_g.value = readRegister(lsm6dsox_reg_CTRL2_G);
    updateGyroCtrlRegFullScale(&ctrl2_g, config.fullScale);
    ctrl2_g.ODR_G = (uint8_t)config.outputDataRate;
    writeRegister(lsm6dsox_reg_CTRL2_G, ctrl2_g.value);

    // Update the CTRL5_C register (rounding for data wraparound).
    lsm6dsox_reg_CTRL5_C_t ctrl5_c;
    ctrl5_c.value = readRegister(lsm6dsox_reg_CTRL5_C);

    // Update the global config.
    if (config.outputDataRate == lsm6dsox_gyroConfig_odr_off)
    {
        g_config.gyroState = gyroState_poweredOff;

        // Disable wraparound for the gyro.
        ctrl5_c.ROUNDING &= ~CTRL5_C_ROUNDING_EN_GYRO_MASK;
    }
    else
    {
        g_config.gyroConfig = config;
        g_config.gyroState = gyroState_onDataNotReady;

        // Enable wraparound for the gyro.
        ctrl5_c.ROUNDING |= CTRL5_C_ROUNDING_EN_GYRO_MASK;
    }

    writeRegister(lsm6dsox_reg_CTRL5_C, ctrl5_c.value);

    return lsm6dsox_err_none;
}

/// @brief Get the full-scale selection of the gyro configuration.
/// @param  
/// @return The full-scale selection as lsm6dsox_gyroConfig_fs_t.
lsm6dsox_gyroConfig_fs_t lsm6dsox_getGyroConfigFullScale(void)
{
    return g_config.gyroConfig.fullScale;
}

/// @brief Get the output data rate selection of the gyro configuration.
/// @param  
/// @return The output data rate selection as lsm6dsox_gyroConfig_odr_t.
lsm6dsox_gyroConfig_odr_t lsm6dsox_getGyroConfigOutputDataRate(void)
{
    return g_config.gyroConfig.outputDataRate;
}

/// @brief Get the most recent raw gyro data (in LSB).
/// @param dataPtr Pointer to the lsm6dsox_rawData_t to put the raw gyro data.
/// @return The LSM6DSOX-specific error if the gyro data cannot be retrieved;
///         if the data can be retrieved, lsm6dsox_err_none.
lsm6dsox_err_t lsm6dsox_getGyroDataRaw(lsm6dsox_rawData_t* const dataPtr)
{
    if (g_started == false)
        return lsm6dsox_err_stopped;
    if (g_config.gyroState == gyroState_poweredOff)
        return lsm6dsox_err_gyroPoweredOff;
    if (dataPtr == NULL)
        return lsm6dsox_err_paramNull;

    if (g_config.gyroState == gyroState_onDataNotReady)
    {
        if (isGyroDataReady() == false)
            return lsm6dsox_err_gyroDataNotReady;
        // Only get get here if the gyro data is ready, so update the state.
        g_config.gyroState = gyroState_onDataReady;
    }

    imuData_t data;
    readRegisterMulti(lsm6dsox_reg_OUTX_L_G, data.array, sizeof(imuData_t));
    *dataPtr = data.rawData;

    return lsm6dsox_err_none;
}

/// @brief Get the most recent gyro data in dps; this is also known as the
///        angular rate.
/// @param dataPtr Pointer to the lsm6dsox_scaledData_t to put the gyro data in
///                dps.
/// @return The LSM6DSOX-specific error if the gyro data cannot be retrieved;
///         if the data can be retrieved, lsm6dsox_err_none.
lsm6dsox_err_t lsm6dsox_getGyroDataDPS(lsm6dsox_scaledData_t* const dataPtr)
{
    lsm6dsox_rawData_t data;
    lsm6dsox_err_t err = lsm6dsox_getGyroDataRaw(&data);
    if (err != lsm6dsox_err_none)
        return err;

    lsm6dsox_scaledData_t dataDPS = lsm6dsox_convertGyroDataRawToDPS(data);
    *dataPtr = dataDPS;

    return err;
}

/// @brief Conversion function to scale the raw gyro data in LSB to dps.
/// @param data The raw gyro data to convert.
/// @return The converted data in the scaled units.
lsm6dsox_scaledData_t lsm6dsox_convertGyroDataRawToDPS(lsm6dsox_rawData_t data)
{
    lsm6dsox_scaledData_t dataDPS =
    {
        .x = convertGyroDataRawToDPS(data.x),
        .y = convertGyroDataRawToDPS(data.y),
        .z = convertGyroDataRawToDPS(data.z),
    };
    return dataDPS;
}

/// @brief Start the gyro self-test based on the gyro self-test config.
/// @param config The gyro self-test config (specifically the self-test mode and
///               the full-scale selection).
/// @return The LSM6DSOX-specific error if the self-test failed to complete;
///         if the self-test completed, lsm6dsox_err_none.
lsm6dsox_err_t lsm6dsox_startGyroSelfTest(lsm6dsox_gyroSelfTestConfig_t config)
{
    if (g_started == false)
        return lsm6dsox_err_stopped;

    // Retain the CTRL2_G and CTRL5_C registers to revert them when the self-
    // test is complete.
    lsm6dsox_reg_CTRL2_G_t ctrl2_g_orig;
    ctrl2_g_orig.value = readRegister(lsm6dsox_reg_CTRL2_G);
    lsm6dsox_reg_CTRL5_C_t ctrl5_c_orig;
    ctrl5_c_orig.value = readRegister(lsm6dsox_reg_CTRL5_C);
    
    lsm6dsox_err_t err = lsm6dsox_err_none;
    lsm6dsox_rawData_t noSelfTestData;
    lsm6dsox_rawData_t selfTestData;
    // Encompass the following block of code in a do-while loop to allow for
    // early breaks in the case of an error.
    do
    {
        // Configure the full-scale selection.
        g_gyroSelfTestResult = lsm6dsox_gyroSelfTestResult_running;
        lsm6dsox_reg_CTRL2_G_t ctrl2_g;
        ctrl2_g.value = readRegister(lsm6dsox_reg_CTRL2_G);
        lsm6dsox_reg_CTRL2_G_t ctrl2_g_orig = ctrl2_g;
        updateGyroCtrlRegFullScale(&ctrl2_g, G_SettingsGyroSelfTest[config].fullScale);
        writeRegister(lsm6dsox_reg_CTRL2_G, ctrl2_g.value);
        delay_ms(GYRO_SELF_TEST_WAIT_MS);
        if (isGyroDataReadyForCurrentConfig() == false)
        {
            err = lsm6dsox_err_timedOut;
            break;
        }
        lsm6dsox_rawData_t data;
        lsm6dsox_getGyroDataRaw(&data);

        // get the average no self-test accelerometer values
        if (getAverageGyroDataRaw(GYRO_SELF_TEST_SAMPLES, &noSelfTestData) == false)
        {
            err = lsm6dsox_err_timedOut;
            break;
        }

        // start the self test
        lsm6dsox_reg_CTRL5_C_t ctrl5_c;
        ctrl5_c.value = readRegister(lsm6dsox_reg_CTRL5_C);
        lsm6dsox_reg_CTRL5_C_t ctrl5_g_orig = ctrl5_c;
        ctrl5_c.ST_G = (uint8_t)G_SettingsGyroSelfTest[config].mode;
        ctrl5_c.ROUNDING |= CTRL5_C_ROUNDING_EN_GYRO_MASK;
        writeRegister(lsm6dsox_reg_CTRL5_C, ctrl5_c.value);
        delay_ms(GYRO_SELF_TEST_WAIT_MS);
        if (isGyroDataReadyForCurrentConfig() == false)
        {
            err = lsm6dsox_err_timedOut;
            break;
        }
        lsm6dsox_getGyroDataRaw(&data);

        // get the average self-test accelerometer values
        if (getAverageGyroDataRaw(GYRO_SELF_TEST_SAMPLES, &selfTestData) == false)
        {
            err = lsm6dsox_err_timedOut;
            break;
        }
    } while (false);

    // restore registers
    writeRegister(lsm6dsox_reg_CTRL5_C, ctrl5_c_orig.value);
    writeRegister(lsm6dsox_reg_CTRL2_G, ctrl2_g_orig.value);

    // handle timeout errors
    if (err == lsm6dsox_err_timedOut)
    {
        g_gyroSelfTestResult = lsm6dsox_gyroSelfTestResult_none;
        return err;
    }

    // determine if the self test passed or failed
    uint32_t deltas[] =
    {
        [0] = abs(selfTestData.x - noSelfTestData.x),
        [1] = abs(selfTestData.y - noSelfTestData.y),
        [2] = abs(selfTestData.z - noSelfTestData.z),
    };
    uint8_t const NumDeltas = sizeof(deltas) / sizeof(deltas[0]);
    bool passed = true;
    for (uint8_t index = 0u; index < NumDeltas; ++index)
    {
        passed = passed
            && (deltas[index] >= G_SettingsGyroSelfTest[config].outputChangeMin)
            && (deltas[index] <= G_SettingsGyroSelfTest[config].outputChangeMax);
    }
    g_gyroSelfTestResult = (passed == false) ?
        (lsm6dsox_gyroSelfTestResult_failed) : (lsm6dsox_gyroSelfTestResult_passed);

    return err;
}

/// @brief Get the gyro self-test result of the last attempted gyro self-test.
///        Note: this function can be called asynchronously and the return value
///        will indicate if a self test is currently running.
/// @param  
/// @return The gyro self-test result of the last attempted gyro self-test, see
///         lsm6dsox_gyroSelfTestResult_t.
lsm6dsox_gyroSelfTestResult_t lsm6dsox_getGyroSelfTestResult(void)
{
    return g_gyroSelfTestResult;
}
