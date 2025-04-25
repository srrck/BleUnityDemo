// BLEManagerDLL.cpp
#include "pch.h"
#include "BLEManager.h"
#include <vector>
#include <memory>

static std::unique_ptr<BLEManager> g_manager;
static std::vector<uint8_t> g_readBuffer; // Read buffer to return pointer

extern "C" {

    void Initialise(void (*callback)(const wchar_t*))
    {
        if (!g_manager)
            g_manager = std::make_unique<BLEManager>();

        g_manager->Initialise([callback](const std::wstring& msg) {
            try {
                if (callback) {
                    std::wstring safe = msg;
                    callback(safe.c_str());
                }
            }
            catch (...) {
                // 오류 무시 또는 로그 출력
            }
            });
    }

    void StartScan(const wchar_t* nameFilter) {
        if (g_manager) g_manager->StartScan(nameFilter);
    }

    void StopScan() {
        if (g_manager) g_manager->StopScan();
    }

    void ConnectToDevice(const wchar_t* deviceId) {
        if (g_manager) g_manager->ConnectToDevice(deviceId);
    }

    void DisconnectDevice(const wchar_t* deviceId) {
        if (g_manager) g_manager->DisconnectDevice(deviceId);
    }

    void DisconnectAllDevices() {
        if (g_manager) g_manager->DisconnectAllDevices();
    }

    void UnpairDevice(const wchar_t* deviceId) {
        if (g_manager) g_manager->UnpairDevice(deviceId);
    }

    void Subscribe(const wchar_t* deviceId, const wchar_t* serviceUuid, const wchar_t* characteristicUuid) {
        if (g_manager) g_manager->Subscribe(deviceId, serviceUuid, characteristicUuid);
    }

    void ListServicesForDevice(const wchar_t* deviceId) {
        if (g_manager) g_manager->ListServices(deviceId);
    }

    void ListCharacteristicsForService(const wchar_t* deviceId, const wchar_t* serviceUuid) {
        if (g_manager) g_manager->ListCharacteristics(deviceId, serviceUuid);
    }

    const uint8_t* ReadCharacteristic(const wchar_t* deviceId, int index, int* length) {
        if (!g_manager) return nullptr;
        g_readBuffer = g_manager->Read(deviceId, index);
        if (length) *length = static_cast<int>(g_readBuffer.size());
        return g_readBuffer.data();
    }

    bool WriteCharacteristic(const wchar_t* deviceId, int index, const uint8_t* data, int length) {
        if (!g_manager || !data || length <= 0) return false;
        std::vector<uint8_t> buffer(data, data + length);
        return g_manager->Write(deviceId, index, buffer);
    }

    void ListPairedDevices() {
        if (g_manager) g_manager->ListPairedDevices();
    }

    void ListConnectedDevices() {
        if (g_manager) g_manager->ListConnectedDevices();
    }

    void QuitBLE() {
        if (g_manager) {
            g_manager->Quit();
            g_manager.reset();
        }
    }

} // extern "C"
