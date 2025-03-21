#pragma once

typedef void (*DeviceFoundCallback)(const wchar_t*, const wchar_t*);
typedef void (*ServiceListCallback)(const wchar_t*);
typedef void (*ServiceConfigListCallback)(const wchar_t*);

extern "C" {
    PLUGIN_API void StartDeviceWatcher();
    PLUGIN_API void DeviceFound(DeviceFoundCallback callback);
    PLUGIN_API void SelectDevice(const wchar_t*);
    PLUGIN_API void ServiceList(ServiceListCallback callback);
    PLUGIN_API void SelectService(const wchar_t* uuid);
    PLUGIN_API void ServiceConfigList(ServiceConfigListCallback callback);
    PLUGIN_API void SelectConfigService(const wchar_t* uuid);
    PLUGIN_API void EnableNotifications(int index = 0);
    PLUGIN_API void QuitBLE();
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

