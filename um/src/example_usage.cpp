// Example usage of dumped Roblox offsets
// This file demonstrates how to use the offsets dumped by the Roblox dumper

#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>

// Include the dumped offsets
#include "roblox_offsets.h"

// Vector3 structure (same as in roblox_reader.h)
struct Vector3 {
    float x, y, z;
    
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    float distance(const Vector3& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        float dz = z - other.z;
        return sqrt(dx * dx + dy * dy + dz * dz);
    }
};

// Simple memory reading class using your driver
class MemoryReader {
private:
    HANDLE driver_handle;
    
public:
    MemoryReader() : driver_handle(INVALID_HANDLE_VALUE) {}
    
    bool initialize() {
        driver_handle = CreateFile(L"\\\\.\\centipede", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        return driver_handle != INVALID_HANDLE_VALUE;
    }
    
    ~MemoryReader() {
        if (driver_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(driver_handle);
        }
    }
    
    template<typename T>
    T read_memory(uintptr_t address) {
        T value = {};
        
        // Define the driver request structure
        struct Request {
            HANDLE process_id;
            PVOID target;
            PVOID buffer;
            SIZE_T size;
            SIZE_T return_Size;
        };
        
        Request r;
        r.target = reinterpret_cast<PVOID>(address);
        r.buffer = &value;
        r.size = sizeof(T);
        
        // Use the driver's read code
        constexpr ULONG read_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
        DeviceIoControl(driver_handle, read_code, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
        
        return value;
    }
    
    template<typename T>
    bool write_memory(uintptr_t address, const T& value) {
        struct Request {
            HANDLE process_id;
            PVOID target;
            PVOID buffer;
            SIZE_T size;
            SIZE_T return_Size;
        };
        
        Request r;
        r.target = reinterpret_cast<PVOID>(address);
        r.buffer = const_cast<PVOID>(&value);
        r.size = sizeof(T);
        
        constexpr ULONG write_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
        return DeviceIoControl(driver_handle, write_code, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
    }
    
    std::string read_string(uintptr_t address, size_t max_length = 256) {
        if (address == 0) return "";
        
        std::vector<char> buffer(max_length);
        struct Request {
            HANDLE process_id;
            PVOID target;
            PVOID buffer;
            SIZE_T size;
            SIZE_T return_Size;
        };
        
        Request r;
        r.target = reinterpret_cast<PVOID>(address);
        r.buffer = buffer.data();
        r.size = max_length;
        
        constexpr ULONG read_code = CTL_CODE(FILE_DEVICE_UNKNOWN, 0X697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
        DeviceIoControl(driver_handle, read_code, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
        
        std::string result(buffer.data());
        size_t null_pos = result.find('\0');
        if (null_pos != std::string::npos) {
            result = result.substr(0, null_pos);
        }
        return result;
    }
};

// Example: Reading player information
void read_player_info(MemoryReader& reader, uintptr_t player_address) {
    if (player_address == 0) return;
    
    // Read player name
    uintptr_t name_addr = reader.read_memory<uintptr_t>(player_address + Offsets::Name);
    std::string name = reader.read_string(name_addr);
    
    // Read player health
    float health = reader.read_memory<float>(player_address + Offsets::Health);
    float max_health = reader.read_memory<float>(player_address + Offsets::MaxHealth);
    
    // Read player position (from model instance)
    uintptr_t model_instance = reader.read_memory<uintptr_t>(player_address + Offsets::ModelInstance);
    Vector3 position(0, 0, 0);
    if (model_instance != 0) {
        position = reader.read_memory<Vector3>(model_instance + Offsets::Position);
    }
    
    // Read other player data
    float walkspeed = reader.read_memory<float>(player_address + Offsets::Walkspeed);
    uint32_t user_id = reader.read_memory<uint32_t>(player_address + Offsets::UserId);
    
    std::cout << "Player: " << name << "\n";
    std::cout << "Health: " << health << "/" << max_health << "\n";
    std::cout << "Position: (" << position.x << ", " << position.y << ", " << position.z << ")\n";
    std::cout << "Walk Speed: " << walkspeed << "\n";
    std::cout << "User ID: " << user_id << "\n";
}

// Example: Modifying player data
void modify_player(MemoryReader& reader, uintptr_t player_address) {
    if (player_address == 0) return;
    
    // Set player health to maximum
    float max_health = reader.read_memory<float>(player_address + Offsets::MaxHealth);
    reader.write_memory<float>(player_address + Offsets::Health, max_health);
    
    // Set walk speed to 50
    reader.write_memory<float>(player_address + Offsets::Walkspeed, 50.0f);
    
    // Set jump power to 100
    reader.write_memory<float>(player_address + Offsets::JumpPower, 100.0f);
    
    std::cout << "Player modified: Health restored, speed increased, jump power increased\n";
}

// Example: Teleporting player
void teleport_player(MemoryReader& reader, uintptr_t player_address, const Vector3& new_position) {
    if (player_address == 0) return;
    
    uintptr_t model_instance = reader.read_memory<uintptr_t>(player_address + Offsets::ModelInstance);
    if (model_instance != 0) {
        reader.write_memory<Vector3>(model_instance + Offsets::Position, new_position);
        std::cout << "Player teleported to (" << new_position.x << ", " << new_position.y << ", " << new_position.z << ")\n";
    }
}

// Example: Reading game information
void read_game_info(MemoryReader& reader, uintptr_t data_model_address) {
    if (data_model_address == 0) return;
    
    uint64_t place_id = reader.read_memory<uint64_t>(data_model_address + Offsets::PlaceId);
    uint64_t game_id = reader.read_memory<uint64_t>(data_model_address + Offsets::GameId);
    uint64_t creator_id = reader.read_memory<uint64_t>(data_model_address + Offsets::CreatorId);
    bool game_loaded = reader.read_memory<bool>(data_model_address + Offsets::GameLoaded);
    
    std::cout << "Game Information:\n";
    std::cout << "Place ID: " << place_id << "\n";
    std::cout << "Game ID: " << game_id << "\n";
    std::cout << "Creator ID: " << creator_id << "\n";
    std::cout << "Game Loaded: " << (game_loaded ? "Yes" : "No") << "\n";
}

// Example: Reading camera information
void read_camera_info(MemoryReader& reader, uintptr_t camera_address) {
    if (camera_address == 0) return;
    
    Vector3 position = reader.read_memory<Vector3>(camera_address + Offsets::CameraPosition);
    Vector3 rotation = reader.read_memory<Vector3>(camera_address + Offsets::CameraRotation);
    float fov = reader.read_memory<float>(camera_address + Offsets::FieldOfView);
    
    std::cout << "Camera Information:\n";
    std::cout << "Position: (" << position.x << ", " << position.y << ", " << position.z << ")\n";
    std::cout << "Rotation: (" << rotation.x << ", " << rotation.y << ", " << rotation.z << ")\n";
    std::cout << "Field of View: " << fov << "\n";
}

// Example: Finding instances by class
void find_instances_by_class(MemoryReader& reader, uintptr_t workspace_address, const std::string& class_name) {
    if (workspace_address == 0) return;
    
    std::cout << "Searching for instances of class: " << class_name << "\n";
    
    // This is a simplified example - in practice you'd need to traverse the instance tree
    // For now, we'll just read the workspace name
    uintptr_t name_addr = reader.read_memory<uintptr_t>(workspace_address + Offsets::Name);
    std::string workspace_name = reader.read_string(name_addr);
    
    std::cout << "Workspace name: " << workspace_name << "\n";
}

// Main example function
int main() {
    std::cout << "=== Roblox Offset Usage Example ===\n";
    
    MemoryReader reader;
    if (!reader.initialize()) {
        std::cout << "[-] Failed to initialize memory reader\n";
        std::cout << "[-] Make sure the kernel driver is loaded\n";
        return 1;
    }
    
    std::cout << "[+] Memory reader initialized successfully\n";
    
    // Example addresses (these would normally be found by the dumper)
    uintptr_t data_model_address = Offsets::FakeDataModelPointer; // This is just an example
    uintptr_t local_player_address = 0; // Would be found by reading DataModel + LocalPlayer offset
    uintptr_t camera_address = 0; // Would be found by reading DataModel + CurrentCamera offset
    uintptr_t workspace_address = 0; // Would be found by reading DataModel + Workspace offset
    
    // Example usage (these would work if you have valid addresses)
    if (data_model_address != 0) {
        read_game_info(reader, data_model_address);
    }
    
    if (local_player_address != 0) {
        read_player_info(reader, local_player_address);
        modify_player(reader, local_player_address);
        
        // Teleport to a new position
        Vector3 new_position(100.0f, 50.0f, 0.0f);
        teleport_player(reader, local_player_address, new_position);
    }
    
    if (camera_address != 0) {
        read_camera_info(reader, camera_address);
    }
    
    if (workspace_address != 0) {
        find_instances_by_class(reader, workspace_address, "BasePart");
    }
    
    std::cout << "\nExample completed. Check the README for more information.\n";
    return 0;
}

/*
 * How to use this example:
 * 
 * 1. Build and run the main Roblox dumper first to get valid addresses
 * 2. Copy the dumped offsets from roblox_offsets.h
 * 3. Use the addresses found by the dumper in this example
 * 4. Modify the example functions to suit your needs
 * 
 * Remember to:
 * - Run as Administrator
 * - Have the kernel driver loaded
 * - Have Roblox running
 * - Use valid memory addresses
 */
