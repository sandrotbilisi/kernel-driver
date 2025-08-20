#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include "offsets.hpp"

// Get the process ID by name
static DWORD get_process_id(const wchar_t* process_name) {
	DWORD process_id = 0;
	HANDLE snap_shot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap_shot == INVALID_HANDLE_VALUE)
		return 0;

	PROCESSENTRY32W entry = {};
	entry.dwSize = sizeof(entry);

	if (Process32FirstW(snap_shot, &entry)) {
		do {
			if (_wcsicmp(process_name, entry.szExeFile) == 0) {
				process_id = entry.th32ProcessID;
				break;
			}
		} while (Process32NextW(snap_shot, &entry));
	}

	CloseHandle(snap_shot);
	return process_id;
}

// Get the base address of a module in a process
static std::uintptr_t get_module_base(const DWORD pid, const wchar_t* module_name) {
	std::uintptr_t module_base = 0;

	// Use bitwise OR, not logical ORa
	HANDLE snap_shot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (snap_shot == INVALID_HANDLE_VALUE)
		return 0;

	MODULEENTRY32W entry = {};
	entry.dwSize = sizeof(entry);

	if (Module32FirstW(snap_shot, &entry)) {
		do {
			if (_wcsicmp(module_name, entry.szModule) == 0) {
				module_base = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
				break;
			}
		} while (Module32NextW(snap_shot, &entry));
	}

	CloseHandle(snap_shot);
	return module_base;
}

namespace driver {
	namespace codes {
		constexpr ULONG attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X696, METHOD_BUFFERED, FILE_ANY_ACCESS);
		constexpr ULONG read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X697, METHOD_BUFFERED, FILE_ANY_ACCESS);
		constexpr ULONG write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X698, METHOD_BUFFERED, FILE_ANY_ACCESS);
	}
	struct Request {
		HANDLE process_id;
		PVOID target;
		PVOID buffer;

		SIZE_T size;
		SIZE_T return_Size;
	};

	bool attach_to_process(HANDLE driver_handle, const DWORD pid) {
		Request r;
		r.process_id = reinterpret_cast<HANDLE>(pid);

		return DeviceIoControl(driver_handle, codes::attach, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
	}

		template <class T>
	T read_memory(HANDLE driver_handle, const std::uintptr_t addr) {
		T temp = {};
		Request r;
		r.target = reinterpret_cast<PVOID>(addr);
		r.buffer = &temp;
		r.size = sizeof(temp);

		DWORD bytes_returned = 0;
		BOOL result = DeviceIoControl(driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), &bytes_returned, nullptr);
		
		if (!result) {
			DWORD error = GetLastError();
			std::cout << "Read failed at address 0x" << std::hex << addr << std::dec << " Error: " << error << std::endl;
		}
		
		return temp;
	}

	template <class T>
	void write_memory(HANDLE driver_handle, const std::uintptr_t addr, const T& value) {
		Request r;
		r.target = reinterpret_cast<PVOID>(addr);
		r.buffer = (PVOID)&value;
		r.size = sizeof(T);


		DeviceIoControl(driver_handle, codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);

	}


}

int main() {

	const DWORD pid = get_process_id(L"RobloxPlayerBeta.exe");

	if (pid == 0) {
		std::cout << "Failed To Find Roblox Open\n";
		std::cin.get();
		return 1;
	}

	std::cout << "found instance\n";

	const HANDLE driver = CreateFile(L"\\\\.\\centipede", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (driver == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		std::cout << "Failed To Create Driver Handle. Error: " << error << std::endl;
		std::cout << "Make sure the driver is loaded and running with administrator privileges." << std::endl;
		std::cin.get();
		return 1;
	}

	if (driver::attach_to_process(driver, pid) == true) {

		std::cout << "Successfully Attached To Roblox\n";

		std::uintptr_t roblox_base = get_module_base(pid, L"RobloxPlayerBeta.exe");

		std::cout << "Roblox Base: " << std::hex << roblox_base << std::dec << "\n";
		std::cout << "VRealDataModel offset: 0x" << std::hex << dump::VRealDataModel << std::dec << "\n";
		std::cout << "Calculated address: 0x" << std::hex << (roblox_base + dump::VRealDataModel) << std::dec << "\n";

		// Test read at base address to verify driver is working
		std::uint32_t test_read = driver::read_memory<std::uint32_t>(driver, roblox_base);
		std::cout << "Test read at base: 0x" << std::hex << test_read << std::dec << "\n";

		std::uintptr_t data_model = driver::read_memory<std::uintptr_t>(driver, roblox_base + dump::VRealDataModel);
		std::cout << "Data Model: " << std::hex << data_model << std::dec << "\n";
		std::cout << "Data Model Address: 0x" << std::hex << (roblox_base + dump::VRealDataModel) << std::dec << "\n";

		if (data_model == 0) {
			std::cout << "ERROR: Data Model is 0, cannot continue!\n";
			CloseHandle(driver);
			std::cin.get();
			return 1;
		}

		std::uintptr_t player_service = driver::read_memory<std::uintptr_t>(driver, data_model + dump::DPlayerService);
		std::cout << "Player Service: " << std::hex << player_service << std::dec << "\n";
		std::cout << "Player Service Address: 0x" << std::hex << (data_model + dump::DPlayerService) << std::dec << "\n";

		if (player_service == 0) {
			std::cout << "ERROR: Player Service is 0!\n";
			CloseHandle(driver);
			std::cin.get();
			return 1;
		}

		std::uintptr_t local_player = driver::read_memory<std::uintptr_t>(driver, player_service + dump::PLocalPlayer);
		std::cout << "Local Player: " << std::hex << local_player << std::dec << "\n";
		std::cout << "Local Player Address: 0x" << std::hex << (player_service + dump::PLocalPlayer) << std::dec << "\n";

		if (local_player == 0) {
			std::cout << "ERROR: Local Player is 0!\n";
			CloseHandle(driver);
			std::cin.get();
			return 1;
		}

		std::uintptr_t player_health = driver::read_memory<std::uintptr_t>(driver, local_player + dump::HHealth);
		std::cout << "Player Health: " << std::hex << player_health << std::dec << "\n";

		std::uintptr_t player_max_health = driver::read_memory<std::uintptr_t>(driver, local_player + dump::HMaxHealth);
		std::cout << "Player Max Health: " << std::hex << player_max_health << std::dec << "\n";


	}
	CloseHandle(driver);


	std::cout << "hello world" << std::endl;
	return 0;
}