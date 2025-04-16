#include "pch.h"
#include "BLEManager.h"
#pragma comment(lib, "windowsapp")
using namespace std;
using namespace winrt;
using namespace winrt::impl;
using namespace Windows::Foundation;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Foundation::Collections;
using namespace Windows::Devices::Bluetooth::Advertisement;
using namespace Windows::Storage::Streams;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;


// 콜백 정의
OnSendMessage m_callback;

// 동기화 및 상태 저장
std::shared_mutex m_mutex;
DeviceWatcher m_watcher{ nullptr };
static std::vector<uint8_t> g_readBuffer; // Read buffer to return pointer
static GattCharacteristic::ValueChanged_revoker g_valueChangedRevoker;

// BLE 장치 관리
struct BLEDeviceInfo {
    BluetoothLEDevice device{ nullptr };
    IVectorView<GattDeviceService> services;
    IVectorView<GattCharacteristic> characteristics;
};
std::unordered_map<std::wstring, BLEDeviceInfo> m_devices;

// 함수 선언
void Initialise(OnSendMessage callback);
void StartScan(const wchar_t*);
void StopScan();
void ConnectToDevice(const wchar_t*);
IAsyncAction ConnectToDeviceAsync(const wchar_t* deviceId);
void DisconnectDevice(const wchar_t* deviceId);
void DisconnectAllDevices();
void UnpairDevice(const wchar_t* deviceId);
void Subscribe(const wchar_t* deviceId, const wchar_t* serviceUuid, const wchar_t* characteristicUuid);
IAsyncAction SubscribeAsync(const wchar_t* deviceId, const wchar_t* serviceUuid, const wchar_t* characteristicUuid);
const uint8_t* ReadCharacteristic(const wchar_t* deviceId, int index, int* length);
std::vector<uint8_t> Read(const wchar_t* deviceId, int characteristicIndex);
bool Write(const wchar_t* deviceId, int characteristicIndex, const std::vector<uint8_t>& data);
void ListPairedDevices();
void ListConnectedDevices();
void Quit();

// 내부 유틸 함수
void DeviceAdded(DeviceWatcher const&, DeviceInformation const& info);
void DeviceUpdated(DeviceWatcher const&, DeviceInformationUpdate const&);
void AddDevice(const std::wstring& deviceId, const BLEDeviceInfo& device);
void RemoveDevice(const std::wstring& deviceId);
std::optional<BLEDeviceInfo> GetDevice(const std::wstring& deviceId);
std::wstring ToWStringGuid(GUID guid);
std::wstring EncodeBase64(const std::vector<uint8_t>& data);

// 초기화
void Initialise(OnSendMessage callback) {
    m_callback = callback;
   
    if(callback)
        std::wcout << L"[LOG] " << std::endl;
}

// 스캔 시작/정지
void StartScan(const wchar_t* nameFilter) {
    StopScan();
    IVector<hstring> props = single_threaded_vector<hstring>({
        L"System.Devices.Aep.DeviceAddress",
        L"System.Devices.Aep.IsConnected",
        L"System.Devices.Aep.Bluetooth.Le.IsConnectable"
        });

    std::wstring selector = std::format(
        L"(System.Devices.Aep.ProtocolId:=\"{{bb7bb05e-5972-42b5-94fc-76eaa7084d49}}\" AND System.ItemNameDisplay:~=\"{}\")",
        nameFilter);

    m_watcher = DeviceInformation::CreateWatcher(selector, props, DeviceInformationKind::AssociationEndpoint);
    m_watcher.Added(&DeviceAdded);
    m_watcher.Updated(&DeviceUpdated);
    m_watcher.Start();
    if (m_callback) m_callback(L"[BLE] Started scanning");
}

void StopScan() {
    if (m_watcher) {
        m_watcher.Stop();
        m_watcher = nullptr;
        if (m_callback) m_callback(L"[BLE] Scan stopped");
    }
}

// 디바이스 추가/업데이트 이벤트
void DeviceAdded(DeviceWatcher const&, DeviceInformation const& info) {
    if (m_callback) {
        m_callback(std::format(L"[BLE] Found: {}, {}", info.Name().c_str(), info.Id().c_str()).c_str());
    }
}

void DeviceUpdated(DeviceWatcher const&, DeviceInformationUpdate const&) {
    // Not used
}

// 연결/해제
void ConnectToDevice(const wchar_t* deviceId) {
    ConnectToDeviceAsync(deviceId);
}

IAsyncAction ConnectToDeviceAsync(const wchar_t* deviceId) {
    try {
        auto bleDevice = co_await BluetoothLEDevice::FromIdAsync(deviceId);
        if (!bleDevice) {
            if (m_callback) m_callback(std::format(L"[BLE] Connection failed: {}", deviceId).c_str());
            co_return;
        }

        auto result = co_await bleDevice.GetGattServicesAsync();
        if (result.Status() != GattCommunicationStatus::Success) {
            if (m_callback) m_callback(L"[BLE] Failed to get services");
            co_return;
        }

        BLEDeviceInfo info{ bleDevice, result.Services(), nullptr };
        AddDevice(deviceId, info);

        if (m_callback) m_callback(std::format(L"[BLE] Connected to device: {}", deviceId).c_str());
    }
    catch (...) {
        if (m_callback) m_callback(std::format(L"[BLE] Connection exception: {}", deviceId).c_str());
    }
}

void DisconnectDevice(const wchar_t* deviceId) {
    std::unique_lock lock(m_mutex);
    auto it = m_devices.find(deviceId);
    if (it != m_devices.end()) {
        it->second.device.Close();
        m_devices.erase(it);
        if (m_callback) m_callback(std::format(L"[BLE] Disconnected: {}", deviceId).c_str());
    }
}

void DisconnectAllDevices() {
    std::unique_lock lock(m_mutex);
    for (auto& [_, dev] : m_devices) dev.device.Close();
    m_devices.clear();
    if (m_callback) m_callback(L"[BLE] All devices disconnected");
}

void UnpairDevice(const wchar_t* deviceId) {
    auto info = DeviceInformation::CreateFromIdAsync(deviceId).get();
    if (info && info.Pairing().IsPaired()) {
        auto result = info.Pairing().UnpairAsync().get();
        if (result.Status() == DeviceUnpairingResultStatus::Unpaired && m_callback) {
            m_callback(std::format(L"[BLE] Unpaired: {}", deviceId).c_str());
        }
    }
}

// 구독
void Subscribe(const wchar_t* deviceId, const wchar_t* serviceUuid, const wchar_t* characteristicUuid) {
    SubscribeAsync(deviceId, serviceUuid, characteristicUuid);
}

void Characteristic_ValueChanged(GattCharacteristic const& sender, GattValueChangedEventArgs const& args)
{
    std::wcout << L"[LOG] " << std::endl;
    DataReader reader = DataReader::FromBuffer(args.CharacteristicValue());
    std::vector<uint8_t> data(reader.UnconsumedBufferLength());
    reader.ReadBytes(data);

    if (m_callback) {
        m_callback(std::format(L"[BLE] ValueChanged received: {} bytes", data.size()).c_str());
    }
}
IAsyncAction SubscribeAsync(const wchar_t* deviceId, const wchar_t* serviceUuid, const wchar_t* characteristicUuid) {
    try {
        auto opt = GetDevice(deviceId);
        if (!opt) {
            if (m_callback) m_callback(std::format(L"[BLE] Device not found: {}", deviceId).c_str());
            co_return;
        }
        auto& deviceInfo = *opt;
        for (auto const& service : deviceInfo.services) {
            if (ToWStringGuid(service.Uuid()) == serviceUuid) {
                auto charResult = co_await service.GetCharacteristicsAsync();
                if (charResult.Status() != GattCommunicationStatus::Success) {
                    if (m_callback) m_callback(std::format(L"[BLE] Failed to get characteristics for service: {}", serviceUuid).c_str());
                    co_return;
                }

                for (auto const& characteristic : charResult.Characteristics()) {
                    if (ToWStringGuid(characteristic.Uuid()) == characteristicUuid) {
                        auto status = co_await characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
                            GattClientCharacteristicConfigurationDescriptorValue::Notify);

                        if (status != GattCommunicationStatus::Success) {
                            if (m_callback) m_callback(std::format(L"[BLE] Failed to enable notifications for characteristic: {}", characteristicUuid).c_str());
                            co_return;
                        }

                        g_valueChangedRevoker = characteristic.ValueChanged(auto_revoke,&Characteristic_ValueChanged);

                        if (m_callback) m_callback(std::format(L"[BLE] Subscribed to characteristic: {}", characteristicUuid).c_str());
                        break;
                    }
                }
            }
        }
    }
    catch (winrt::hresult_error const& ex) {
        if (m_callback) m_callback(std::format(L"[BLE] Subscribe exception: {}", std::to_wstring(static_cast<uint32_t>(ex.code()))).c_str());
    }
}

const uint8_t* ReadCharacteristic(const wchar_t* deviceId, int index, int* length) {
    g_readBuffer = Read(deviceId, index);
    if (length) *length = static_cast<int>(g_readBuffer.size());
    return g_readBuffer.data();
}

// 읽기 / 쓰기
std::vector<uint8_t> Read(const wchar_t* deviceId, int index) {
    std::vector<uint8_t> data;
    auto opt = GetDevice(deviceId);
    if (!opt || index >= static_cast<int>(opt->characteristics.Size())) return data;

    auto result = opt->characteristics.GetAt(index).ReadValueAsync().get();
    if (result.Status() == GattCommunicationStatus::Success) {
        DataReader reader = DataReader::FromBuffer(result.Value());
        data.resize(reader.UnconsumedBufferLength());
        reader.ReadBytes(data);
    }
    return data;
}

bool Write(const wchar_t* deviceId, int index, const std::vector<uint8_t>& data) {
    auto opt = GetDevice(deviceId);
    if (!opt || index >= static_cast<int>(opt->characteristics.Size())) return false;

    DataWriter writer;
    writer.WriteBytes(data);
    auto status = opt->characteristics.GetAt(index).WriteValueAsync(writer.DetachBuffer()).get();
    return status == GattCommunicationStatus::Success;
}

// 디버깅 도우미
void ListPairedDevices() {
    auto selector = BluetoothLEDevice::GetDeviceSelectorFromPairingState(true);
    auto devices = DeviceInformation::FindAllAsync(selector).get();
    for (auto const& dev : devices) {
        if (m_callback) m_callback(std::format(L"[BLE] Paired: {}", dev.Name().c_str()).c_str());
    }
}

void ListConnectedDevices() {
    std::shared_lock lock(m_mutex);
    for (auto& [id, _] : m_devices) {
        if (m_callback) m_callback(std::format(L"[BLE] Connected: {}", id).c_str());
    }
}

void Quit() {
    StopScan();
    DisconnectAllDevices();
}

// 내부 관리
void AddDevice(const std::wstring& id, const BLEDeviceInfo& info) {
    std::unique_lock lock(m_mutex);
    m_devices[id] = info;
}

void RemoveDevice(const std::wstring& id) {
    std::unique_lock lock(m_mutex);
    m_devices.erase(id);
}

std::optional<BLEDeviceInfo> GetDevice(const std::wstring& id) {
    std::shared_lock lock(m_mutex);
    auto it = m_devices.find(id);
    if (it != m_devices.end()) {
        return std::make_optional(it->second);
    }
    return std::nullopt;
}

// 유틸 함수
std::wstring ToWStringGuid(GUID guid) {
    wchar_t buffer[64];
    swprintf_s(buffer, L"{%08x-%04x-%04x-%04x-%012llx}",
        guid.Data1, guid.Data2, guid.Data3,
        (guid.Data4[0] << 8) | guid.Data4[1],
        (static_cast<unsigned long long>(guid.Data4[2]) << 40) |
        (static_cast<unsigned long long>(guid.Data4[3]) << 32) |
        (static_cast<unsigned long long>(guid.Data4[4]) << 24) |
        (static_cast<unsigned long long>(guid.Data4[5]) << 16) |
        (static_cast<unsigned long long>(guid.Data4[6]) << 8) |
        (static_cast<unsigned long long>(guid.Data4[7])));
    return buffer;
}

std::wstring EncodeBase64(const std::vector<uint8_t>& data) {
    static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0, valb = -6;
    for (uint8_t c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (result.size() % 4) result.push_back('=');
    return std::wstring(result.begin(), result.end());
}

