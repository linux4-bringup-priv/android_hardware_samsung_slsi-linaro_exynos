/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.thermal@2.0-impl"

#include <errno.h>
#include <math.h>

#include <vector>

#include <log/log.h>

#include <hardware/hardware.h>
#include <hardware/thermal.h>

#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>


#include "Thermal.h"

namespace android {
namespace hardware {
namespace thermal {
namespace V2_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::interfacesEqual;
using ::android::hardware::thermal::V1_0::CoolingDevice;
using ::android::hardware::thermal::V2_0::CoolingType;
using ::android::hardware::thermal::V1_0::CpuUsage;
using ::android::hardware::thermal::V2_0::Temperature;
using ::android::hardware::thermal::V2_0::TemperatureType;
using ::android::hardware::thermal::V1_0::ThermalStatus;
using ::android::hardware::thermal::V1_0::ThermalStatusCode;
using ::android::hardware::thermal::V2_0::IThermalChangedCallback;
using ::android::hardware::thermal::V2_0::IThermal;

Thermal::Thermal()
	: thermal_notifier_(new ThermalNotifier(
			  std::bind(&Thermal::sendThermalChangedCallback, this, std::placeholders::_1))) {
	initExynosThermalHal();
	if (!thermal_notifier_->startWatchingDeviceFiles())
		LOG(FATAL) << "ThermalHAL could not start notifier thread properly.";
}
// Methods from ::android::hardware::thermal::V1_0::IThermal follow.
Return<void> Thermal::getTemperatures(getTemperatures_cb _hidl_cb) {
	ThermalStatus status;
	status.code = ThermalStatusCode::SUCCESS;
	hidl_vec<Temperature_1_0> temperatures;

	status = getAllTemperatures(temperatures);

	_hidl_cb(status, temperatures);
	return Void();
}

Return<void> Thermal::getCpuUsages(getCpuUsages_cb _hidl_cb) {
	ThermalStatus status;
	status.code = ThermalStatusCode::SUCCESS;
	hidl_vec<CpuUsage> cpuUsage;

	status = getCpuUsage(cpuUsage);

	_hidl_cb(status, cpuUsage);
	return Void();
}

Return<void> Thermal::getCoolingDevices(getCoolingDevices_cb _hidl_cb) {
	ThermalStatus status;
	status.code = ThermalStatusCode::SUCCESS;
	vector<CoolingDevice_1_0> cdevs;

	status = coolingDevices(cdevs);

	_hidl_cb(status, cdevs);
	return Void();
}

Return<void> Thermal::getCurrentTemperatures(bool filterType, TemperatureType type,
									getCurrentTemperatures_cb _hidl_cb) {
	ThermalStatus status;
	status.code = ThermalStatusCode::SUCCESS;
	hidl_vec<Temperature_2_0> temperatures;

	if (filterType) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "Failed to read data";
	} else {
		status = getTypeTemperatures(type, temperatures);
	}

	_hidl_cb(status, temperatures);
	return Void();
}

Return<void> Thermal::getTemperatureThresholds(bool filterType, TemperatureType type,
									getTemperatureThresholds_cb _hidl_cb) {
	ThermalStatus status;
	status.code = ThermalStatusCode::SUCCESS;
	vector<TemperatureThreshold> temperature_thresholds;

	if (filterType) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "Failed to read data";
	} else {
		status = temperatureThresholds(type, temperature_thresholds);
	}

	_hidl_cb(status, temperature_thresholds);
	return Void();
}

Return<void> Thermal::registerThermalChangedCallback(const sp<IThermalChangedCallback>& callback,
													 bool filterType, TemperatureType type,
													 registerThermalChangedCallback_cb _hidl_cb) {
	ThermalStatus status;
	if (callback == nullptr) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "Invalid nullptr callback";
		LOG(ERROR) << status.debugMessage;
		_hidl_cb(status);
		return Void();
	} else {
		status.code = ThermalStatusCode::SUCCESS;
	}
	std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
	if (std::any_of(callbacks_.begin(), callbacks_.end(), [&](const CallbackSetting& c) {
			return interfacesEqual(c.callback, callback);
		})) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "Same callback interface registered already";
		LOG(ERROR) << status.debugMessage;
	} else {
		callbacks_.emplace_back(callback, filterType, type);
		LOG(INFO) << "A callback has been registered to ThermalHAL, isFilter: " << filterType
				<< " Type: " << android::hardware::thermal::V2_0::toString(type);
	}
	_hidl_cb(status);
	return Void();
}

Return<void> Thermal::unregisterThermalChangedCallback(
	const sp<IThermalChangedCallback>& callback, unregisterThermalChangedCallback_cb _hidl_cb) {
	ThermalStatus status;
	if (callback == nullptr) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "Invalid nullptr callback";
		LOG(ERROR) << status.debugMessage;
		_hidl_cb(status);
		return Void();
	} else {
		status.code = ThermalStatusCode::SUCCESS;
	}
	bool removed = false;
	std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
	callbacks_.erase(
		std::remove_if(callbacks_.begin(), callbacks_.end(),
					[&](const CallbackSetting& c) {
						if (interfacesEqual(c.callback, callback)) {
							LOG(INFO)
								<< "A callback has been unregistered from ThermalHAL, isFilter: "
								<< c.is_filter_type << " Type: "
								<< android::hardware::thermal::V2_0::toString(c.type);
							removed = true;
							return true;
						}
						return false;
					}),
		callbacks_.end());
	if (!removed) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "The callback was not registered before";
		LOG(ERROR) << status.debugMessage;
	}
	_hidl_cb(status);
	return Void();
}

Return<void> Thermal::getCurrentCoolingDevices(bool filterType, CoolingType type,
									getCurrentCoolingDevices_cb _hidl_cb) {
	ThermalStatus status;
	status.code = ThermalStatusCode::SUCCESS;
	hidl_vec<CoolingDevice_2_0> cooling_devices;

	if (filterType) {
		status.code = ThermalStatusCode::FAILURE;
		status.debugMessage = "Failed to read data";
	} else {
		status = curCoolingDevices(type, cooling_devices);
	}

	_hidl_cb(status, cooling_devices);
	return Void();
}

void Thermal::sendThermalChangedCallback(const std::vector<Temperature_2_0> &temps) {
	std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
	for (auto &t : temps) {
		LOG(VERBOSE) << "Sending notification: "
			<< " Type: " << android::hardware::thermal::V2_0::toString(t.type)
			<< " Name: " << t.name << " CurrentValue: " << t.value << " ThrottlingStatus: "
			<< android::hardware::thermal::V2_0::toString(t.throttlingStatus);
		callbacks_.erase(
				std::remove_if(callbacks_.begin(), callbacks_.end(),
					[&](const CallbackSetting &c) {
					if (!c.is_filter_type || t.type == c.type) {
					Return<void> ret = c.callback->notifyThrottling(t);
					return !ret.isOk();
					}
					LOG(ERROR)
					<< "a Thermal callback is dead, removed from callback list.";
					return false;
					}),
				callbacks_.end());
	}
}

} // namespace implementation
} // namespace V2_0
} // namespace thermal
} // namespace hardware
} // namespace android
