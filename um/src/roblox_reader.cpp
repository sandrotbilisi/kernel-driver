#include "roblox_reader.h"
#include <cmath>
#include <algorithm>

// Include the driver namespace
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
}

RobloxReader::RobloxReader(HANDLE driver, const RobloxOffsets& offs) 
    : driver_handle(driver), offsets(offs), data_model_address(0), local_player_address(0), workspace_address(0), camera_address(0) {
}

RobloxReader::~RobloxReader() {
    // Handle will be closed by the caller
}

bool RobloxReader::initialize() {
    // Get core addresses
    data_model_address = get_data_model();
    if (data_model_address == 0) {
        std::cout << "[-] Failed to get DataModel address\n";
        return false;
    }
    
    workspace_address = get_workspace();
    local_player_address = get_local_player();
    camera_address = get_current_camera();
    
    std::cout << "[+] DataModel: 0x" << std::hex << data_model_address << std::dec << "\n";
    std::cout << "[+] Workspace: 0x" << std::hex << workspace_address << std::dec << "\n";
    std::cout << "[+] LocalPlayer: 0x" << std::hex << local_player_address << std::dec << "\n";
    std::cout << "[+] Camera: 0x" << std::hex << camera_address << std::dec << "\n";
    
    return true;
}

uintptr_t RobloxReader::get_data_model() {
    // This would typically be found through pattern scanning
    // For now, we'll use a simplified approach
    return offsets.FakeDataModelPointer;
}

uintptr_t RobloxReader::get_workspace() {
    if (data_model_address == 0) return 0;
    return read_memory<uintptr_t>(data_model_address + offsets.Workspace);
}

uintptr_t RobloxReader::get_local_player() {
    if (data_model_address == 0) return 0;
    return read_memory<uintptr_t>(data_model_address + offsets.LocalPlayer);
}

uintptr_t RobloxReader::get_current_camera() {
    if (data_model_address == 0) return 0;
    return read_memory<uintptr_t>(data_model_address + offsets.CurrentCamera);
}

PlayerInfo RobloxReader::get_local_player_info() {
    PlayerInfo info;
    if (local_player_address == 0) return info;
    
    info.address = local_player_address;
    info.is_local_player = true;
    
    // Read player data
    uintptr_t name_addr = read_memory<uintptr_t>(local_player_address + offsets.Name);
    info.name = read_string(name_addr);
    
    uintptr_t model_instance = read_memory<uintptr_t>(local_player_address + offsets.ModelInstance);
    if (model_instance != 0) {
        info.position = read_memory<Vector3>(model_instance + offsets.Position);
        info.velocity = read_memory<Vector3>(model_instance + offsets.Velocity);
    }
    
    info.health = read_memory<float>(local_player_address + offsets.Health);
    info.max_health = read_memory<float>(local_player_address + offsets.MaxHealth);
    info.walkspeed = read_memory<float>(local_player_address + offsets.Walkspeed);
    info.user_id = read_memory<uint32_t>(local_player_address + offsets.UserId);
    
    return info;
}

std::vector<PlayerInfo> RobloxReader::get_all_players() {
    std::vector<PlayerInfo> players;
    
    if (data_model_address == 0) return players;
    
    // Get Players service
    uintptr_t players_service = get_instance_address(data_model_address, "Players");
    if (players_service == 0) return players;
    
    // Get all players
    std::vector<uintptr_t> player_instances = get_children(players_service);
    
    for (uintptr_t player_addr : player_instances) {
        if (player_addr == 0) continue;
        
        PlayerInfo info;
        info.address = player_addr;
        
        // Read player data
        uintptr_t name_addr = read_memory<uintptr_t>(player_addr + offsets.Name);
        info.name = read_string(name_addr);
        
        uintptr_t model_instance = read_memory<uintptr_t>(player_addr + offsets.ModelInstance);
        if (model_instance != 0) {
            info.position = read_memory<Vector3>(model_instance + offsets.Position);
            info.velocity = read_memory<Vector3>(model_instance + offsets.Velocity);
        }
        
        info.health = read_memory<float>(player_addr + offsets.Health);
        info.max_health = read_memory<float>(player_addr + offsets.MaxHealth);
        info.walkspeed = read_memory<float>(player_addr + offsets.Walkspeed);
        info.user_id = read_memory<uint32_t>(player_addr + offsets.UserId);
        info.is_local_player = (player_addr == local_player_address);
        
        players.push_back(info);
    }
    
    return players;
}

PlayerInfo RobloxReader::get_player_by_name(const std::string& name) {
    std::vector<PlayerInfo> players = get_all_players();
    
    for (const PlayerInfo& player : players) {
        if (player.name == name) {
            return player;
        }
    }
    
    return PlayerInfo(); // Return empty player if not found
}

InstanceInfo RobloxReader::get_instance_info(uintptr_t instance_address) {
    InstanceInfo info;
    if (instance_address == 0) return info;
    
    info.address = instance_address;
    
    // Read instance data
    uintptr_t name_addr = read_memory<uintptr_t>(instance_address + offsets.Name);
    info.name = read_string(name_addr);
    
    uintptr_t class_descriptor = read_memory<uintptr_t>(instance_address + offsets.ClassDescriptor);
    if (class_descriptor != 0) {
        uintptr_t class_name_addr = read_memory<uintptr_t>(class_descriptor + offsets.ClassName);
        info.class_name = read_string(class_name_addr);
    }
    
    // Read primitive data if available
    uintptr_t primitive = read_memory<uintptr_t>(instance_address + offsets.Primative);
    if (primitive != 0) {
        info.position = read_memory<Vector3>(primitive + offsets.Position);
        info.size = read_memory<Vector3>(primitive + offsets.Size);
        info.velocity = read_memory<Vector3>(primitive + offsets.Velocity);
        info.transparency = read_memory<float>(primitive + offsets.Transparency);
        info.can_collide = read_memory<bool>(primitive + offsets.CanCollide);
        info.can_touch = read_memory<bool>(primitive + offsets.CanTouch);
        info.anchored = read_memory<bool>(primitive + offsets.Anchored);
    }
    
    return info;
}

std::vector<InstanceInfo> RobloxReader::get_instances_by_class(const std::string& class_name) {
    std::vector<InstanceInfo> instances;
    
    if (workspace_address == 0) return instances;
    
    // Recursively search through workspace
    std::function<void(uintptr_t)> search_instances = [&](uintptr_t parent) {
        if (parent == 0) return;
        
        std::vector<uintptr_t> children = get_children(parent);
        for (uintptr_t child : children) {
            if (child == 0) continue;
            
            InstanceInfo info = get_instance_info(child);
            if (info.class_name == class_name) {
                instances.push_back(info);
            }
            
            // Recursively search children
            search_instances(child);
        }
    };
    
    search_instances(workspace_address);
    return instances;
}

std::vector<InstanceInfo> RobloxReader::get_instances_in_workspace() {
    std::vector<InstanceInfo> instances;
    
    if (workspace_address == 0) return instances;
    
    std::function<void(uintptr_t)> collect_instances = [&](uintptr_t parent) {
        if (parent == 0) return;
        
        std::vector<uintptr_t> children = get_children(parent);
        for (uintptr_t child : children) {
            if (child == 0) continue;
            
            InstanceInfo info = get_instance_info(child);
            instances.push_back(info);
            
            // Recursively collect children
            collect_instances(child);
        }
    };
    
    collect_instances(workspace_address);
    return instances;
}

CameraInfo RobloxReader::get_camera_info() {
    CameraInfo info;
    if (camera_address == 0) return info;
    
    info.position = read_memory<Vector3>(camera_address + offsets.CameraPosition);
    info.rotation = read_memory<Vector3>(camera_address + offsets.CameraRotation);
    info.field_of_view = read_memory<float>(camera_address + offsets.FieldOfView);
    info.min_zoom_distance = read_memory<float>(camera_address + offsets.CameraMinZoomDistance);
    info.max_zoom_distance = read_memory<float>(camera_address + offsets.CameraMaxZoomDistance);
    info.camera_mode = read_memory<uint32_t>(camera_address + offsets.CameraMode);
    
    return info;
}

Matrix4x4 RobloxReader::get_view_matrix() {
    Matrix4x4 matrix;
    
    // This would typically be read from the VisualEngine
    // For now, we'll return an identity matrix
    matrix.m[0][0] = 1.0f; matrix.m[0][1] = 0.0f; matrix.m[0][2] = 0.0f; matrix.m[0][3] = 0.0f;
    matrix.m[1][0] = 0.0f; matrix.m[1][1] = 1.0f; matrix.m[1][2] = 0.0f; matrix.m[1][3] = 0.0f;
    matrix.m[2][0] = 0.0f; matrix.m[2][1] = 0.0f; matrix.m[2][2] = 1.0f; matrix.m[2][3] = 0.0f;
    matrix.m[3][0] = 0.0f; matrix.m[3][1] = 0.0f; matrix.m[3][2] = 0.0f; matrix.m[3][3] = 1.0f;
    
    return matrix;
}

uint64_t RobloxReader::get_place_id() {
    if (data_model_address == 0) return 0;
    return read_memory<uint64_t>(data_model_address + offsets.PlaceId);
}

uint64_t RobloxReader::get_game_id() {
    if (data_model_address == 0) return 0;
    return read_memory<uint64_t>(data_model_address + offsets.GameId);
}

uint64_t RobloxReader::get_creator_id() {
    if (data_model_address == 0) return 0;
    return read_memory<uint64_t>(data_model_address + offsets.CreatorId);
}

bool RobloxReader::is_game_loaded() {
    if (data_model_address == 0) return false;
    return read_memory<bool>(data_model_address + offsets.GameLoaded);
}

Vector3 RobloxReader::world_to_screen(const Vector3& world_pos, const Matrix4x4& view_matrix, int screen_width, int screen_height) {
    // Simplified world to screen projection
    // In a real implementation, you'd use proper matrix multiplication
    
    Vector3 screen_pos;
    screen_pos.x = (world_pos.x / world_pos.z) * screen_width / 2 + screen_width / 2;
    screen_pos.y = (world_pos.y / world_pos.z) * screen_height / 2 + screen_height / 2;
    screen_pos.z = world_pos.z;
    
    return screen_pos;
}

float RobloxReader::get_distance_to_player(const Vector3& position) {
    PlayerInfo local_player = get_local_player_info();
    return local_player.position.distance(position);
}

std::vector<InstanceInfo> RobloxReader::get_instances_in_radius(const Vector3& center, float radius) {
    std::vector<InstanceInfo> instances;
    std::vector<InstanceInfo> all_instances = get_instances_in_workspace();
    
    for (const InstanceInfo& instance : all_instances) {
        if (instance.position.distance(center) <= radius) {
            instances.push_back(instance);
        }
    }
    
    return instances;
}

bool RobloxReader::set_player_health(uintptr_t player_address, float health) {
    if (player_address == 0) return false;
    
    driver::Request r;
    r.target = reinterpret_cast<PVOID>(player_address + offsets.Health);
    r.buffer = &health;
    r.size = sizeof(float);
    
    return DeviceIoControl(driver_handle, driver::codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
}

bool RobloxReader::set_player_walkspeed(uintptr_t player_address, float speed) {
    if (player_address == 0) return false;
    
    driver::Request r;
    r.target = reinterpret_cast<PVOID>(player_address + offsets.Walkspeed);
    r.buffer = &speed;
    r.size = sizeof(float);
    
    return DeviceIoControl(driver_handle, driver::codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
}

bool RobloxReader::set_player_jump_power(uintptr_t player_address, float power) {
    if (player_address == 0) return false;
    
    driver::Request r;
    r.target = reinterpret_cast<PVOID>(player_address + offsets.JumpPower);
    r.buffer = &power;
    r.size = sizeof(float);
    
    return DeviceIoControl(driver_handle, driver::codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
}

bool RobloxReader::teleport_player(uintptr_t player_address, const Vector3& position) {
    if (player_address == 0) return false;
    
    uintptr_t model_instance = read_memory<uintptr_t>(player_address + offsets.ModelInstance);
    if (model_instance == 0) return false;
    
    driver::Request r;
    r.target = reinterpret_cast<PVOID>(model_instance + offsets.Position);
    r.buffer = const_cast<PVOID>(&position);
    r.size = sizeof(Vector3);
    
    return DeviceIoControl(driver_handle, driver::codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
}

bool RobloxReader::teleport_to_player(uintptr_t target_player_address) {
    if (local_player_address == 0 || target_player_address == 0) return false;
    
    PlayerInfo target_player = get_player_by_name(""); // This would need to be implemented properly
    if (target_player.address == 0) return false;
    
    return teleport_player(local_player_address, target_player.position);
}

std::vector<InstanceInfo> RobloxReader::get_visible_instances() {
    // This is a simplified implementation
    // In a real ESP, you'd do proper visibility checks
    return get_instances_in_workspace();
}

bool RobloxReader::is_visible(const Vector3& from, const Vector3& to) {
    // Simplified visibility check
    // In a real implementation, you'd do proper raycasting
    return true;
}

bool RobloxReader::is_valid_pointer(uintptr_t address) {
    if (address < 0x1000 || address > 0x7FFFFFFFFFFF) return false;
    
    try {
        uint8_t test = read_memory<uint8_t>(address);
        return true;
    } catch (...) {
        return false;
    }
}

uintptr_t RobloxReader::get_instance_address(uintptr_t parent, const std::string& name) {
    if (parent == 0) return 0;
    
    std::vector<uintptr_t> children = get_children(parent);
    for (uintptr_t child : children) {
        if (child == 0) continue;
        
        uintptr_t name_addr = read_memory<uintptr_t>(child + offsets.Name);
        std::string child_name = read_string(name_addr);
        
        if (child_name == name) {
            return child;
        }
    }
    
    return 0;
}

std::vector<uintptr_t> RobloxReader::get_children(uintptr_t parent) {
    std::vector<uintptr_t> children;
    
    if (parent == 0) return children;
    
    uintptr_t children_start = read_memory<uintptr_t>(parent + offsets.ChildrenStart);
    uintptr_t children_end = read_memory<uintptr_t>(parent + offsets.ChildrenEnd);
    
    if (children_start == 0 || children_end == 0) return children;
    
    // Read children array
    size_t child_count = (children_end - children_start) / sizeof(uintptr_t);
    if (child_count > 1000) return children; // Sanity check
    
    for (size_t i = 0; i < child_count; i++) {
        uintptr_t child = read_memory<uintptr_t>(children_start + i * sizeof(uintptr_t));
        if (is_valid_pointer(child)) {
            children.push_back(child);
        }
    }
    
    return children;
}
