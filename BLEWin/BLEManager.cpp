// BLEManager.cpp (클래스 기반 구현)
#include "pch.h"
#include "BLEManager.h"
#pragma comment(lib, "windowsapp")
using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Storage::Streams;

void BLEManager::Initialise(std::function<void(const wchar_t*)> callback) {
    m_callback = callback;
}

void BLEManager::StartScan(const wchar_t* nameFilter) {
    StopScan();
    IVector<hstring> props = single_threaded_vector<hstring>({
        L"System.Devices.Aep.DeviceAddress",
        L"System.Devices.Aep.IsConnected",
        L"System.Devices.Aep.Bluetooth.Le.IsConnectable"
        });
    std::wstring selector = std::format(L"(System.Devices.Aep.ProtocolId:=\"{{bb7bb05e-5972-42b5-94fc-76eaa7084d49}}\" AND System.ItemNameDisplay:~=\"{}\")", nameFilter);
    m_watcher = DeviceInformation::CreateWatcher(selector, props, DeviceInformationKind::AssociationEndpoint);
    m_watcher.Added({ this, &BLEManager::DeviceAdded });
    m_watcher.Updated({ this, &BLEManager::DeviceUpdated });
    m_watcher.Start();
    if (m_callback) m_callback(L"[BLE] Started scanning");
}

void BLEManager::StopScan() {
    if (m_watcher) {
        m_watcher.Stop();
        m_watcher = nullptr;
        if (m_callback) m_callback(L"[BLE] Scan stopped");
    }
}

void BLEManager::DeviceAdded(DeviceWatcher const&, DeviceInformation const& info) {
    if (m_callback) m_callback(std::format(L"[BLE] Found: {}, {}", info.Name().c_str(), info.Id().c_str()).c_str());
}

void BLEManager::DeviceUpdated(DeviceWatcher const&, DeviceInformationUpdate const&) {
    // Optional: handle updates
}

void BLEManager::DeviceStopped(DeviceWatcher const& sender, IInspectable const&) {
    if (m_callback) {
        m_callback(L"[BLE] Device scan stopped.");
    }

    // 상태 출력 (선택사항)
    std::wstring status;
    switch (sender.Status()) {
    case DeviceWatcherStatus::Created: status = L"Created"; break;
    case DeviceWatcherStatus::Started: status = L"Started"; break;
    case DeviceWatcherStatus::EnumerationCompleted: status = L"EnumerationCompleted"; break;
    case DeviceWatcherStatus::Stopping: status = L"Stopping"; break;
    case DeviceWatcherStatus::Stopped: status = L"Stopped"; break;
    case DeviceWatcherStatus::Aborted: status = L"Aborted"; break;
    }

    if (m_callback) {
        m_callback(std::format(L"[BLE] Watcher status: {}", status).c_str());
    }
}

void BLEManager::ConnectToDevice(const wchar_t* deviceId) {
    ConnectToDeviceAsync(deviceId);
}

IAsyncAction BLEManager::ConnectToDeviceAsync(const wchar_t* deviceId)
{
    try {
        {
            std::shared_lock lock(m_mutex);
            if (m_devices.find(deviceId) != m_devices.end()) {
                if (m_callback) m_callback(std::format(L"[BLE] Already connected: {}",deviceId).c_str());
                co_return;
            }
        }

        auto device = co_await winrt::Windows::Devices::Bluetooth::BluetoothLEDevice::FromIdAsync(deviceId);
        if (!device) {
            if (m_callback) m_callback(std::format(L"[BLE] Failed to connect: null device").c_str());
            co_return;
        }

        auto result = co_await device.GetGattServicesAsync();
        if (result.Status() != winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success) {
            if (m_callback) m_callback(std::format(L"[BLE] Failed to get services").c_str());
            co_return;
        }

        BLEDeviceInfo info;
        info.device = device;
        info.services = result.Services();

        std::vector<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic> allChars;
        for (auto const& service : info.services) {
            auto charResult = co_await service.GetCharacteristicsAsync();
            if (charResult.Status() == winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus::Success) {
                for (auto const& ch : charResult.Characteristics()) {
                    allChars.push_back(ch);
                }
            }
        }
        info.characteristics = winrt::single_threaded_vector(std::move(allChars)).GetView();

        AddDevice(deviceId, info);
        if (m_callback) m_callback(std::format(L"[BLE] Connected: {}", deviceId).c_str());
    }
    catch (winrt::hresult_error const& ex) {
        if (m_callback) m_callback(std::format(L"[BLE] Connect exception: {}", std::to_wstring(static_cast<uint32_t>(ex.code()))).c_str());
    }
    co_return;
}

void BLEManager::DisconnectDevice(const std::wstring& deviceId) {
    std::unique_lock lock(m_mutex);
    auto it = m_devices.find(deviceId);
    if (it != m_devices.end()) {
        it->second.device.Close();
        m_devices.erase(it);
        if (m_callback) m_callback(std::format(L"[BLE] Disconnected: {}", deviceId).c_str());
    }
}

void BLEManager::DisconnectAllDevices() {
    std::unique_lock lock(m_mutex);
    for (auto& [_, dev] : m_devices) dev.device.Close();
    m_devices.clear();
    if (m_callback) m_callback(L"[BLE] All devices disconnected");
}

void BLEManager::UnpairDevice(const wchar_t* deviceId) {
    auto info = DeviceInformation::CreateFromIdAsync(deviceId).get();
    if (info && info.Pairing().IsPaired()) {
        auto result = info.Pairing().UnpairAsync().get();
        if (result.Status() == DeviceUnpairingResultStatus::Unpaired && m_callback) {
            m_callback(std::format(L"[BLE] Unpaired: {}", deviceId).c_str());
        }
    }
}

void BLEManager::Subscribe(const wchar_t* deviceId, const wchar_t* serviceUuid, const wchar_t* characteristicUuid) {
    SubscribeAsync(deviceId, serviceUuid, characteristicUuid);
}

IAsyncAction BLEManager::SubscribeAsync(const wchar_t* deviceId, const wchar_t* serviceUuid, const wchar_t* characteristicUuid) {
    try {
        // 장치 가져오기
        auto opt = GetDevice(deviceId);
        if (!opt) {
            if (m_callback) m_callback(std::format(L"[BLE] Device not found: {}", deviceId).c_str());
            co_return;
        }

        auto& deviceInfo = *opt;

        // 서비스 검색
        for (auto const& service : deviceInfo.services) {
            if (ToWStringGuid(service.Uuid()) == serviceUuid) {
                auto charResult = co_await service.GetCharacteristicsAsync();
                if (charResult.Status() != GattCommunicationStatus::Success) {
                    if (m_callback) m_callback(std::format(L"[BLE] Failed to get characteristics for service: {}", serviceUuid).c_str());
                    co_return;
                }

                // 특성 검색 및 알림 활성화
                for (auto const& characteristic : charResult.Characteristics()) {
                    if (ToWStringGuid(characteristic.Uuid()) == characteristicUuid) {
                        // 알림 활성화
                        auto status = co_await characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
                            GattClientCharacteristicConfigurationDescriptorValue::Notify);

                        if (status != GattCommunicationStatus::Success) {
                            if (m_callback) m_callback(std::format(L"[BLE] Failed to enable notifications for characteristic: {}", characteristicUuid).c_str());
                            co_return;
                        }

                        // ValueChanged 이벤트 핸들러 등록
                        characteristic.ValueChanged([this](auto&&, auto&& args) {
                            DataReader reader = DataReader::FromBuffer(args.CharacteristicValue());
                            std::vector<uint8_t> data(reader.UnconsumedBufferLength());
                            reader.ReadBytes(data);

                            if (m_callback) {
                                m_callback(std::format(L"[BLE] Notification received: {}", EncodeBase64(data)).c_str());
                            }
                            });

                        if (m_callback) m_callback(std::format(L"[BLE] Subscribed to characteristic: {}", characteristicUuid).c_str());
                        co_return;
                    }
                }
            }
        }

        if (m_callback) m_callback(std::format(L"[BLE] Service or characteristic not found: {}, {}", serviceUuid, characteristicUuid).c_str());
    }
    catch (winrt::hresult_error const& ex) {
        if (m_callback) m_callback(std::format(L"[BLE] Subscribe exception: {}", std::to_wstring(static_cast<uint32_t>(ex.code()))).c_str());
    }
    co_return;
}

void BLEManager::ListServices(const wchar_t* deviceId) {
    auto opt = GetDevice(deviceId);
    if (!opt) {
        if (m_callback) m_callback(std::format(L"[BLE] Device not found: {}", deviceId).c_str());
        return;
    }

    const auto& services = opt->services;
    if (!services || services.Size() == 0) {
        if (m_callback) m_callback(std::format(L"[BLE] No services found for device: {}", deviceId).c_str());
        return;
    }

    for (const auto& service : services) {
        auto uuidStr = ToWStringGuid(service.Uuid());
        if (m_callback) m_callback(std::format(L"[BLE] Service UUID: {}", uuidStr).c_str());
    }
}

void BLEManager::ListCharacteristics(const wchar_t* deviceId, const wchar_t* serviceUuid) {
    auto opt = GetDevice(deviceId);
    if (!opt) {
        if (m_callback) m_callback(std::format(L"[BLE] Device not found: {}", deviceId).c_str());
        return;
    }

    const auto& services = opt->services;
    if (!services || services.Size() == 0) {
        if (m_callback) m_callback(std::format(L"[BLE] No services found for device: {}", deviceId).c_str());
        return;
    }

    for (const auto& characteristic : opt->characteristics) {
        if (ToWStringGuid(characteristic.Service().Uuid()) == serviceUuid) {
            auto uuidStr = ToWStringGuid(characteristic.Uuid());
            if (m_callback) m_callback(std::format(L"[BLE] Characteristic UUID: {}", uuidStr).c_str());
        }
    }
}


std::vector<uint8_t> BLEManager::Read(const wchar_t* deviceId, int index) {
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

bool BLEManager::Write(const wchar_t* deviceId, int index, const std::vector<uint8_t>& data) {
    auto opt = GetDevice(deviceId);
    if (!opt || index >= static_cast<int>(opt->characteristics.Size())) return false;
    DataWriter writer;
    writer.WriteBytes(data);
    auto status = opt->characteristics.GetAt(index).WriteValueAsync(writer.DetachBuffer()).get();
    return status == GattCommunicationStatus::Success;
}

bool BLEManager::WriteByUuid(const wchar_t* deviceId, const wchar_t* serviceUuid, const wchar_t* characteristicUuid, const std::vector<uint8_t>& data) {
    auto opt = GetDevice(deviceId);
    if (!opt) return false;

    const auto& services = opt->services;
    if (!services || services.Size() == 0) {
        if (m_callback) m_callback(std::format(L"[BLE] No services found for device: {}", deviceId).c_str());
        return false;
    }

    // 서비스 탐색
    for (const auto& characteristic : opt->characteristics) {
        if (ToWStringGuid(characteristic.Service().Uuid()) == serviceUuid) {
            if (ToWStringGuid(characteristic.Uuid()) == characteristicUuid) {
                // 데이터 작성 및 전송
                DataWriter writer;
                writer.WriteBytes(data);
                auto status = characteristic.WriteValueAsync(writer.DetachBuffer()).get();
                return status == GattCommunicationStatus::Success;
            }
        }
    }

    return false;  // 일치하는 service/characteristic을 찾지 못함
}

void BLEManager::ListPairedDevices() {
    auto selector = BluetoothLEDevice::GetDeviceSelectorFromPairingState(true);
    auto devices = DeviceInformation::FindAllAsync(selector).get();
    for (auto const& dev : devices) {
        if (m_callback) m_callback(std::format(L"[BLE] Paired: {}", dev.Name().c_str()).c_str());
    }
}

void BLEManager::ListConnectedDevices() {
    std::shared_lock lock(m_mutex);
    for (auto& [id, _] : m_devices) {
        if (m_callback) m_callback(std::format(L"[BLE] Connected: {}", id).c_str());
    }
}

void BLEManager::Quit() {
    StopScan();
    DisconnectAllDevices();
}

void BLEManager::AddDevice(const std::wstring& id, const BLEDeviceInfo& info) {
    std::unique_lock lock(m_mutex);
    m_devices[id] = info;
}

void BLEManager::RemoveDevice(const std::wstring& id) {
    std::unique_lock lock(m_mutex);
    m_devices.erase(id);
}

std::optional<BLEManager::BLEDeviceInfo> BLEManager::GetDevice(const std::wstring& id) {
    std::shared_lock lock(m_mutex);
    auto it = m_devices.find(id);
    if (it != m_devices.end()) return it->second;
    return std::nullopt;
}

std::wstring BLEManager::ToWStringGuid(GUID guid) {
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

std::wstring BLEManager::EncodeBase64(const std::vector<uint8_t>& data) {
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
