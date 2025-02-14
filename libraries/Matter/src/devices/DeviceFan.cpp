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

#include "DeviceFan.h"

DeviceFan::DeviceFan(const char* device_name,
                     uint8_t speed_max) :
  Device(device_name),
  fan_speed_max(speed_max),
  current_percent(0),
  current_fan_mode(fan_mode_t::Off)
{
  ;
}

uint8_t DeviceFan::GetPercentSetting()
{
  return this->current_percent;
}

void DeviceFan::SetPercentSetting(uint8_t percent, bool setMode)
{
  if (percent > 100) {
    percent = 100;
  }

  bool changed = this->current_percent != percent;
  ChipLogProgress(DeviceLayer, "FanDevice[%s]: new percent='%d'", this->device_name, percent);
  this->current_percent = percent;

  if (changed) {
    this->HandleFanDeviceStatusChanged(kChanged_PercentSetting);
    this->HandleFanDeviceStatusChanged(kChanged_PercentCurrent);
    CallDeviceChangeCallback();
  }

  if (!changed || !setMode) {
    return;
  }

  if (percent == 0) {
    SetFanMode((uint8_t)fan_mode_t::Off, false);
  } else if (percent <= 33) {
    SetFanMode((uint8_t)fan_mode_t::Low, false);
  } else if (percent <= 66) {
    SetFanMode((uint8_t)fan_mode_t::Med, false);
  } else {
    SetFanMode((uint8_t)fan_mode_t::High, false);
  }
}

uint8_t DeviceFan::GetPercentCurrent()
{
  return this->current_percent;
}

void DeviceFan::SetPercentCurrent(uint8_t percent)
{
  (void)percent;
}

uint8_t DeviceFan::GetSpeedSetting()
{
  return PercentToSpeed(this->GetPercentSetting());
}

void DeviceFan::SetSpeedSetting(uint8_t speed, bool setMode)
{
  this->SetPercentSetting(this->SpeedToPercent(speed), setMode);
}

uint8_t DeviceFan::GetSpeedCurrent()
{
  return this->PercentToSpeed(this->GetPercentCurrent());
}

void DeviceFan::SetSpeedCurrent(uint8_t speed)
{
  this->SetPercentCurrent(this->SpeedToPercent(speed));
}

void DeviceFan::SetFanMode(uint8_t fan_mode, bool setPercent)
{
  if (fan_mode == (uint8_t)fan_mode_t::On) {
    fan_mode = (uint8_t)fan_mode_t::High;
  } else if (fan_mode == (uint8_t)fan_mode_t::Smart) {
    fan_mode = (uint8_t)fan_mode_t::High;
  }
  
  bool changed = this->current_fan_mode != fan_mode;
  ChipLogProgress(DeviceLayer, "FanDevice[%s]: new mode='%d'", this->device_name, fan_mode);
  this->current_fan_mode = (fan_mode_t)fan_mode;

  if (changed) {
    this->HandleFanDeviceStatusChanged(kChanged_ModeSetting);
    CallDeviceChangeCallback();
  }

  if (!changed || !setPercent) {
    return;
  }

  // Adjust the percentage to the selected mode
  switch ((fan_mode_t)fan_mode) {
    case fan_mode_t::Off:
      this->SetPercentSetting(0, false);
      break;
    
    case fan_mode_t::Low:
      this->SetPercentSetting(20, false);
      break;

    case fan_mode_t::Med:
      this->SetPercentSetting(50, false);
      break;

    case fan_mode_t::High:
      this->SetPercentSetting(100, false);
      break;

    default:
      break;
  }
}

uint8_t DeviceFan::GetFanMode()
{
  return this->current_fan_mode;
}

uint8_t DeviceFan::GetFanModeSequence()
{
  return this->fan_mode_sequence;
}

uint8_t DeviceFan::GetFanSpeedMax()
{
  return this->fan_speed_max;
}

uint32_t DeviceFan::GetFanClusterFeatureMap()
{
  return this->fan_cluster_feature_map;
}

uint16_t DeviceFan::GetFanClusterRevision()
{
  return this->fan_cluster_revision;
}

EmberAfStatus DeviceFan::HandleReadEmberAfAttribute(ClusterId clusterId,
                                                    chip::AttributeId attributeId,
                                                    uint8_t* buffer,
                                                    uint16_t maxReadLength)
{
  if (!this->reachable) {
    return EMBER_ZCL_STATUS_FAILURE;
  }

  using namespace ::chip::app::Clusters::FanControl::Attributes;
  ChipLogProgress(DeviceLayer, "HandleReadFanControlAttribute: clusterId=%lu attrId=%ld", clusterId, attributeId);

  if (clusterId == chip::app::Clusters::BridgedDeviceBasicInformation::Id) {
    return this->HandleReadBridgedDeviceBasicAttribute(clusterId, attributeId, buffer, maxReadLength);
  }

  if (clusterId != chip::app::Clusters::FanControl::Id) {
    return EMBER_ZCL_STATUS_FAILURE;
  }

  if ((attributeId == FanMode::Id) && (maxReadLength == 1)) {
    uint8_t fanmode = this->GetFanMode();
    memcpy(buffer, &fanmode, sizeof(fanmode));
  } else if ((attributeId == FanModeSequence::Id) && (maxReadLength == 1)) {
    uint8_t fanmode_sequence = this->GetFanModeSequence();
    memcpy(buffer, &fanmode_sequence, sizeof(fanmode_sequence));
  } else if ((attributeId == PercentSetting::Id) && (maxReadLength == 1)) {
    uint8_t percent_setting = this->GetPercentSetting();
    memcpy(buffer, &percent_setting, sizeof(percent_setting));
  } else if ((attributeId == PercentCurrent::Id) && (maxReadLength == 1)) {
    uint8_t percent_current = this->GetPercentCurrent();
    memcpy(buffer, &percent_current, sizeof(percent_current));
  } else if ((attributeId == SpeedMax::Id) && (maxReadLength == 1)) {
    uint8_t speed_max = this->GetFanSpeedMax();
    memcpy(buffer, &speed_max, sizeof(speed_max));
  } else if ((attributeId == SpeedSetting::Id) && (maxReadLength == 1)) {
    uint8_t speed_setting = this->GetSpeedSetting();
    memcpy(buffer, &speed_setting, sizeof(speed_setting));
  } else if ((attributeId == SpeedCurrent::Id) && (maxReadLength == 1)) {
    uint8_t speed_current = this->GetSpeedCurrent();
    memcpy(buffer, &speed_current, sizeof(speed_current));
  } else if ((attributeId == FeatureMap::Id) && (maxReadLength == 4)) {
    uint32_t featureMap = this->GetFanClusterFeatureMap();
    memcpy(buffer, &featureMap, sizeof(featureMap));
  } else if ((attributeId == ClusterRevision::Id) && (maxReadLength == 2)) {
    uint16_t clusterRevision = this->GetFanClusterRevision();
    memcpy(buffer, &clusterRevision, sizeof(clusterRevision));
  } else {
    return EMBER_ZCL_STATUS_FAILURE;
  }

  return EMBER_ZCL_STATUS_SUCCESS;
}

EmberAfStatus DeviceFan::HandleWriteEmberAfAttribute(ClusterId clusterId,
                                                     chip::AttributeId attributeId,
                                                     uint8_t* buffer)
{
  if (!this->reachable) {
    return EMBER_ZCL_STATUS_FAILURE;
  }

  using namespace ::chip::app::Clusters::FanControl::Attributes;
  ChipLogProgress(DeviceLayer, "HandleWriteFanControlAttribute: clusterId=%lu attrId=%ld", clusterId, attributeId);

  if (clusterId != chip::app::Clusters::FanControl::Id) {
    return EMBER_ZCL_STATUS_FAILURE;
  }

  if (attributeId == PercentSetting::Id) {
    this->SetPercentSetting(*buffer, true);
  } else if (attributeId == SpeedSetting::Id) {
    this->SetSpeedSetting(*buffer, true);
  } else if (attributeId == FanMode::Id) {
    this->SetFanMode(*buffer, true);
  } else {
    return EMBER_ZCL_STATUS_FAILURE;
  }

  return EMBER_ZCL_STATUS_SUCCESS;
}

uint8_t DeviceFan::SpeedToPercent(uint8_t speed)
{
  (uint8_t) floorf((float)speed / (float)fan_speed_max * 100);
}

uint8_t DeviceFan::PercentToSpeed(uint8_t percent)
{
  return (uint8_t) ceilf(fan_speed_max * (percent * 0.01f));
}

void DeviceFan::HandleFanDeviceStatusChanged(Changed_t itemChangedMask)
{
  using namespace ::chip::app::Clusters;

  if (itemChangedMask & kChanged_PercentSetting) {
    ScheduleMatterReportingCallback(this->endpoint_id, FanControl::Id, FanControl::Attributes::PercentSetting::Id);
  }
  if (itemChangedMask & kChanged_PercentCurrent) {
    ScheduleMatterReportingCallback(this->endpoint_id, FanControl::Id, FanControl::Attributes::PercentCurrent::Id);
  }
  if (itemChangedMask & kChanged_ModeSetting) {
    ScheduleMatterReportingCallback(this->endpoint_id, FanControl::Id, FanControl::Attributes::FanMode::Id);
  }
}
