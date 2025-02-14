#include "pch.h"
#include "BLEManager.h"

using namespace std;
using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Foundation::Collections;
using namespace Windows::Devices::Bluetooth::Advertisement;
using namespace Windows::Storage::Streams;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;

std::vector<DeviceInformation> bleDevices;
BluetoothLEDevice bleDevice{ nullptr };
DeviceWatcher deviceWatcher{ nullptr };
DeviceFoundCallback deviceFoundCallback = nullptr;

DeviceWatcher::Added_revoker deviceWatcherAddedRevoker;

std::wstring to_wstring_guid(GUID guid) {
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

IAsyncAction ListServices(BluetoothLEDevice device)
{
    if (device)
    {
        try
        {
            auto result = co_await device.GetGattServicesAsync();

            if (result.Status() == GattCommunicationStatus::Success)
            {
                for (auto const& s : result.Services())
                {
                    std::wcout << L"  Service: " << to_wstring_guid(s.Uuid()) << L" (" << s.AttributeHandle() << ")" << std::endl;

                    deviceFoundCallback(to_wstring_guid(s.Uuid()).c_str(), L"");
                }
            }
        }
        catch (winrt::hresult_error const& ex)
        {
            std::wcerr << L"Failed to get services." << std::endl;
        }
    }

    co_return;
}

IAsyncAction Watcher_Added(DeviceWatcher sender, DeviceInformation info)
{
    try {
        auto device = BluetoothLEDevice::FromIdAsync(info.Id()).get(); // Retrieve the BluetoothLEDevice
        if (device) {
            std::wcout << L"Device Added: " << device.Name().c_str() << L" (" << device.BluetoothAddress() << L")" << std::endl;

            ListServices(device);

            if (deviceFoundCallback)
            {
                deviceFoundCallback(device.Name().c_str(), std::to_wstring(device.BluetoothAddress()).c_str());
            }
        }
    }
    catch (winrt::hresult_error const& ex) {
        std::wcerr << L"Error retrieving device or its advertisement data: " << ex.message().c_str() << std::endl;
    }

    co_return;
}

void RegisterCallback(DeviceFoundCallback callback)
{
    deviceFoundCallback = callback;
    std::wcout << L"Callback "<< std::endl;
}

void SelectDevice(BluetoothLEDevice device)
{
    
}

void StartDeviceWatcher() {
    IVector<hstring> requestedProperties = single_threaded_vector<hstring>({ L"System.Devices.Aep.DeviceAddress", L"System.Devices.Aep.IsConnected", L"System.Devices.Aep.Bluetooth.Le.IsConnectable" });
    std::wstring deviceSelector = L"(System.Devices.Aep.ProtocolId:=\"{bb7bb05e-5972-42b5-94fc-76eaa7084d49}\")"; // list Bluetooth LE devices
    std::wcout << L"1" << std::endl;
    // 2. Create Device Watcher
    deviceWatcher = DeviceInformation::CreateWatcher(
        deviceSelector,
        requestedProperties, // Optional additional properties to retrieve
        DeviceInformationKind::AssociationEndpoint); //Signal the watcher to start enumerating the devices immediately
    std::wcout << L"2" << std::endl;
    deviceWatcher.Added(&Watcher_Added);

    deviceWatcher.Start();
    std::wcout << L"BLE 장치 검색 시작..." << std::endl;
}

void StopDeviceWatcher() {
    if (deviceWatcher) {
        deviceWatcher.Stop();
        std::wcout << L"BLE 장치 검색 중지" << std::endl;
    }
}

void ListDevices() {
    for (size_t i = 0; i < bleDevices.size(); ++i) {
        std::wcout << i << L": " << bleDevices[i].Name().c_str() << std::endl;
    }
}

bool ConnectToDevice(int index) {
    if (index < 0 || index >= bleDevices.size()) return false;

    bleDevice = BluetoothLEDevice::FromIdAsync(bleDevices[index].Id()).get();
    if (bleDevice) {
        std::wcout << L"Connected to: " << bleDevice.Name().c_str() << std::endl;
        return true;
    }
    return false;
}

void DiscoverServices() {
    if (!bleDevice) return;

    /*auto servicesResult = bleDevice.GetGattServicesAsync().get();
    auto services = servicesResult.Services();
    for (auto const& service : services)
    {
        std::wcout << L"Service UUID: " << service.Uuid() << std::endl;
    }*/
}

std::vector<uint8_t> ReadCharacteristic() {
    std::vector<uint8_t> data;
    if (!bleDevice) return data;

    /*auto servicesResult = bleDevice.GetGattServicesAsync().get();
    auto service = servicesResult.Services().GetAt(0);
    auto characteristicsResult = service.GetCharacteristicsAsync().get();
    auto characteristic = characteristicsResult.Characteristics().GetAt(0);

    auto readResult = characteristic.ReadValueAsync().get();
    if (readResult.Status() == GattCommunicationStatus::Success) {
        auto buffer = readResult.Value();
        DataReader reader = DataReader::FromBuffer(buffer);
        data.resize(reader.UnconsumedBufferLength());
        reader.ReadBytes(data);
    }*/
    return data;
}

bool WriteCharacteristic(const std::vector<uint8_t>& data) {
    if (!bleDevice) return false;

    /*auto servicesResult = bleDevice.GetGattServicesAsync().get();
    auto service = servicesResult.Services().GetAt(0);
    auto characteristicsResult = service.GetCharacteristicsAsync().get();
    auto characteristic = characteristicsResult.Characteristics().GetAt(0);

    DataWriter writer;
    writer.WriteBytes(data);
    auto buffer = writer.DetachBuffer();

    auto status = characteristic.WriteValueAsync(buffer).get();
    return status == GattCommunicationStatus::Success;*/
}