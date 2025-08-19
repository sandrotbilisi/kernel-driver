#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <thread>
#include <chrono>
#include <string>

// Include offsets
#include "offsets.h"

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
		constexpr ULONG attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		constexpr ULONG read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		constexpr ULONG write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}
	
	// Helper function to read a pointer from memory
	std::uintptr_t read_pointer(HANDLE driver_handle, const std::uintptr_t addr) {
		return read_memory<std::uintptr_t>(driver_handle, addr);
	}
	
	// Helper function to read a string from memory
	std::string read_string(HANDLE driver_handle, const std::uintptr_t addr, size_t max_length = 256) {
		std::string result;
		result.reserve(max_length);
		
		for (size_t i = 0; i < max_length; i++) {
			char c = read_memory<char>(driver_handle, addr + i);
			if (c == '\0') break;
			result += c;
		}
		
		return result;
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

		DeviceIoControl(driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);

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
	
	// Function to try to find the player instance
	std::uintptr_t find_player_instance(HANDLE driver_handle, std::uintptr_t roblox_base) {
		try {
			// Try to get the DataModel first
			std::uintptr_t data_model = driver::read_pointer(driver_handle, roblox_base + dump::VRealDataModel);
			if (data_model == 0) {
				std::cout << "Failed to get DataModel\n";
				return 0;
			}
			
			// Try to get PlayerService
			std::uintptr_t player_service = driver::read_pointer(driver_handle, data_model + dump::DPlayerService);
			if (player_service == 0) {
				std::cout << "Failed to get PlayerService\n";
				return 0;
			}
			
			// Try to get LocalPlayer
			std::uintptr_t local_player = driver::read_pointer(driver_handle, player_service + dump::PLocalPlayer);
			if (local_player == 0) {
				std::cout << "Failed to get LocalPlayer\n";
				return 0;
			}
			
			std::cout << "Found LocalPlayer at: 0x" << std::hex << local_player << std::dec << "\n";
			return local_player;
			
		} catch (...) {
			std::cout << "Error finding player instance\n";
			return 0;
		}
	}

}

int main() {
	// Change from notepad.exe to RobloxPlayerBeta.exe
	const DWORD pid = get_process_id(L"RobloxPlayerBeta.exe");

	if (pid == 0) {
		std::cout << "Failed To Find Roblox Open\n";
		std::cin.get();
		return 1;
	}

	std::cout << "Found Roblox instance (PID: " << pid << ")\n";

	const HANDLE driver = CreateFile(L"\\\\.\\centipede", GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (driver == INVALID_HANDLE_VALUE) {
		std::cout << "Failed To Create Driver Handle\n";
		std::cin.get();
		return 1;
	}

	if (driver::attach_to_process(driver, pid) == true) {
		std::cout << "Successfully Attached To Roblox\n";
		
		// Get Roblox base address
		const std::uintptr_t roblox_base = get_module_base(pid, L"RobloxPlayerBeta.exe");
		if (roblox_base == 0) {
			std::cout << "Failed to get Roblox base address\n";
			CloseHandle(driver);
			std::cin.get();
			return 1;
		}
		
		std::cout << "Roblox base address: 0x" << std::hex << roblox_base << std::dec << "\n";
		
		// Try to find the player instance
		std::uintptr_t player_instance = driver::find_player_instance(driver, roblox_base);
		
		// Try to read health data
		std::cout << "\nAttempting to read health data...\n";
		std::cout << "Press Ctrl+C to exit\n\n";
		
		while (true) {
			try {
				if (player_instance != 0) {
					// Read data from the actual player instance
					float health = driver::read_memory<float>(driver, player_instance + dump::HHealth);
					float max_health = driver::read_memory<float>(driver, player_instance + dump::HMaxHealth);
					float walk_speed = driver::read_memory<float>(driver, player_instance + dump::HWalkSpeed);
					float jump_power = driver::read_memory<float>(driver, player_instance + dump::HJumpPower);
					
					// Try to read position from player instance
					float pos_x = driver::read_memory<float>(driver, player_instance + dump::PPosition);
					float pos_y = driver::read_memory<float>(driver, player_instance + dump::PPosition + 4);
					float pos_z = driver::read_memory<float>(driver, player_instance + dump::PPosition + 8);
					
					std::cout << "=== Player Data ===" << std::endl;
					std::cout << "Health: " << health << " / " << max_health << std::endl;
					std::cout << "Walk Speed: " << walk_speed << std::endl;
					std::cout << "Jump Power: " << jump_power << std::endl;
					std::cout << "Position: (" << pos_x << ", " << pos_y << ", " << pos_z << ")" << std::endl;
				} else {
					// Fallback: try to read from base address directly
					std::cout << "Player instance not found, trying direct reads..." << std::endl;
					
					std::uintptr_t health_addr = roblox_base + dump::HHealth;
					float health = driver::read_memory<float>(driver, health_addr);
					
					std::uintptr_t max_health_addr = roblox_base + dump::HMaxHealth;
					float max_health = driver::read_memory<float>(driver, max_health_addr);
					
					std::uintptr_t walk_speed_addr = roblox_base + dump::HWalkSpeed;
					float walk_speed = driver::read_memory<float>(driver, walk_speed_addr);
					
					std::cout << "Health: " << health << " / " << max_health << std::endl;
					std::cout << "Walk Speed: " << walk_speed << std::endl;
				}
				
				std::cout << "------------------------\n";
				
			} catch (...) {
				std::cout << "Error reading memory - this is expected if offsets are outdated\n";
			}
			
			// Sleep for 1 second before next read
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	} else {
		std::cout << "Failed to attach to Roblox process\n";
	}

	CloseHandle(driver);
	std::cout << "hello world" << std::endl;
	return 0;
}