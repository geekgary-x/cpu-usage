#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <Wbemidl.h>
#include <comdef.h>
#include <assert.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#pragma comment(lib, "wbemuuid.lib")
#include "cpumonitor.h"
HRESULT GetCpuTemperature(LPLONG pTemperature)
{
    if (pTemperature == NULL) return E_INVALIDARG;

    *pTemperature = -1;
    HRESULT ci    = CoInitialize(NULL);
    HRESULT hr    = CoInitializeSecurity(NULL,
                                      -1,
                                      NULL,
                                      NULL,
                                      RPC_C_AUTHN_LEVEL_DEFAULT,
                                      RPC_C_IMP_LEVEL_IMPERSONATE,
                                      NULL,
                                      EOAC_NONE,
                                      NULL);
    if (SUCCEEDED(hr)) {
        IWbemLocator* pLocator;
        hr = CoCreateInstance(CLSID_WbemAdministrativeLocator,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IWbemLocator,
                              (LPVOID*)&pLocator);
        if (SUCCEEDED(hr)) {
            IWbemServices* pServices;
            BSTR           ns = SysAllocString(L"root\\WMI");
            hr = pLocator->ConnectServer(ns, NULL, NULL, NULL, 0, NULL, NULL, &pServices);
            pLocator->Release();
            SysFreeString(ns);
            if (SUCCEEDED(hr)) {
                BSTR query = SysAllocString(L"SELECT * FROM MSAcpi_ThermalZoneTemperature");
                BSTR wql   = SysAllocString(L"WQL");
                IEnumWbemClassObject* pEnum;
                hr = pServices->ExecQuery(wql,
                                          query,
                                          WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                          NULL,
                                          &pEnum);
                SysFreeString(wql);
                SysFreeString(query);
                pServices->Release();
                if (SUCCEEDED(hr)) {
                    IWbemClassObject* pObject;
                    ULONG             returned;
                    hr = pEnum->Next(WBEM_INFINITE, 1, &pObject, &returned);
                    pEnum->Release();
                    if (SUCCEEDED(hr)) {
                        BSTR    temp = SysAllocString(L"CurrentTemperature");
                        VARIANT v;
                        VariantInit(&v);
                        hr = pObject->Get(temp, 0, &v, NULL, NULL);
                        pObject->Release();
                        SysFreeString(temp);
                        if (SUCCEEDED(hr)) {
                            *pTemperature = V_I4(&v);
                        }
                        VariantClear(&v);
                    }
                }
            }
            if (ci == S_OK) {
                CoUninitialize();
            }
        }
    }
    return hr;
}

int main(int argc, char** argv)
{
    LONG temp;
    GetCpuTemperature(&temp);
    printf("temp=%lf\n", ((double)temp / 10 - 273.15));

    std::srand(std::time(nullptr));
    SL::NET::CPUMemMonitor mon;
    std::thread            runner;
    std::cout << std::fixed;
    std::cout << std::setprecision(2);
    std::thread th([&]() {
        auto              counter = 0;
        std::vector<char> mem;
        while (counter++ < 20) {
            auto memusage = mon.getMemoryUsage();
            auto cpuusage = mon.getCPUUsage();
            std::cout << "Total CPU Usage: " << cpuusage.TotalUse << std::endl;
            std::cout << "Total CPU Process Usage: " << cpuusage.ProcessUse << std::endl;
            std::cout << "Physical Process Memory Usage: "
                      << SL::NET::to_PrettyBytes(memusage.PhysicalProcessUsed) << std::endl;
            std::cout << "Total Physical Process Memory Available: "
                      << SL::NET::to_PrettyBytes(memusage.PhysicalTotalAvailable) << std::endl;
            std::cout << "Total Physical Memory Usage: "
                      << SL::NET::to_PrettyBytes(memusage.PhysicalTotalUsed) << std::endl;
            std::cout << "Virtual Process Memory Usage: "
                      << SL::NET::to_PrettyBytes(memusage.VirtualProcessUsed) << std::endl;
            std::cout << "Total Virtual Process Memory Usage: "
                      << SL::NET::to_PrettyBytes(memusage.VirtualTotalAvailable) << std::endl;
            std::cout << "Total Virtual Process Memory Usage: "
                      << SL::NET::to_PrettyBytes(memusage.VirtualTotalUsed) << std::endl;

            std::this_thread::sleep_for(1s);
            if (counter == 5) {
                std::cout << "---Starting busy work in this process---" << std::endl;
                runner = std::thread([&]() {
                    auto start  = std::chrono::high_resolution_clock::now();
                    auto waited = 0;
                    while (std::chrono::duration_cast<std::chrono::seconds>(
                               std::chrono::high_resolution_clock::now() - start)
                               .count() < 5) {
                        waited += 1;
                    }
                    std::cout << "---Done with busy work in this process---" << waited << std::endl;
                });
            }
            if (counter == 10) {
                std::cout << "Allocating 20 more MBs" << std::endl;
                mem.reserve(mem.capacity() + static_cast<size_t>(1024 * 1024 * 20));
            }
            if (counter == 11) {
                std::cout << "Allocating 20 more MBs" << std::endl;
                mem.reserve(mem.capacity() + static_cast<size_t>(1024 * 1024 * 20));
            }
            if (counter == 12) {
                std::cout << "Allocating 20 more MBs" << std::endl;
                mem.reserve(mem.capacity() + static_cast<size_t>(1024 * 1024 * 20));
            }
            if (counter == 13) {
                std::cout << "Allocating 20 more MBs" << std::endl;
                mem.reserve(mem.capacity() + static_cast<size_t>(1024 * 1024 * 20));
            }
            if (counter == 14) {
                std::cout << "Deallocating the memory" << std::endl;
                mem.shrink_to_fit();
            }
        }
    });
    th.join();
    runner.join();
    return 0;
}