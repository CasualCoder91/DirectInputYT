// dllmain.cpp : Defines the entry point for the DLL application.
#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>
#include<iostream>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib, "Dinput8.lib")
#pragma comment(lib, "Dxguid.lib")

#include "Memory.h"
#pragma comment(lib, "CasualLibrary.lib")

#include "detours.h"
#pragma comment(lib, "detours.lib")

#include "Menu.h"

HMODULE myhModule;

DWORD __stdcall EjectThread(LPVOID lpParameter) {
    Sleep(100);
    FreeLibraryAndExitThread(myhModule, 0);
    return 0;
}

uintptr_t getFunctionAddress(LPDIRECTINPUTDEVICE8 lpdid, int index) {
    uintptr_t avTable = Internal::Memory::read<uintptr_t>(lpdid);
    uintptr_t pFunction = avTable + index * sizeof(uintptr_t);
    return Internal::Memory::read<uintptr_t>(pFunction);
}

typedef HRESULT(__stdcall* GetDeviceStateT)(IDirectInputDevice8* pThis, DWORD cbData, LPVOID lpvData);
GetDeviceStateT pGetDeviceState = nullptr; //pointer to original function

bool blockMouse = false;
HRESULT __stdcall hookGetDeviceState(IDirectInputDevice8* pThis, DWORD cbData, LPVOID lpvData) {
    HRESULT result = pGetDeviceState(pThis, cbData, lpvData);

    if (result == DI_OK) {
        if (cbData == sizeof(DIMOUSESTATE)) {//caller is a mouse
            if (((LPDIMOUSESTATE)lpvData)->rgbButtons[0] != 0) {
                std::cout << "[LMB]" << std::endl;
            }
            if (((LPDIMOUSESTATE)lpvData)->rgbButtons[1] != 0) {
                std::cout << "[RMB]" << std::endl;
            }
        }
        if (cbData == sizeof(DIMOUSESTATE2)) {//caller is also a mouse but different struct
            if (((LPDIMOUSESTATE2)lpvData)->rgbButtons[0] != 0) {
                std::cout << "[LMB2]" << std::endl;
            }
            if (((LPDIMOUSESTATE2)lpvData)->rgbButtons[1] != 0) {
                std::cout << "[RMB2]" << std::endl;
            }
        }
        if (blockMouse) { // I know that in our case the struct is LPDIMOUSESTATE2
            ((LPDIMOUSESTATE2)lpvData)->rgbButtons[0] = 0;
            ((LPDIMOUSESTATE2)lpvData)->rgbButtons[1] = 0;
        }
    }

    return result;

}

typedef HRESULT(__stdcall* GetDeviceDataT)(IDirectInputDevice8*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
GetDeviceDataT pGetDeviceData = nullptr;


bool blockKeyboard = false;
HRESULT __stdcall hookGetDeviceData(IDirectInputDevice8* pThis, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags) {
    HRESULT result = pGetDeviceData(pThis, cbObjectData, rgdod, pdwInOut, dwFlags);

    if (result == DI_OK) {
        for (int i = 0; i < *pdwInOut; ++i) {
            if (LOBYTE(rgdod[i].dwData) > 0) { //key down
                if (rgdod[i].dwOfs == DIK_W) {
                    std::cout << "[w] pressed" << std::endl;
                }
            }
            if (LOBYTE(rgdod[i].dwData) == 0) { //key up
                if (rgdod[i].dwOfs == DIK_W) {
                    std::cout << "[w] released" << std::endl;
                }
            }
        }
        if (blockKeyboard) {
            *pdwInOut = 0; //set array size 0
        }
    }
    return result;
}

BOOL CALLBACK DIEnumDevicesProc(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef){
    wchar_t wGUID[40] = { 0 };
    char cGUID[40] = { 0 };

    if (StringFromGUID2(lpddi->guidProduct, wGUID, ARRAYSIZE(wGUID)))
    {
        WideCharToMultiByte(CP_ACP, 0, wGUID, -1, cGUID, 40, NULL, NULL);
        std::cout << "ProductName: " << lpddi->tszProductName << std::endl;
        std::cout << "ProductGuid: " << cGUID << std::endl << std::endl;
    }
    return DIENUM_CONTINUE;
}

DWORD WINAPI Menue() {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout); // output only

    IDirectInput8* pDirectInput = nullptr;
    if (DirectInput8Create(myhModule, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&pDirectInput, NULL) != DI_OK) {
        std::cout << "DirectInput8Create failed!" << std::endl;
        return -1;
    }

    LPDIRECTINPUTDEVICE8 lpdiMouse;

    if (pDirectInput->CreateDevice(GUID_SysMouse, &lpdiMouse, NULL) != DI_OK) {
        pDirectInput->Release();
        std::cout << "CreateDevice failed!" << std::endl;
        return -1;
    }

    //hook
    pGetDeviceState = (GetDeviceStateT)DetourFunction((PBYTE)getFunctionAddress(lpdiMouse, 9), (PBYTE)hookGetDeviceState);
    pGetDeviceData = (GetDeviceDataT)DetourFunction((PBYTE)getFunctionAddress(lpdiMouse, 10), (PBYTE)hookGetDeviceData);

    Menu menu = Menu("DirectInput8 Hook Tutorial", "Press N0 to exit");
    menu.AddOption("[N1] Block Mouse", 0);
    menu.AddOption("[N2] Block Keyboard", 0);
    menu.AddOption("[N3] List Devices");
    menu.Print();

    while (true) {
        Sleep(100);
        if (GetAsyncKeyState(VK_NUMPAD0))
            break;
        if (GetAsyncKeyState(VK_NUMPAD1)) {
            blockMouse = !blockMouse;
            menu.UpdateOption(0, blockMouse);
        }
        if (GetAsyncKeyState(VK_NUMPAD2)) {
            blockKeyboard = !blockKeyboard;
            menu.UpdateOption(1, blockKeyboard);
        }
        if (GetAsyncKeyState(VK_NUMPAD3)) {
            std::cout << "Enumerating Devices ..." << std::endl;
            if (!pDirectInput->EnumDevices(DI8DEVCLASS_ALL, DIEnumDevicesProc, NULL, DIEDFL_ALLDEVICES) == DI_OK) {
                std::cout << "... failed!" << std::endl;
            }
        }
    }
    //cleanup
    pDirectInput->Release();
    lpdiMouse->Release();
    DetourRemove((PBYTE)pGetDeviceState, (PBYTE)hookGetDeviceState);
    DetourRemove((PBYTE)pGetDeviceData, (PBYTE)hookGetDeviceData);
    fclose(fp);
    FreeConsole();

    CreateThread(0, 0, EjectThread, 0, 0, 0);
    return 0;
}


BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        myhModule = hModule;
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Menue, NULL, 0, NULL);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}