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

std::vector<DeviceInformation> bleDevices = {};
BluetoothLEDevice bleDevice{ nullptr };
DeviceWatcher deviceWatcher{ nullptr };
GattDeviceService deviceService{ nullptr };

DeviceFoundCallback deviceFoundCallback = nullptr;
ServiceListCallback serviceListCallback = nullptr;
ServiceConfigListCallback serviceConfigListCallback = nullptr;

IVectorView<GattDeviceService> deviceServices;
IVectorView<GattCharacteristic> characteristics;


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

// 커넥트
IAsyncAction SelectDeviceServices(const wchar_t* id)
{
        try
        {
	        if (const auto device = BluetoothLEDevice::FromIdAsync(id).get())
            {
                bleDevice = device;
                auto result = co_await device.GetGattServicesAsync();

                if (result.Status() == GattCommunicationStatus::Success)
                {
                    deviceServices = result.Services();
                    for (auto const& s : deviceServices)
                    {
                        std::wcout << L"  Service: " << to_wstring_guid(s.Uuid()) << L" (" << s.AttributeHandle() << ")" << std::endl;

                        serviceConfigListCallback(to_wstring_guid(s.Uuid()).c_str());
                    }
                }
            }
        }
        catch (winrt::hresult_error const& ex)
        {
            std::wcerr << L"Failed to get services." << std::endl;
        }

        co_return;
}

// 서비스 선택
IAsyncAction SelectSerivceCharacteristic(const wchar_t* uuid)
{
	try
	{
        for (auto const& s : deviceServices)
        {
            if (uuid == to_wstring_guid(s.Uuid()))
            {
                deviceService = s;
                GattCharacteristicsResult result = co_await s.GetCharacteristicsAsync();

                if (result.Status() == GattCommunicationStatus::Success)
                {
                    characteristics = result.Characteristics();
                    for (auto const& c: result.Characteristics())
                    {
                        std::wcout << L"  Characteristic: " << to_wstring_guid(c.Uuid()) << std::endl;
                        serviceListCallback(to_wstring_guid(c.Uuid()).c_str());
                    }
                }
            }
        }
	}
	catch (winrt::hresult_error const& ex)
	{
        std::wcerr << L"Failed to get services." << std::endl;
	}

    co_return;
}

void Watcher_Added(DeviceWatcher sender, DeviceInformation info)
{
    try {
        if (deviceWatcher == sender)
        {
            if (deviceFoundCallback)
            {
                deviceFoundCallback(info.Name().c_str(), info.Id().c_str());
            }
        }
    }
    catch (winrt::hresult_error const& ex) {
        std::wcerr << L"Error retrieving device or its advertisement data: " << ex.message().c_str() << std::endl;
    }
}

// 업데이트가 있어야 목록을 바로 가지고옴
void Watcher_Updated(DeviceWatcher sender, DeviceInformationUpdate update)
{
    try {
        
    }
    catch (winrt::hresult_error const& ex) {
        std::wcerr << L"Error retrieving device or its advertisement data: " << ex.message().c_str() << std::endl;
    }
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
    deviceWatcher.Updated(&Watcher_Updated);

    deviceWatcher.Start();
    std::wcout << L"BLE 장치 검색 시작..." << std::endl;
}

void StopDeviceWatcher() {
    if (deviceWatcher) {
        deviceWatcher.Stop();
        deviceWatcher = nullptr;
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

IAsyncAction CharateristicValueChanged(GattCharacteristic const& characteristic, GattValueChangedEventArgs args)
{
    co_return;
}
void EnableNotifications(int idx)
{
    auto characteristic = characteristics.GetAt(idx);
    auto notify = characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::Notify).get();

    if (notify == GattCommunicationStatus::Success)
    {
        characteristic.ValueChanged(&CharateristicValueChanged);
    }
}

std::vector<uint8_t> ReadCharacteristic(int index) {
    std::vector<uint8_t> data;
    if (!bleDevice) return data;

    auto servicesResult = bleDevice.GetGattServicesAsync().get();
    auto service = servicesResult.Services().GetAt(0);
    auto characteristicsResult = service.GetCharacteristicsAsync().get();
    auto characteristic = characteristicsResult.Characteristics().GetAt(0);

    auto readResult = characteristic.ReadValueAsync().get();
    if (readResult.Status() == GattCommunicationStatus::Success) {
        auto buffer = readResult.Value();
        DataReader reader = DataReader::FromBuffer(buffer);
        data.resize(reader.UnconsumedBufferLength());
        reader.ReadBytes(data);
    }
    return data;
}

bool WriteCharacteristic(const std::vector<uint8_t>& data) {
    if (!bleDevice) return false;

    auto servicesResult = bleDevice.GetGattServicesAsync().get();
    auto service = servicesResult.Services().GetAt(0);
    auto characteristicsResult = service.GetCharacteristicsAsync().get();
    auto characteristic = characteristicsResult.Characteristics().GetAt(0);

    DataWriter writer;
    writer.WriteBytes(data);
    auto buffer = writer.DetachBuffer();

    auto status = characteristic.WriteValueAsync(buffer).get();
    return status == GattCommunicationStatus::Success;
}

void DeviceFound(DeviceFoundCallback callback)
{
    deviceFoundCallback = callback;
    std::wcout << L"Callback " << std::endl;
}

void SelectDevice(const wchar_t* id)
{
    //StopDeviceWatcher(); // 얘가 멈춰있으면 서비스 조회가 안됨.
    SelectDeviceServices(id);
}

void ServiceList(ServiceListCallback callback)
{
    serviceListCallback = callback;
}

void SelectService(const wchar_t* uuid)
{
    SelectSerivceCharacteristic(uuid);
}

void ServiceConfigList(ServiceConfigListCallback callback) {
    serviceConfigListCallback = callback;
}

void QuitBLE()
{
    StopDeviceWatcher();
    deviceService.Close();
    bleDevice.Close();
    deviceService = nullptr;
    bleDevice = nullptr;
}
