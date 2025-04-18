// BLEManager.h
#pragma once

#ifdef BLEMANAGER_EXPORTS
#define BLEMANAGER_API __declspec(dllexport)
#else
#define BLEMANAGER_API __declspec(dllimport)
#endif

typedef void (*OnSendMessage)(const wchar_t*);

extern "C" {
    BLEMANAGER_API void Initialise(void (*callback)(const wchar_t*));
    BLEMANAGER_API void StartScan(const wchar_t*);
    BLEMANAGER_API void StopScan();
    BLEMANAGER_API void ConnectToDevice(const wchar_t* deviceId);
    BLEMANAGER_API void DisconnectDevice(const wchar_t* deviceId);
    BLEMANAGER_API void DisconnectAllDevices();
    BLEMANAGER_API void UnpairDevice(const wchar_t* deviceId);
    BLEMANAGER_API void Subscribe(const wchar_t* deviceId, const wchar_t* serviceUuid, const wchar_t* characteristicUuid);
    BLEMANAGER_API const uint8_t* ReadCharacteristic(const wchar_t* deviceId, int index, int* length);
    BLEMANAGER_API bool WriteCharacteristic(const wchar_t* deviceId, int index, const uint8_t* data, int length);
    BLEMANAGER_API void ListPairedDevices();
    BLEMANAGER_API void ListConnectedDevices();
    BLEMANAGER_API void QuitBLE();
}

class BLEManager {
public:
    void Initialise(std::function<void(const wchar_t*)> callback);
    void StartScan(const wchar_t* nameFilter = L"");
    void StopScan();
    void ConnectToDevice(const wchar_t* deviceId);
    winrt::Windows::Foundation::IAsyncAction ConnectToDeviceAsync(const wchar_t* deviceId);
    void DisconnectDevice(const std::wstring& deviceId);
    void DisconnectAllDevices();
    void UnpairDevice(const wchar_t* deviceId);
    void Subscribe(const wchar_t* deviceId, const wchar_t* serviceUuid, const wchar_t* characteristicUuid);
    winrt::Windows::Foundation::IAsyncAction SubscribeAsync(const wchar_t* deviceId, const wchar_t* serviceUuid, const wchar_t* characteristicUuid);
    std::vector<uint8_t> Read(const wchar_t* deviceId, int characteristicIndex);
    bool Write(const wchar_t* deviceId, int characteristicIndex, const std::vector<uint8_t>& data);
    void ListPairedDevices();
    void ListConnectedDevices();
    void Quit();

private:
    struct BLEDeviceInfo {
        winrt::Windows::Devices::Bluetooth::BluetoothLEDevice device{ nullptr };
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService> services{ nullptr };
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic> characteristics{ nullptr };
    };

    winrt::Windows::Devices::Enumeration::DeviceWatcher m_watcher{ nullptr };
    std::unordered_map<std::wstring, BLEDeviceInfo> m_devices;
    std::unordered_map<std::wstring, winrt::event_token> m_notifyTokens;
    std::shared_mutex m_mutex;
    std::function<void(const wchar_t*)> m_callback;

    void AddDevice(const std::wstring& deviceId, const BLEDeviceInfo& device);
    void RemoveDevice(const std::wstring& deviceId);
    std::optional<BLEDeviceInfo> GetDevice(const std::wstring& deviceId);

    void DeviceAdded(winrt::Windows::Devices::Enumeration::DeviceWatcher const& sender, winrt::Windows::Devices::Enumeration::DeviceInformation const& info);
    void DeviceUpdated(winrt::Windows::Devices::Enumeration::DeviceWatcher const& sender, winrt::Windows::Devices::Enumeration::DeviceInformationUpdate const& update);
    void DeviceStopped(winrt::Windows::Devices::Enumeration::DeviceWatcher const& sender, winrt::Windows::Foundation::IInspectable const& stop);

    std::wstring ToWStringGuid(GUID guid);
    std::wstring EncodeBase64(const std::vector<uint8_t>& data);
};
