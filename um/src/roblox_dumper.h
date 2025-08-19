#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <json/json.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <memory>

// Roblox offset structures
struct RobloxOffsets {
    // Core pointers
    uintptr_t VisualEnginePointer;
    uintptr_t FakeDataModelPointer;
    
    // DataModel offsets
    uintptr_t DataModelOffset;
    uintptr_t PlaceId;
    uintptr_t GameId;
    uintptr_t CreatorId;
    uintptr_t GameLoaded;
    uintptr_t Workspace;
    uintptr_t LocalPlayer;
    
    // Instance offsets
    uintptr_t Name;
    uintptr_t ChildrenStart;
    uintptr_t ChildrenEnd;
    uintptr_t Parent;
    uintptr_t ClassDescriptor;
    uintptr_t ClassName;
    
    // Player offsets
    uintptr_t UserId;
    uintptr_t ModelInstance;
    uintptr_t Team;
    uintptr_t Health;
    uintptr_t MaxHealth;
    uintptr_t Walkspeed;
    uintptr_t WalkspeedCheck;
    uintptr_t JumpPower;
    uintptr_t HipHeight;
    uintptr_t Ping;
    
    // Camera offsets
    uintptr_t CurrentCamera;
    uintptr_t CameraPosition;
    uintptr_t CameraRotation;
    uintptr_t CameraSubject;
    uintptr_t FieldOfView;
    uintptr_t CameraMinZoomDistance;
    uintptr_t CameraMaxZoomDistance;
    uintptr_t CameraMode;
    
    // Primitive offsets
    uintptr_t Primative;
    uintptr_t Position;
    uintptr_t Size;
    uintptr_t Rotation;
    uintptr_t Transparency;
    uintptr_t CanCollide;
    uintptr_t CanTouch;
    uintptr_t Anchored;
    uintptr_t Material;
    uintptr_t Velocity;
    
    // Input offsets
    uintptr_t InputObject;
    uintptr_t MousePosition;
    
    // View matrix
    uintptr_t ViewMatrix;
    uintptr_t Dimensions;
    
    // String offsets
    uintptr_t StringLength;
};

// Memory pattern structure
struct MemoryPattern {
    std::string pattern;
    std::string mask;
    int offset;
    std::string name;
};

// Roblox version info
struct RobloxVersion {
    std::string version;
    std::string build;
    uintptr_t base_address;
    size_t module_size;
};

class RobloxDumper {
private:
    HANDLE driver_handle;
    DWORD roblox_pid;
    RobloxVersion version_info;
    RobloxOffsets offsets;
    
    // Pattern scanning
    std::vector<uint8_t> read_memory_bytes(uintptr_t address, size_t size);
    uintptr_t find_pattern(const std::string& pattern, const std::string& mask, uintptr_t start_address, size_t size);
    uintptr_t find_pattern_in_module(const std::string& pattern, const std::string& mask, const std::string& module_name);
    
    // Offset finding methods
    uintptr_t find_visual_engine_pointer();
    uintptr_t find_fake_data_model_pointer();
    uintptr_t find_data_model_offset();
    uintptr_t find_workspace_offset();
    uintptr_t find_local_player_offset();
    uintptr_t find_health_offset();
    uintptr_t find_position_offset();
    uintptr_t find_camera_offset();
    uintptr_t find_view_matrix_offset();
    
    // Utility methods
    std::string get_roblox_version();
    uintptr_t get_module_base(const std::string& module_name);
    size_t get_module_size(const std::string& module_name);
    bool is_valid_pointer(uintptr_t address);
    
public:
    RobloxDumper(HANDLE driver);
    ~RobloxDumper();
    
    bool initialize();
    bool dump_offsets();
    bool save_offsets_to_file(const std::string& filename);
    RobloxOffsets get_offsets() const { return offsets; }
    RobloxVersion get_version_info() const { return version_info; }
    
    // Memory reading helpers
    template<typename T>
    T read_memory(uintptr_t address) {
        T value = {};
        driver::Request r;
        r.target = reinterpret_cast<PVOID>(address);
        r.buffer = &value;
        r.size = sizeof(T);
        
        DeviceIoControl(driver_handle, driver::codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
        return value;
    }
    
    template<typename T>
    bool write_memory(uintptr_t address, const T& value) {
        driver::Request r;
        r.target = reinterpret_cast<PVOID>(address);
        r.buffer = const_cast<PVOID>(&value);
        r.size = sizeof(T);
        
        return DeviceIoControl(driver_handle, driver::codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
    }
};
