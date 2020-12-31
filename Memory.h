/**
 * The Memory class is a collection functions used for external memory hacking.
 *
 * @author CasualCommunity: Alien, C3x0r, CasualGamer
 * @version 0.1 23.10.2020
*/

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <string>
#include <TlHelp32.h>
#include <iostream>

#include <dbghelp.h> //needed for ImageNtHeader

#pragma comment(lib, "dbghelp.lib")

#ifdef _MSC_VER
#pragma warning(disable : 6001)
#endif


class Address
{
public:
    Address() = default;

    Address(const uintptr_t _ptr) : ptr(_ptr)
    {
    }

    Address(const void* _ptr) : ptr(reinterpret_cast<uintptr_t> (_ptr))
    {
    }

    operator uintptr_t() const
    {
        return ptr;
    }

    explicit operator void* () const
    {
        return reinterpret_cast<void*>(ptr);
    }

    bool isValid() const
    {
        return reinterpret_cast<void*>(ptr) != nullptr;
    }

    uintptr_t get() const
    {
        return ptr;
    }

    template < typename T >
    T get()
    {
        return reinterpret_cast<T>(ptr);
    }

    Address addOffset(const uint32_t offset)
    {
        ptr += offset;
        return *this;
    }

protected:
    uintptr_t ptr;
};

std::vector<int> patternToBytes(const char* pattern);

std::string getLastErrorAsString();

class Helper {
public:
	static bool matchingBuilt(const HANDLE process);
private:
	static bool is64(const HANDLE process);
	static bool is86(const HANDLE process);
	static BOOL isWow64(const HANDLE process);
};


namespace External {
    /**
     * Serves as communicator between your application and the games memory.
     *
     * @author CasualComunity: Alien, C3x0r, CasualGamer
     * @version 0.1.2 06.11.2020
     * @todo: optimize scanner speed, optional parameter with desiredAccess, memory checks, add readString(...) to internal
    */
    class Memory
    {

    public:
        Memory(const char* proc,bool debug = false);

        ~Memory(void) noexcept;

        template <typename T>
        __forceinline T read(const uintptr_t addToBeRead) noexcept {
            T varBuff;
            ReadProcessMemory(handle, (LPBYTE*)addToBeRead, &varBuff, sizeof(varBuff), nullptr);
            return varBuff;
        }

        template <typename T>
        __forceinline T read(const Address addToBeRead) noexcept {
            T varBuff;
            ReadProcessMemory(handle, (LPBYTE*)addToBeRead.get(), &varBuff, sizeof(varBuff), nullptr);
            return varBuff;
        }

        template <typename T>
        __forceinline T write(const uintptr_t addToWrite, const T valToWrite) noexcept {
            WriteProcessMemory(handle, (LPBYTE*)addToWrite, &valToWrite, sizeof(valToWrite), nullptr);
            return valToWrite;
        }

        std::string readString(const uintptr_t addToBeRead, std::size_t size = 0) noexcept;
        std::string readString(const Address addToBeRead, std::size_t size = 0) noexcept;
        /**@brief returns process id of the target process*/
        DWORD getProcessID(void) noexcept;
        /**@brief returns the module base address of \p modName (example: "client.dll")*/
        Address getModule(const char* modName) noexcept;
        /**@brief returns the dynamic pointer from base pointer \p basePrt and \p offsets*/
        Address getAddress(const uintptr_t basePrt, const std::vector<uintptr_t>& offsets) noexcept;
        /**@brief returns the dynamic pointer from base pointer \p basePrt and \p offsets*/
        Address getAddress(Address basePrt, const std::vector<uintptr_t>& offsets) noexcept;
        bool memoryCompare(const BYTE* bData, const std::vector<int>& signature) noexcept;
        /**
        @brief a basic signature scanner
        @param start Address where to start scanning.
        @param size Size of the area to search within.
        @param sig Signature to search for. example: {-1, 0x39, 0x05, 0xF0, 0xA2, 0xF6, 0xFF} where -1 is a wild card.
        */
        Address findSignature(const uintptr_t start, const char* sig, const size_t size) noexcept;
        Address findSignature(const Address start, const char* sig, const size_t size) noexcept;

    private:
        /** @brief handle of the target process */
        HANDLE handle;
        /** @brief ID of the target process */
        DWORD processID = 0;
        /** @brief debug flag. Set true for debug messages*/
        bool debug = false;
        /**@brief initialize member variables.
        @param proc Name of the target process ("target.exe")
        @param access Desired access right to the process
        @returns whether or not initialization was successful*/
        bool init(const char* proc, const DWORD access = PROCESS_ALL_ACCESS);

    };
}

namespace Internal
{
    /**
     * Serves as communicator between injected dll and the games memory.
     *
     * @author CasualComunity: tomsa, CasualGamer
     * @version 0.2 06.11.2020
     * @todo: optimize scanner speed, memory checks
    */
    namespace Memory
    {
        template<typename T>
        __forceinline T read(uintptr_t address) {
            return *(T*)address;
        }

        template <typename T>
        __forceinline T read(const Address addToBeRead) noexcept {
            return *(T*)addToBeRead.get();
        }

        template<typename T>
        __forceinline void write(uintptr_t address, T value) {
            try { *(T*)address = value; }
            catch (...) { return; }
        }

        /**@brief returns the module base address of \p modName (example: "client.dll")*/
        Address getModule(const char* modName) noexcept;
        /**@brief returns the dynamic pointer from base pointer \p basePrt and \p offsets*/
        Address getAddress(const uintptr_t basePrt, const std::vector<uintptr_t>& offsets) noexcept;
        /**@brief returns the dynamic pointer from base pointer \p basePrt and \p offsets*/
        Address getAddress(Address basePrt, const std::vector<uintptr_t>& offsets) noexcept;
        /**
        @brief a basic signature scanner
        @param start Address where to start scanning.
        @param size Size of the area to search within.
        @param sig Signature to search for. example: {-1, 0x39, 0x05, 0xF0, 0xA2, 0xF6, 0xFF} where -1 is a wild card.
        */
        Address findSignature(uintptr_t start, const char* sig, size_t size = 0) noexcept;
        /**
        @brief a basic signature scanner
        @param start Address where to start scanning.
        @param size Size of the area to search within.
        @param sig Signature to search for. example: {-1, 0x39, 0x05, 0xF0, 0xA2, 0xF6, 0xFF} where -1 is a wild card.
        */
        Address findSignature(const Address start, const char* sig, size_t size = 0) noexcept;

        /**
        @brief a basic signature scanner searching within the address space of one module.
        @param mod name of the module ("client.dll")
        @param sig Signature to search for. example: {-1, 0x39, 0x05, 0xF0, 0xA2, 0xF6, 0xFF} where -1 is a wild card.
        */
        Address findModuleSignature(const char* mod, const char* sig) noexcept;

        template<typename T>
        T getVirtualFunction(void* baseClass, uint32_t index){
            return (*static_cast<T**>(baseClass))[index];
        }

        template<typename T, typename ... Args>
        T callVirtualFunction(void* baseClass, uint32_t index, Args... args){
            return getVirtualFunction<T(__thiscall*)(void*, Args...)>(baseClass, index)(baseClass, args...);
        }
    }
}
