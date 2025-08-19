#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include "roblox_dumper.h"
#include "roblox_reader.h"

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


}

int main() {
    std::cout << "=== Roblox Offset Dumper & Reader ===\n";
    std::cout << "By Centipede Driver\n\n";

    // Create driver handle
    const HANDLE driver = CreateFile(L"\\\\.\\centipede", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (driver == INVALID_HANDLE_VALUE) {
        std::cout << "[-] Failed to create driver handle\n";
        std::cout << "[-] Make sure the kernel driver is loaded\n";
        std::cin.get();
        return 1;
    }

    std::cout << "[+] Driver handle created successfully\n";

    // Initialize Roblox dumper
    RobloxDumper dumper(driver);
    if (!dumper.initialize()) {
        std::cout << "[-] Failed to initialize Roblox dumper\n";
        std::cout << "[-] Make sure Roblox is running\n";
        CloseHandle(driver);
        std::cin.get();
        return 1;
    }

    // Dump offsets
    if (!dumper.dump_offsets()) {
        std::cout << "[-] Failed to dump offsets\n";
        CloseHandle(driver);
        std::cin.get();
        return 1;
    }

    // Save offsets to file
    dumper.save_offsets_to_file("roblox_offsets.h");

    // Get dumped offsets
    RobloxOffsets offsets = dumper.get_offsets();

    // Initialize Roblox reader
    RobloxReader reader(driver, offsets);
    if (!reader.initialize()) {
        std::cout << "[-] Failed to initialize Roblox reader\n";
        CloseHandle(driver);
        std::cin.get();
        return 1;
    }

    std::cout << "\n=== Roblox Game Information ===\n";
    
    // Get game information
    uint64_t place_id = reader.get_place_id();
    uint64_t game_id = reader.get_game_id();
    uint64_t creator_id = reader.get_creator_id();
    bool game_loaded = reader.is_game_loaded();

    std::cout << "Place ID: " << place_id << "\n";
    std::cout << "Game ID: " << game_id << "\n";
    std::cout << "Creator ID: " << creator_id << "\n";
    std::cout << "Game Loaded: " << (game_loaded ? "Yes" : "No") << "\n";

    // Get local player information
    PlayerInfo local_player = reader.get_local_player_info();
    if (local_player.address != 0) {
        std::cout << "\n=== Local Player Information ===\n";
        std::cout << "Name: " << local_player.name << "\n";
        std::cout << "User ID: " << local_player.user_id << "\n";
        std::cout << "Health: " << local_player.health << "/" << local_player.max_health << "\n";
        std::cout << "Walk Speed: " << local_player.walkspeed << "\n";
        std::cout << "Position: (" << local_player.position.x << ", " << local_player.position.y << ", " << local_player.position.z << ")\n";
    }

    // Get all players
    std::vector<PlayerInfo> players = reader.get_all_players();
    if (!players.empty()) {
        std::cout << "\n=== All Players (" << players.size() << ") ===\n";
        for (const PlayerInfo& player : players) {
            std::cout << "Name: " << player.name;
            if (player.is_local_player) std::cout << " (Local)";
            std::cout << " | Health: " << player.health << "/" << player.max_health;
            std::cout << " | Position: (" << player.position.x << ", " << player.position.y << ", " << player.position.z << ")\n";
        }
    }

    // Get camera information
    CameraInfo camera = reader.get_camera_info();
    std::cout << "\n=== Camera Information ===\n";
    std::cout << "Position: (" << camera.position.x << ", " << camera.position.y << ", " << camera.position.z << ")\n";
    std::cout << "Rotation: (" << camera.rotation.x << ", " << camera.rotation.y << ", " << camera.rotation.z << ")\n";
    std::cout << "Field of View: " << camera.field_of_view << "\n";

    // Get instances in workspace
    std::vector<InstanceInfo> instances = reader.get_instances_in_workspace();
    if (!instances.empty()) {
        std::cout << "\n=== Workspace Instances (" << instances.size() << ") ===\n";
        for (const InstanceInfo& instance : instances) {
            std::cout << "Name: " << instance.name << " | Class: " << instance.class_name;
            std::cout << " | Position: (" << instance.position.x << ", " << instance.position.y << ", " << instance.position.z << ")\n";
        }
    }

    // Example: Get all BaseParts
    std::vector<InstanceInfo> base_parts = reader.get_instances_by_class("BasePart");
    if (!base_parts.empty()) {
        std::cout << "\n=== BaseParts (" << base_parts.size() << ") ===\n";
        for (const InstanceInfo& part : base_parts) {
            std::cout << "Name: " << part.name << " | Position: (" << part.position.x << ", " << part.position.y << ", " << part.position.z << ")\n";
        }
    }

    // Example: Get instances in radius around local player
    if (local_player.address != 0) {
        std::vector<InstanceInfo> nearby_instances = reader.get_instances_in_radius(local_player.position, 50.0f);
        if (!nearby_instances.empty()) {
            std::cout << "\n=== Nearby Instances (50 studs radius) (" << nearby_instances.size() << ") ===\n";
            for (const InstanceInfo& instance : nearby_instances) {
                float distance = local_player.position.distance(instance.position);
                std::cout << "Name: " << instance.name << " | Distance: " << distance << " studs\n";
            }
        }
    }

    std::cout << "\n=== Interactive Menu ===\n";
    std::cout << "1. Set local player health\n";
    std::cout << "2. Set local player walk speed\n";
    std::cout << "3. Teleport to position\n";
    std::cout << "4. Refresh player list\n";
    std::cout << "5. Exit\n";

    while (true) {
        std::cout << "\nEnter choice (1-5): ";
        int choice;
        std::cin >> choice;

        switch (choice) {
            case 1: {
                std::cout << "Enter new health value: ";
                float health;
                std::cin >> health;
                if (reader.set_player_health(local_player.address, health)) {
                    std::cout << "[+] Health set to " << health << "\n";
                } else {
                    std::cout << "[-] Failed to set health\n";
                }
                break;
            }
            case 2: {
                std::cout << "Enter new walk speed: ";
                float speed;
                std::cin >> speed;
                if (reader.set_player_walkspeed(local_player.address, speed)) {
                    std::cout << "[+] Walk speed set to " << speed << "\n";
                } else {
                    std::cout << "[-] Failed to set walk speed\n";
                }
                break;
            }
            case 3: {
                std::cout << "Enter X position: ";
                float x, y, z;
                std::cin >> x;
                std::cout << "Enter Y position: ";
                std::cin >> y;
                std::cout << "Enter Z position: ";
                std::cin >> z;
                Vector3 position(x, y, z);
                if (reader.teleport_player(local_player.address, position)) {
                    std::cout << "[+] Teleported to (" << x << ", " << y << ", " << z << ")\n";
                } else {
                    std::cout << "[-] Failed to teleport\n";
                }
                break;
            }
            case 4: {
                players = reader.get_all_players();
                std::cout << "\n=== Updated Player List (" << players.size() << ") ===\n";
                for (const PlayerInfo& player : players) {
                    std::cout << "Name: " << player.name;
                    if (player.is_local_player) std::cout << " (Local)";
                    std::cout << " | Health: " << player.health << "/" << player.max_health << "\n";
                }
                break;
            }
            case 5:
                std::cout << "Exiting...\n";
                CloseHandle(driver);
                return 0;
            default:
                std::cout << "Invalid choice. Please enter 1-5.\n";
                break;
        }
    }

    CloseHandle(driver);
    return 0;
}