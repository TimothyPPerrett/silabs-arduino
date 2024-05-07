/*
 * This file is part of the Silicon Labs Arduino Core
 *
 * The MIT License (MIT)
 *
 * Copyright 2024 Silicon Laboratories Inc. www.silabs.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "Arduino.h"
#include "arduino_variant.h"
#include "arduino_i2c_config.h"
#include "arduino_spi_config.h"

extern "C" {
  #include "sl_component_catalog.h"
  #ifdef SL_CATALOG_RAIL_LIB_PRESENT
    #include "rail.h"
  #endif
}

#ifdef ARDUINO_MATTER

#include "AppConfig.h"
#include <DeviceInfoProviderImpl.h>
#include <MatterConfig.h>
#include <app/server/Server.h>
#include <platform/silabs/platformAbstraction/SilabsPlatform.h>
#include <provision/ProvisionManager.h>

#ifdef BLE_DEV_NAME
#undef BLE_DEV_NAME
#endif
#define BLE_DEV_NAME "Arduino Matter device"

static chip::DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

using namespace ::chip;
using namespace ::chip::DeviceLayer;
using namespace ::chip::Credentials;
using namespace chip::DeviceLayer::Silabs;

#else //ARDUINO_MATTER

extern "C" {
  #include "sl_system_init.h"
}

#endif //ARDUINO_MATTER

void init_arduino_variant()
{
  #ifdef ARDUINO_MATTER
  // Initialize the Matter stack
  GetPlatform().Init();

  if (Provision::Manager::GetInstance().ProvisionRequired()) {
    Provision::Manager::GetInstance().Start();
  } else {
    if (SilabsMatterConfig::InitMatter(BLE_DEV_NAME) != CHIP_NO_ERROR) {
      appError(CHIP_ERROR_INTERNAL);
    }
    gExampleDeviceInfoProvider.SetStorageDelegate(&chip::Server::GetInstance().GetPersistentStorage());
    chip::DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    chip::DeviceLayer::PlatformMgr().LockChipStack();
    // Initialize device attestation config
    SetDeviceAttestationCredentialsProvider(&Provision::Manager::GetInstance().GetStorage());
    chip::DeviceLayer::PlatformMgr().UnlockChipStack();
  }

  #else //ARDUINO_MATTER

  sl_system_init();

  #endif //ARDUINO_MATTER

  // Disable SWO by default and allow PA3 to be used as a GPIO pin
  GPIO_DbgSWOEnable(false);

  #ifdef SL_CATALOG_RAIL_LIB_PRESENT
  // Disable RAIL PTI by default and allow PC4 and PC5 to be used as a GPIO pin
  RAIL_PtiConfig_t railPtiConfig = {};
  railPtiConfig.mode = RAIL_PTI_MODE_DISABLED;
  RAIL_ConfigPti(RAIL_EFR32_HANDLE, &railPtiConfig);
  RAIL_EnablePti(RAIL_EFR32_HANDLE, false);
  #endif

  // Deinit Serial, Wire and SPI by default - sl_system_init() initializes it
  Serial.end();
  Serial1.end();
  I2C_Deinit(SL_I2C_PERIPHERAL); // Wire.end()
  SPIDRV_DeInit(SL_SPIDRV_PERIPHERAL_HANDLE); //SPI.end();

  // Reenable the clock for EUSART0 in CMU (cmuClock_EUSART0) for the SPI1 to successfully deinit
  // This clock is turned off in Serial.end() as they share the same EUSART0 peripheral.
  // We turn it back on so that the SPI1 can deinitialize without running to a fault when accessing the EUSART0 registers.
  CMU_ClockEnable(cmuClock_EUSART0, true);
  SPIDRV_DeInit(SL_SPIDRV1_PERIPHERAL_HANDLE); // SPI1.end();
}

// Variant pin mapping - maps Arduino pin numbers to Silabs ports/pins
// D0 -> Dmax -> A0 -> Amax -> Other peripherals
PinName gPinNames[] = {
  PC7, // D0
  PA5, // D1 - Tx
  PA6, // D2 - Rx
  PC6, // D3 - SPI SDI
  PC3, // D4 - SPI SDO
  PC2, // D5 - SPI SCK
  PC1, // D6 - SPI SS
  PC0, // D7
  PD0, // D8
  PD1, // D9
  PD2, // D10
  PD3, // D11
  PB4, // A0 - SDA
  PB3, // A1 - SCL - DAC3
  PB2, // A2 - SPI1 SDI - Rx1 - DAC2
  PB1, // A3 - SPI1 SDO - Tx1 - DAC1
  PB0, // A4 - SPI1 SCK - DAC0
  PA0, // A5 - SPI1 SS
  PA4, // A6
  PC4, // A7
  PC5, // A8
  PA8, // LED - 21
  PA7, // SD card SPI CS - 22
};

unsigned int getPinCount()
{
  return sizeof(gPinNames) / sizeof(gPinNames[0]);
}