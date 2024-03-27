// === INCLUDES ================================================================

#include "lsm6dsox.h"
#include "lsm6dsox_reg.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>


// === DEFINES =================================================================

#define GYRO_SELF_TEST_SAMPLES                              (5u)

#define GYRO_SELF_TEST_WAIT_MS                              (100u)

#define GYRO_DATA_READY_WAIT_MS                             (10u)

#define GRYO_SELF_TEST_250_DPS_OUTPUT_CHANGE_MIN            (20u)

#define GRYO_SELF_TEST_250_DPS_OUTPUT_CHANGE_MAX            (80u)

#define GRYO_SELF_TEST_2000_DPS_OUTPUT_CHANGE_MIN           (150u)

#define GRYO_SELF_TEST_2000_DPS_OUTPUT_CHANGE_MAX           (700u)


// === TYPE DEFINES ============================================================

typedef enum
{
    gyroSelfTestMode_positiveSign = 1,
    gyroSelfTestMode_negativeSign = 3,
} gyroSelfTestMode_t;

typedef union
{
    lsm6dsox_rawData_t rawData;
    struct
    {
        uint8_t xLo;
        uint8_t xHi;
        uint8_t yLo;
        uint8_t yHi;
        uint8_t zLo;
        uint8_t zHi;
    };
} data_t;

typedef struct
{
    bool gyroPoweredOn;
    lsm6dsox_gyroConfig_t gyroConfig;
} config_t;

typedef struct
{
    gyroSelfTestMode_t mode;
    lsm6dsox_gyroConfig_fs_t fs;
    uint16_t outputChangeMin;
    uint16_t outputChangeMax;
} configGyroSelfTest_t;


// === STATIC CONSTANTS ========================================================

static configGyroSelfTest_t const G_ConfigGyroSelfTest[] =
{
    [lsm6dsox_gyroSelfTestMode_250_dps_positive] =
    {
        .mode = gyroSelfTestMode_positiveSign,
        .fs = lsm6dsox_gyroConfig_fs_250_dps,
        .outputChangeMin = GRYO_SELF_TEST_250_DPS_OUTPUT_CHANGE_MIN,
        .outputChangeMax = GRYO_SELF_TEST_250_DPS_OUTPUT_CHANGE_MAX,
    },
    [lsm6dsox_gyroSelfTestMode_250_dps_negative] =
    {
        .mode = gyroSelfTestMode_negativeSign,
        .fs = lsm6dsox_gyroConfig_fs_250_dps,
        .outputChangeMin = GRYO_SELF_TEST_250_DPS_OUTPUT_CHANGE_MIN,
        .outputChangeMax = GRYO_SELF_TEST_250_DPS_OUTPUT_CHANGE_MAX,
    },
    [lsm6dsox_gyroSelfTestMode_2000_dps_positive] =
    {
        .mode = gyroSelfTestMode_positiveSign,
        .fs = lsm6dsox_gyroConfig_fs_2000_dps,
        .outputChangeMin = GRYO_SELF_TEST_2000_DPS_OUTPUT_CHANGE_MIN,
        .outputChangeMax = GRYO_SELF_TEST_2000_DPS_OUTPUT_CHANGE_MAX,
    },
    [lsm6dsox_gyroSelfTestMode_2000_dps_negative] =
    {
        .mode = gyroSelfTestMode_negativeSign,
        .fs = lsm6dsox_gyroConfig_fs_2000_dps,
        .outputChangeMin = GRYO_SELF_TEST_2000_DPS_OUTPUT_CHANGE_MIN,
        .outputChangeMax = GRYO_SELF_TEST_2000_DPS_OUTPUT_CHANGE_MAX,
    },
};


// === STATIC VARIABLES ========================================================

static bool g_started = false;

static config_t g_config =
{
    .gyroPoweredOn = false,
    .gyroConfig =
    {
        .fs = lsm6dsox_gyroConfig_fs_125_dps,
        .odr = lsm6dsox_gyroConfig_odr_12p5_Hz,
    },
};

static lsm6dsox_gyroSelfTestResult_t g_gyroSelfTestResult = lsm6dsox_gyroSelfTestResult_none;


// === DEPENDENCIES FUNCTION PROTOTYPES ========================================

/// @brief 
/// @param  
extern void comm_start(void);

/// @brief 
/// @param  
extern void comm_stop(void);

/// @brief 
/// @param data 
/// @param dataLength 
/// @return 
extern uint16_t comm_read(uint8_t* data, uint16_t dataLength);

/// @brief 
/// @param data 
/// @param dataLength 
/// @return 
extern uint16_t comm_write(uint8_t const* data, uint16_t dataLength);

/// @brief 
/// @param time_ms 
extern void delay_ms(uint32_t time_ms);


// === PRIVATE FUNCTIONS =======================================================

static void writeRegister(lsm6dsox_reg_t reg, uint8_t val)
{
    uint8_t temp[2] =
    {
        [0] = reg,
        [1] = val,
    };
    comm_write(temp, sizeof(temp));
}

static uint8_t readRegister(lsm6dsox_reg_t reg)
{
    uint8_t temp = reg;
    comm_write(&temp, sizeof(temp));
    uint8_t value = 0u;
    comm_read(&value, sizeof(value));
    return value;
}

static uint16_t readNRegisters(lsm6dsox_reg_t reg, uint8_t* data, uint16_t dataLength)
{
    if ((data == NULL) || (dataLength == 0u))
        return 0u;

    uint8_t temp = reg;
    comm_write(&temp, sizeof(temp));
    return comm_read(data, dataLength);
}

static bool confirmWhoAmI(void)
{
    uint8_t value = readRegister(lsm6dsox_reg_WHO_AM_I);
    return (value == 0x6c);
}

static void updateGyroFullScale(lsm6dsox_gyroConfig_fs_t fs, lsm6dsox_reg_CTRL2_G_t* const ctrl2_g)
{
    // modify the FS (full-scale) selection
    switch (fs)
    {
        case lsm6dsox_gyroConfig_fs_125_dps:
            ctrl2_g->FS_125 = 1u;
            ctrl2_g->FS_G = 0u;
            break;
        case lsm6dsox_gyroConfig_fs_250_dps:
            ctrl2_g->FS_125 = 0u;
            ctrl2_g->FS_G = 0u;
            break;
        case lsm6dsox_gyroConfig_fs_500_dps:
            ctrl2_g->FS_125 = 0u;
            ctrl2_g->FS_G = 1u;
            break;
        case lsm6dsox_gyroConfig_fs_1000_dps:
            ctrl2_g->FS_125 = 0u;
            ctrl2_g->FS_G = 2u;
            break;
        case lsm6dsox_gyroConfig_fs_2000_dps:
            ctrl2_g->FS_125 = 0u;
            ctrl2_g->FS_G = 3u;
            break;
    }
}

static float convertGyroDataRawToDPS(int16_t data)
{
    float factorDPS = 0.0f;
    switch (g_config.gyroConfig.fs)
    {
        case lsm6dsox_gyroConfig_fs_125_dps:
            factorDPS = 4.375f;
            break;
        case lsm6dsox_gyroConfig_fs_250_dps:
            factorDPS = 8.75f;
            break;
        case lsm6dsox_gyroConfig_fs_500_dps:
            factorDPS = 17.5f;
            break;
        case lsm6dsox_gyroConfig_fs_1000_dps:
            factorDPS = 35.0f;
            break;
        case lsm6dsox_gyroConfig_fs_2000_dps:
            factorDPS = 70.0f;
            break;
    }
    float dataDPS = data * factorDPS / 1000.0f;
    return dataDPS;
}

static bool isGyroDataReady(uint16_t timeout_ms)
{
    uint16_t wait_ms = (timeout_ms / 4u) + 1u;
    uint32_t time_ms = 0u;
    do
    {
        lsm6dsox_reg_STATUS_REG_t status_reg;
        status_reg.value = readRegister(lsm6dsox_reg_STATUS_REG);
        if (status_reg.GDA != 0u)
            return true;
        delay_ms(wait_ms);
        time_ms += wait_ms;
    } while (time_ms <= timeout_ms);

    return false;
}

static uint16_t getDataReadyTimeout(lsm6dsox_gyroConfig_odr_t odr)
{
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

    return G_GyroDataReadyTimeout_ms[odr];
}

static bool getAverageGyroDataRaw(uint16_t samples, lsm6dsox_rawData_t *const dataPtr)
{
    int32_t xTotal = 0u;
    int32_t yTotal = 0u;
    int32_t zTotal = 0u;
    for (uint8_t index = 0u; index < samples; ++index)
    {
        if (isGyroDataReady(getDataReadyTimeout(g_config.gyroConfig.odr)) == false)
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

void lsm6dsox_start(void)
{
    if (g_started == true)
        return;

    comm_start();
    if (confirmWhoAmI() == false)
        return;
    g_started = true;

    g_config.gyroPoweredOn = true;
    lsm6dsox_updateGyroConfig(g_config.gyroConfig);
}

void lsm6dsox_stop(void)
{
    if (g_started == false)
        return;

    lsm6dsox_reg_CTRL2_G_t ctrl2_g;
    ctrl2_g.value = readRegister(lsm6dsox_reg_CTRL2_G);
    ctrl2_g.ODR_G = (uint8_t)lsm6dsox_gyroConfig_odr_off; // power down the gyroscope
    writeRegister(lsm6dsox_reg_CTRL2_G, ctrl2_g.value);
    g_config.gyroPoweredOn = false;

    comm_stop();
    g_started = false;
}

lsm6dsox_err_t lsm6dsox_updateGyroConfig(lsm6dsox_gyroConfig_t config)
{
    if (g_started == false)
        return lsm6dsox_err_stopped;

    // update the gyro config
    g_config.gyroConfig = config;
    
    // read the CTRL2_G register
    lsm6dsox_reg_CTRL2_G_t ctrl2_g;
    ctrl2_g.value = readRegister(lsm6dsox_reg_CTRL2_G);

    // modify the FS (full-scale) selection
    updateGyroFullScale(config.fs, &ctrl2_g);

    // modify the ODR (output data rate) selection
    ctrl2_g.ODR_G = (uint8_t)config.odr;

    // write the CTRL2_G register
    writeRegister(lsm6dsox_reg_CTRL2_G, ctrl2_g.value);

    return lsm6dsox_err_none;
}

lsm6dsox_gyroConfig_fs_t lsm6dsox_getGyroConfigFS(void)
{
    return g_config.gyroConfig.fs;
}

lsm6dsox_gyroConfig_odr_t lsm6dsox_getGyroConfigODR(void)
{
    return g_config.gyroConfig.odr;
}

lsm6dsox_err_t lsm6dsox_getGyroDataRaw(lsm6dsox_rawData_t* const dataPtr)
{
    if (g_started == false)
        return lsm6dsox_err_stopped;
    if (g_config.gyroPoweredOn == false)
        return lsm6dsox_err_gyroPoweredOff;
    if (dataPtr == NULL)
        return lsm6dsox_err_paramNull;

    data_t data;
    data.xLo = readRegister(lsm6dsox_reg_OUTX_L_G);
    data.xHi = readRegister(lsm6dsox_reg_OUTX_H_G);
    data.yLo = readRegister(lsm6dsox_reg_OUTY_L_G);
    data.yHi = readRegister(lsm6dsox_reg_OUTY_H_G);
    data.zLo = readRegister(lsm6dsox_reg_OUTZ_L_G);
    data.zHi = readRegister(lsm6dsox_reg_OUTZ_H_G);
    *dataPtr = data.rawData;

    return lsm6dsox_err_none;
}

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

lsm6dsox_err_t lsm6dsox_startGyroSelfTest(lsm6dsox_gyroSelfTestMode_t mode)
{
    if (g_started == false)
        return lsm6dsox_err_stopped;
    if (g_config.gyroPoweredOn == false)
        return lsm6dsox_err_gyroPoweredOff;

    // retain the CTRL2_G and CTRL5_C registers
    lsm6dsox_reg_CTRL2_G_t ctrl2_g_orig;
    ctrl2_g_orig.value = readRegister(lsm6dsox_reg_CTRL2_G);
    lsm6dsox_reg_CTRL5_C_t ctrl5_c_orig;
    ctrl5_c_orig.value = readRegister(lsm6dsox_reg_CTRL5_C);
    
    lsm6dsox_err_t err = lsm6dsox_err_none;
    lsm6dsox_rawData_t noSelfTestData;
    lsm6dsox_rawData_t selfTestData;
    do
    {
        // configure the FS (full scale selection)
        g_gyroSelfTestResult = lsm6dsox_gyroSelfTestResult_running;
        lsm6dsox_reg_CTRL2_G_t ctrl2_g;
        ctrl2_g.value = readRegister(lsm6dsox_reg_CTRL2_G);
        lsm6dsox_reg_CTRL2_G_t ctrl2_g_orig = ctrl2_g;
        updateGyroFullScale(G_ConfigGyroSelfTest[mode].fs, &ctrl2_g);
        writeRegister(lsm6dsox_reg_CTRL2_G, ctrl2_g.value);
        delay_ms(GYRO_SELF_TEST_WAIT_MS);
        if (isGyroDataReady(GYRO_SELF_TEST_WAIT_MS/4) == false)
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
        ctrl5_c.ST_G = (uint8_t)G_ConfigGyroSelfTest[mode].mode;
        writeRegister(lsm6dsox_reg_CTRL5_C, ctrl5_c.value);
        delay_ms(GYRO_SELF_TEST_WAIT_MS);
        if (isGyroDataReady(GYRO_SELF_TEST_WAIT_MS/4) == false)
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
            && (deltas[index] >= G_ConfigGyroSelfTest[mode].outputChangeMin)
            && (deltas[index] <= G_ConfigGyroSelfTest[mode].outputChangeMax);
    }
    g_gyroSelfTestResult = lsm6dsox_gyroSelfTestResult_passed;
    if (passed == false)
        g_gyroSelfTestResult = lsm6dsox_gyroSelfTestResult_failed;

    return err;
}

lsm6dsox_gyroSelfTestResult_t lsm6dsox_getGyroSelfTestResult(void)
{
    return g_gyroSelfTestResult;
}
