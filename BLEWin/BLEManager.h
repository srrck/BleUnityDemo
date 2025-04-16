#pragma once

typedef void (*OnSendMessage)(const wchar_t*);

extern "C" {
    PLUGIN_API void Initialise(OnSendMessage callback);
    PLUGIN_API void StartScan(const wchar_t*);
    PLUGIN_API void StopScan();
    PLUGIN_API void ConnectToDevice(const wchar_t*);
    PLUGIN_API void DisconnectDevice(const wchar_t* deviceId);
    PLUGIN_API void DisconnectAllDevices();
    PLUGIN_API void UnpairDevice(const wchar_t* deviceId);
    PLUGIN_API void Subscribe(const wchar_t* deviceId, const wchar_t* serviceUuid, const wchar_t* characteristicUuid);
    PLUGIN_API const uint8_t* ReadCharacteristic(const wchar_t* deviceId, int index, int* length);
    PLUGIN_API bool Write(const wchar_t* deviceId, int characteristicIndex, const std::vector<uint8_t>& data);
    PLUGIN_API void ListPairedDevices();
    PLUGIN_API void ListConnectedDevices();
    PLUGIN_API void Quit();
}

/*class BLEManager {
public:
    BLEManager();
    ~BLEManager();

    void StartDeviceWatcher();
    void StopDeviceWatcher();
    void ListDevices();
    bool ConnectToDevice(int index);
    void DiscoverServices();
    std::vector<uint8_t> ReadCharacteristic();
    bool WriteCharacteristic(const std::vector<uint8_t>& data);
};*/

