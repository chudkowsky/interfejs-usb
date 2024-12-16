#include <windows.h>
#pragma option push -a1
#include <setupapi.h>
#pragma option pop
#include <iostream>
#include <assert.h>
#include <chrono>
#include <cstring>
#include <thread>

#define bufferLength 32
using namespace std;
void displayError(const char* msg){
    cout << msg << endl;
    system("PAUSE");
    exit(0);
};
//---------------------------------------------------------
template <class T>
inline void releaseMemory(T &x)
{
    assert(x != NULL);
    delete [] x;
    x = NULL;
}
//---------------------------------------------------------
GUID classGuid;
HMODULE hHidLib;
DWORD memberIndex = 0;
DWORD deviceInterfaceDetailDataSize;
HDEVINFO deviceInfoSet;
SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = NULL;
HANDLE hidDeviceObject = INVALID_HANDLE_VALUE;
BYTE inputReportBuffer[bufferLength]; //bufor danych wejściowych
DWORD numberOfBytesRead = 0;
int main()
{
    void (__stdcall *HidD_GetHidGuid)(OUT LPGUID HidGuid);
    bool (__stdcall*HidD_GetNumInputBuffers)(IN HANDLE HidDeviceObject,
                                             OUT PULONG NumberBuffers);
    hHidLib = LoadLibrary("C:\\Windows\\System32\\HID.DLL");
    if (!hHidLib)
        displayError("Błąd dołączenia biblioteki HID.DLL.");
    (FARPROC&) HidD_GetHidGuid=GetProcAddress(hHidLib, "HidD_GetHidGuid");
    (FARPROC&) HidD_GetNumInputBuffers=GetProcAddress(hHidLib,
                                                      "HidD_GetNumInputBuffers");
    if (!HidD_GetHidGuid){
        FreeLibrary(hHidLib);
        displayError("Nie znaleziono jednej lub więcej funkcji eksportowych.\n");
    }
    HidD_GetHidGuid(&classGuid);
    deviceInfoSet = SetupDiGetClassDevs(&classGuid, NULL, NULL,
                                        DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    if (deviceInfoSet == INVALID_HANDLE_VALUE){
        FreeLibrary(hHidLib);
        displayError("Nie zidentyfikowano podłączonych urządzeń.\n");
    }
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    while(SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &classGuid,
                                      memberIndex, &deviceInterfaceData)){
        memberIndex++; //inkrementacja numeru interfejsu
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData,
                                        NULL, 0, &deviceInterfaceDetailDataSize, NULL);
        deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)
                new DWORD[deviceInterfaceDetailDataSize];
        deviceInterfaceDetailData->cbSize=sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        if (!SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData,
                                             deviceInterfaceDetailData, deviceInterfaceDetailDataSize,
                                             NULL, NULL)){
            releaseMemory(deviceInterfaceDetailData);
            SetupDiDestroyDeviceInfoList(deviceInfoSet);
            displayError("Nie można pobrać informacji o interfejsie.\n");
        }
        if (NULL != strstr(deviceInterfaceDetailData->DevicePath, "vid_22ba")){
            cout << "\n"<< deviceInterfaceDetailData->DevicePath << "\n";
            hidDeviceObject=CreateFile(deviceInterfaceDetailData->DevicePath,
                                       GENERIC_READ, FILE_SHARE_READ,
                                       NULL,OPEN_EXISTING,0,NULL);
            if(hidDeviceObject==INVALID_HANDLE_VALUE)
                displayError("Nie można otworzyć urządzenia do transmisji");
            else
                break;
        }
        releaseMemory(deviceInterfaceDetailData);
    };//koniec while
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    ULONG numberBuffers; //pobranie max. długości raportu wejściowego
    HidD_GetNumInputBuffers(hidDeviceObject, &numberBuffers);
    printf("liczba buforów wejściowych = %d\n",numberBuffers);

    while(true) { //cykliczny odczyt danych
        memset(&inputReportBuffer, 0x00, sizeof(inputReportBuffer));
        ReadFile(hidDeviceObject, inputReportBuffer, sizeof(inputReportBuffer),
                 &numberOfBytesRead, NULL);
        printf("%d %d %d %d %d %d %d %d\n", inputReportBuffer[0],
               inputReportBuffer[1], inputReportBuffer[2], inputReportBuffer[3],
               inputReportBuffer[4], inputReportBuffer[5], inputReportBuffer[6],
               inputReportBuffer[7]);
        this_thread::sleep_for(100ms);
        // Odczyt raportu wejściowego kończy naciśnięcie przycisku Select / (11)
        // uniwersalnego kontrolera gier
        if(inputReportBuffer[6]==64)
            break;
    }
    CloseHandle(hidDeviceObject);
    FreeLibrary(hHidLib);
    cout << endl;
    system("PAUSE");
    return 0;
}
