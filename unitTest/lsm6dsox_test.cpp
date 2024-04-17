/* GTest is cpp, so extern source files as c. */
extern "C"
{
    #include "lsm6dsox.h"
    #include "lsm6dsox_reg.h"
    #include "lsm6dsox.c"
}
#include <gtest/gtest.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "lsm6dsox_helper.h"

/* Stub Functions */

/* LSM6DSOX Unit Test Classd */
class lsm6dsoxUnitTest : public ::testing::Test
{
protected:
    void SetUp() override
    {

    }

    void TearDown() override
    {
        /* do nothing */
    }

    /* Initialize */
    void InitLSM6DSOX()
    {

    }

    void InitLSM6DSOXHelper()
    {
        helper_init();
    }
};

/* Test the static writeRegister function. */
TEST_F(lsm6dsoxUnitTest, writeRegister)
{
    EXPECT_EQ()
    //writeRegister(lsm6dsox_reg_CTRL2_G);
}

/* Test the static readRegister function. */
TEST_F(lsm6dsoxUnitTest, writeRegister)
{
    //EXPECT_EQ(0x6c, readRegister(lsm6dsox_reg_WHO_AM_I));
}

/* Test the lsm6dsox_start function. */
TEST_F(lsm6dsoxUnitTest, lsm6dsox_start)
{

}