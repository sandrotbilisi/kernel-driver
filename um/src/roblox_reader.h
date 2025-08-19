#pragma once
#include "roblox_dumper.h"
#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>

// Vector3 structure for positions
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

// Matrix4x4 structure for view matrix
struct Matrix4x4 {
    float m[4][4];
    
    Matrix4x4() {
        memset(m, 0, sizeof(m));
    }
};

// Player information structure
struct PlayerInfo {
    uintptr_t address;
    std::string name;
    Vector3 position;
    float health;
    float max_health;
    float walkspeed;
    uint32_t user_id;
    bool is_local_player;
    
    PlayerInfo() : address(0), health(0), max_health(0), walkspeed(0), user_id(0), is_local_player(false) {}
};

// Instance information structure
struct InstanceInfo {
    uintptr_t address;
    std::string name;
    std::string class_name;
    Vector3 position;
    Vector3 size;
    Vector3 velocity;
    float transparency;
    bool can_collide;
    bool can_touch;
    bool anchored;
    
    InstanceInfo() : address(0), transparency(0), can_collide(false), can_touch(false), anchored(false) {}
};

// Camera information structure
struct CameraInfo {
    Vector3 position;
    Vector3 rotation;
    float field_of_view;
    float min_zoom_distance;
    float max_zoom_distance;
    uint32_t camera_mode;
    
    CameraInfo() : field_of_view(0), min_zoom_distance(0), max_zoom_distance(0), camera_mode(0) {}
};

class RobloxReader {
private:
    HANDLE driver_handle;
    RobloxOffsets offsets;
    uintptr_t data_model_address;
    uintptr_t local_player_address;
    uintptr_t workspace_address;
    uintptr_t camera_address;
    
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
    
    std::string read_string(uintptr_t address, size_t max_length = 256) {
        if (address == 0) return "";
        
        std::vector<char> buffer(max_length);
        driver::Request r;
        r.target = reinterpret_cast<PVOID>(address);
        r.buffer = buffer.data();
        r.size = max_length;
        
        DeviceIoControl(driver_handle, driver::codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
        
        std::string result(buffer.data());
        size_t null_pos = result.find('\0');
        if (null_pos != std::string::npos) {
            result = result.substr(0, null_pos);
        }
        return result;
    }
    
    // Utility methods
    bool is_valid_pointer(uintptr_t address);
    uintptr_t get_instance_address(uintptr_t parent, const std::string& name);
    std::vector<uintptr_t> get_children(uintptr_t parent);
    
public:
    RobloxReader(HANDLE driver, const RobloxOffsets& offs);
    ~RobloxReader();
    
    bool initialize();
    
    // DataModel and core objects
    uintptr_t get_data_model();
    uintptr_t get_workspace();
    uintptr_t get_local_player();
    uintptr_t get_current_camera();
    
    // Player information
    PlayerInfo get_local_player_info();
    std::vector<PlayerInfo> get_all_players();
    PlayerInfo get_player_by_name(const std::string& name);
    
    // Instance information
    InstanceInfo get_instance_info(uintptr_t instance_address);
    std::vector<InstanceInfo> get_instances_by_class(const std::string& class_name);
    std::vector<InstanceInfo> get_instances_in_workspace();
    
    // Camera information
    CameraInfo get_camera_info();
    Matrix4x4 get_view_matrix();
    
    // Game information
    uint64_t get_place_id();
    uint64_t get_game_id();
    uint64_t get_creator_id();
    bool is_game_loaded();
    
    // Utility methods
    Vector3 world_to_screen(const Vector3& world_pos, const Matrix4x4& view_matrix, int screen_width, int screen_height);
    float get_distance_to_player(const Vector3& position);
    std::vector<InstanceInfo> get_instances_in_radius(const Vector3& center, float radius);
    
    // Health and combat
    bool set_player_health(uintptr_t player_address, float health);
    bool set_player_walkspeed(uintptr_t player_address, float speed);
    bool set_player_jump_power(uintptr_t player_address, float power);
    
    // Teleportation
    bool teleport_player(uintptr_t player_address, const Vector3& position);
    bool teleport_to_player(uintptr_t target_player_address);
    
    // ESP and visualization helpers
    std::vector<InstanceInfo> get_visible_instances();
    bool is_visible(const Vector3& from, const Vector3& to);
    
    // Getters
    RobloxOffsets get_offsets() const { return offsets; }
    uintptr_t get_data_model_address() const { return data_model_address; }
    uintptr_t get_local_player_address() const { return local_player_address; }
    uintptr_t get_workspace_address() const { return workspace_address; }
    uintptr_t get_camera_address() const { return camera_address; }
};
