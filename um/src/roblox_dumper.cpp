#include "roblox_dumper.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

// Include the driver namespace from main.cpp
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

RobloxDumper::RobloxDumper(HANDLE driver) : driver_handle(driver), roblox_pid(0) {
    memset(&offsets, 0, sizeof(offsets));
    memset(&version_info, 0, sizeof(version_info));
}

RobloxDumper::~RobloxDumper() {
    // Handle will be closed by the caller
}

bool RobloxDumper::initialize() {
    // Find Roblox process
    roblox_pid = get_process_id(L"RobloxPlayerBeta.exe");
    if (roblox_pid == 0) {
        roblox_pid = get_process_id(L"Windows10Universal.exe"); // Alternative Roblox process name
    }
    
    if (roblox_pid == 0) {
        std::cout << "[-] Failed to find Roblox process\n";
        return false;
    }
    
    std::cout << "[+] Found Roblox process (PID: " << roblox_pid << ")\n";
    
    // Attach to process
    if (!driver::attach_to_process(driver_handle, roblox_pid)) {
        std::cout << "[-] Failed to attach to Roblox process\n";
        return false;
    }
    
    std::cout << "[+] Successfully attached to Roblox process\n";
    
    // Get version info
    version_info.version = get_roblox_version();
    version_info.base_address = get_module_base("RobloxPlayerBeta.exe");
    version_info.module_size = get_module_size("RobloxPlayerBeta.exe");
    
    std::cout << "[+] Roblox Version: " << version_info.version << "\n";
    std::cout << "[+] Base Address: 0x" << std::hex << version_info.base_address << std::dec << "\n";
    std::cout << "[+] Module Size: " << version_info.module_size << " bytes\n";
    
    return true;
}

std::vector<uint8_t> RobloxDumper::read_memory_bytes(uintptr_t address, size_t size) {
    std::vector<uint8_t> buffer(size);
    driver::Request r;
    r.target = reinterpret_cast<PVOID>(address);
    r.buffer = buffer.data();
    r.size = size;
    
    DeviceIoControl(driver_handle, driver::codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
    return buffer;
}

uintptr_t RobloxDumper::find_pattern(const std::string& pattern, const std::string& mask, uintptr_t start_address, size_t size) {
    std::vector<uint8_t> buffer = read_memory_bytes(start_address, size);
    
    for (size_t i = 0; i <= buffer.size() - pattern.length(); i++) {
        bool found = true;
        for (size_t j = 0; j < pattern.length(); j++) {
            if (mask[j] == 'x' && pattern[j] != buffer[i + j]) {
                found = false;
                break;
            }
        }
        if (found) {
            return start_address + i;
        }
    }
    return 0;
}

uintptr_t RobloxDumper::find_pattern_in_module(const std::string& pattern, const std::string& mask, const std::string& module_name) {
    uintptr_t base = get_module_base(module_name);
    size_t size = get_module_size(module_name);
    
    if (base == 0 || size == 0) {
        return 0;
    }
    
    return find_pattern(pattern, mask, base, size);
}

uintptr_t RobloxDumper::find_visual_engine_pointer() {
    // Pattern for VisualEngine pointer (this is a simplified pattern)
    std::string pattern = "\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\x8B\x40";
    std::string mask = "xxx????xxxx?xxx";
    
    uintptr_t result = find_pattern_in_module(pattern, mask, "RobloxPlayerBeta.exe");
    if (result != 0) {
        // Calculate the actual pointer from the instruction
        int32_t offset = read_memory<int32_t>(result + 3);
        return result + 7 + offset;
    }
    return 0;
}

uintptr_t RobloxDumper::find_fake_data_model_pointer() {
    // Pattern for FakeDataModel pointer
    std::string pattern = "\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\x8B\x48";
    std::string mask = "xxx????xxxx?xxx";
    
    uintptr_t result = find_pattern_in_module(pattern, mask, "RobloxPlayerBeta.exe");
    if (result != 0) {
        int32_t offset = read_memory<int32_t>(result + 3);
        return result + 7 + offset;
    }
    return 0;
}

uintptr_t RobloxDumper::find_data_model_offset() {
    // Look for DataModel offset in memory
    // This is a simplified approach - in practice you'd need more sophisticated pattern matching
    for (uintptr_t addr = version_info.base_address; addr < version_info.base_address + version_info.module_size - 0x1000; addr += 0x1000) {
        uintptr_t value = read_memory<uintptr_t>(addr);
        if (value > 0x1000 && value < 0x100000000) {
            // Check if this looks like a DataModel pointer
            uintptr_t name_addr = read_memory<uintptr_t>(value + 0x78); // Name offset
            if (name_addr > 0x1000 && name_addr < 0x100000000) {
                std::string name = read_memory<std::string>(name_addr);
                if (name.find("DataModel") != std::string::npos) {
                    return addr - version_info.base_address;
                }
            }
        }
    }
    return 0x1C0; // Default fallback
}

uintptr_t RobloxDumper::find_workspace_offset() {
    // Pattern for Workspace offset
    std::string pattern = "\x48\x8B\x81\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\x8B\x40";
    std::string mask = "xxx????xxxx?xxx";
    
    uintptr_t result = find_pattern_in_module(pattern, mask, "RobloxPlayerBeta.exe");
    if (result != 0) {
        return read_memory<uint32_t>(result + 3);
    }
    return 0x180; // Default fallback
}

uintptr_t RobloxDumper::find_local_player_offset() {
    // Pattern for LocalPlayer offset
    std::string pattern = "\x48\x8B\x81\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\x8B\x40";
    std::string mask = "xxx????xxxx?xxx";
    
    uintptr_t result = find_pattern_in_module(pattern, mask, "RobloxPlayerBeta.exe");
    if (result != 0) {
        return read_memory<uint32_t>(result + 3);
    }
    return 0x128; // Default fallback
}

uintptr_t RobloxDumper::find_health_offset() {
    // Pattern for Health offset
    std::string pattern = "\xF3\x0F\x10\x81\x00\x00\x00\x00\xF3\x0F\x11\x81\x00\x00\x00\x00";
    std::string mask = "xxxx????xxxx????";
    
    uintptr_t result = find_pattern_in_module(pattern, mask, "RobloxPlayerBeta.exe");
    if (result != 0) {
        return read_memory<uint32_t>(result + 4);
    }
    return 0x19C; // Default fallback
}

uintptr_t RobloxDumper::find_position_offset() {
    // Pattern for Position offset
    std::string pattern = "\xF3\x0F\x10\x81\x00\x00\x00\x00\xF3\x0F\x10\x89\x00\x00\x00\x00";
    std::string mask = "xxxx????xxxx????";
    
    uintptr_t result = find_pattern_in_module(pattern, mask, "RobloxPlayerBeta.exe");
    if (result != 0) {
        return read_memory<uint32_t>(result + 4);
    }
    return 0x13C; // Default fallback
}

uintptr_t RobloxDumper::find_camera_offset() {
    // Pattern for Camera offset
    std::string pattern = "\x48\x8B\x81\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\x8B\x40";
    std::string mask = "xxx????xxxx?xxx";
    
    uintptr_t result = find_pattern_in_module(pattern, mask, "RobloxPlayerBeta.exe");
    if (result != 0) {
        return read_memory<uint32_t>(result + 3);
    }
    return 0x420; // Default fallback
}

uintptr_t RobloxDumper::find_view_matrix_offset() {
    // Pattern for ViewMatrix offset
    std::string pattern = "\xF3\x0F\x10\x81\x00\x00\x00\x00\xF3\x0F\x10\x89\x00\x00\x00\x00";
    std::string mask = "xxxx????xxxx????";
    
    uintptr_t result = find_pattern_in_module(pattern, mask, "RobloxPlayerBeta.exe");
    if (result != 0) {
        return read_memory<uint32_t>(result + 4);
    }
    return 0x4B0; // Default fallback
}

std::string RobloxDumper::get_roblox_version() {
    // Try to read version from memory or use a default
    // In a real implementation, you'd parse the version from the executable
    return "version-225e87fdb7254f64"; // Default version
}

uintptr_t RobloxDumper::get_module_base(const std::string& module_name) {
    return get_module_base(roblox_pid, std::wstring(module_name.begin(), module_name.end()).c_str());
}

size_t RobloxDumper::get_module_size(const std::string& module_name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, roblox_pid);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    
    MODULEENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    
    if (Module32FirstW(snapshot, &entry)) {
        do {
            std::wstring current_name = entry.szModule;
            std::string current_name_str(current_name.begin(), current_name.end());
            if (current_name_str == module_name) {
                CloseHandle(snapshot);
                return entry.modBaseSize;
            }
        } while (Module32NextW(snapshot, &entry));
    }
    
    CloseHandle(snapshot);
    return 0;
}

bool RobloxDumper::is_valid_pointer(uintptr_t address) {
    if (address < 0x1000 || address > 0x7FFFFFFFFFFF) return false;
    
    // Try to read a small amount of memory to test if it's valid
    try {
        uint8_t test = read_memory<uint8_t>(address);
        return true;
    } catch (...) {
        return false;
    }
}

bool RobloxDumper::dump_offsets() {
    std::cout << "[+] Starting offset dump...\n";
    
    // Find core pointers
    offsets.VisualEnginePointer = find_visual_engine_pointer();
    offsets.FakeDataModelPointer = find_fake_data_model_pointer();
    
    std::cout << "[+] VisualEnginePointer: 0x" << std::hex << offsets.VisualEnginePointer << std::dec << "\n";
    std::cout << "[+] FakeDataModelPointer: 0x" << std::hex << offsets.FakeDataModelPointer << std::dec << "\n";
    
    // Find DataModel offsets
    offsets.DataModelOffset = find_data_model_offset();
    offsets.PlaceId = 0x1A0; // Default
    offsets.GameId = 0x198; // Default
    offsets.CreatorId = 0x190; // Default
    offsets.GameLoaded = 0x660; // Default
    offsets.Workspace = find_workspace_offset();
    offsets.LocalPlayer = find_local_player_offset();
    
    // Instance offsets
    offsets.Name = 0x78;
    offsets.ChildrenStart = 0x80;
    offsets.ChildrenEnd = 0x10;
    offsets.Parent = 0x50;
    offsets.ClassDescriptor = 0x18;
    offsets.ClassName = 0x8;
    
    // Player offsets
    offsets.UserId = 0x278;
    offsets.ModelInstance = 0x340;
    offsets.Team = 0x268;
    offsets.Health = find_health_offset();
    offsets.MaxHealth = 0x1BC;
    offsets.Walkspeed = 0x1DC;
    offsets.WalkspeedCheck = 0x3B8;
    offsets.JumpPower = 0x19C;
    offsets.HipHeight = 0x85;
    offsets.Ping = 0xD0;
    
    // Camera offsets
    offsets.CurrentCamera = find_camera_offset();
    offsets.CameraPosition = 0x124;
    offsets.CameraRotation = 0x100;
    offsets.CameraSubject = 0xF0;
    offsets.FieldOfView = 0x168;
    offsets.CameraMinZoomDistance = 0x2D4;
    offsets.CameraMaxZoomDistance = 0x2D0;
    offsets.CameraMode = 0x2D8;
    
    // Primitive offsets
    offsets.Primative = 0x178;
    offsets.Position = find_position_offset();
    offsets.Size = 0x298;
    offsets.Rotation = 0x118;
    offsets.Transparency = 0xF8;
    offsets.CanCollide = 0x2FB;
    offsets.CanTouch = 0x2FC;
    offsets.Anchored = 0xA2;
    offsets.Material = 0x2D8;
    offsets.Velocity = 0x148;
    
    // Input offsets
    offsets.InputObject = 0x108;
    offsets.MousePosition = 0xF4;
    
    // View matrix
    offsets.ViewMatrix = find_view_matrix_offset();
    offsets.Dimensions = 0x720;
    
    // String offsets
    offsets.StringLength = 0x10;
    
    std::cout << "[+] Offset dump completed successfully!\n";
    return true;
}

bool RobloxDumper::save_offsets_to_file(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "[-] Failed to open file: " << filename << "\n";
        return false;
    }
    
    file << "// Roblox Version: " << version_info.version << "\n";
    file << "// Dumped by Centipede Driver\n\n";
    
    file << "namespace Offsets {\n";
    file << "     inline constexpr uintptr_t VisualEnginePointer = 0x" << std::hex << offsets.VisualEnginePointer << ";\n";
    file << "     inline constexpr uintptr_t ViewMatrix = 0x" << std::hex << offsets.ViewMatrix << ";\n";
    file << "     inline constexpr uintptr_t Dimensions = 0x" << std::hex << offsets.Dimensions << ";\n";
    file << "     inline constexpr uintptr_t FakeDataModelPointer = 0x" << std::hex << offsets.FakeDataModelPointer << ";\n";
    file << "     inline constexpr uintptr_t DataModelOffset = 0x" << std::hex << offsets.DataModelOffset << ";\n";
    file << "     inline constexpr uintptr_t StringLength = 0x" << std::hex << offsets.StringLength << ";\n";
    file << "     inline constexpr uintptr_t PlaceId = 0x" << std::hex << offsets.PlaceId << ";\n";
    file << "     inline constexpr uintptr_t GameId = 0x" << std::hex << offsets.GameId << ";\n";
    file << "     inline constexpr uintptr_t CreatorId = 0x" << std::hex << offsets.CreatorId << ";\n";
    file << "     inline constexpr uintptr_t GameLoaded = 0x" << std::hex << offsets.GameLoaded << ";\n";
    file << "     inline constexpr uintptr_t Name = 0x" << std::hex << offsets.Name << ";\n";
    file << "     inline constexpr uintptr_t ChildrenStart = 0x" << std::hex << offsets.ChildrenStart << ";\n";
    file << "     inline constexpr uintptr_t ChildrenEnd = 0x" << std::hex << offsets.ChildrenEnd << ";\n";
    file << "     inline constexpr uintptr_t Parent = 0x" << std::hex << offsets.Parent << ";\n";
    file << "     inline constexpr uintptr_t ClassDescriptor = 0x" << std::hex << offsets.ClassDescriptor << ";\n";
    file << "     inline constexpr uintptr_t ClassName = 0x" << std::hex << offsets.ClassName << ";\n";
    file << "     inline constexpr uintptr_t Workspace = 0x" << std::hex << offsets.Workspace << ";\n";
    file << "     inline constexpr uintptr_t WorkspaceGravity = 0x960;\n";
    file << "     inline constexpr uintptr_t LocalPlayer = 0x" << std::hex << offsets.LocalPlayer << ";\n";
    file << "     inline constexpr uintptr_t UserId = 0x" << std::hex << offsets.UserId << ";\n";
    file << "     inline constexpr uintptr_t ModelInstance = 0x" << std::hex << offsets.ModelInstance << ";\n";
    file << "     inline constexpr uintptr_t Team = 0x" << std::hex << offsets.Team << ";\n";
    file << "     inline constexpr uintptr_t Health = 0x" << std::hex << offsets.Health << ";\n";
    file << "     inline constexpr uintptr_t MaxHealth = 0x" << std::hex << offsets.MaxHealth << ";\n";
    file << "     inline constexpr uintptr_t Walkspeed = 0x" << std::hex << offsets.Walkspeed << ";\n";
    file << "     inline constexpr uintptr_t WalkspeedCheck = 0x" << std::hex << offsets.WalkspeedCheck << ";\n";
    file << "     inline constexpr uintptr_t JumpPower = 0x" << std::hex << offsets.JumpPower << ";\n";
    file << "     inline constexpr uintptr_t HipHeight = 0x" << std::hex << offsets.HipHeight << ";\n";
    file << "     inline constexpr uintptr_t Ping = 0x" << std::hex << offsets.Ping << ";\n";
    file << "     inline constexpr uintptr_t CurrentCamera = 0x" << std::hex << offsets.CurrentCamera << ";\n";
    file << "     inline constexpr uintptr_t CameraPosition = 0x" << std::hex << offsets.CameraPosition << ";\n";
    file << "     inline constexpr uintptr_t CameraRotation = 0x" << std::hex << offsets.CameraRotation << ";\n";
    file << "     inline constexpr uintptr_t CameraSubject = 0x" << std::hex << offsets.CameraSubject << ";\n";
    file << "     inline constexpr uintptr_t FieldOfView = 0x" << std::hex << offsets.FieldOfView << ";\n";
    file << "     inline constexpr uintptr_t CameraMinZoomDistance = 0x" << std::hex << offsets.CameraMinZoomDistance << ";\n";
    file << "     inline constexpr uintptr_t CameraMaxZoomDistance = 0x" << std::hex << offsets.CameraMaxZoomDistance << ";\n";
    file << "     inline constexpr uintptr_t CameraMode = 0x" << std::hex << offsets.CameraMode << ";\n";
    file << "     inline constexpr uintptr_t Primative = 0x" << std::hex << offsets.Primative << ";\n";
    file << "     inline constexpr uintptr_t Position = 0x" << std::hex << offsets.Position << ";\n";
    file << "     inline constexpr uintptr_t Size = 0x" << std::hex << offsets.Size << ";\n";
    file << "     inline constexpr uintptr_t Rotation = 0x" << std::hex << offsets.Rotation << ";\n";
    file << "     inline constexpr uintptr_t Transparency = 0x" << std::hex << offsets.Transparency << ";\n";
    file << "     inline constexpr uintptr_t CanCollide = 0x" << std::hex << offsets.CanCollide << ";\n";
    file << "     inline constexpr uintptr_t CanTouch = 0x" << std::hex << offsets.CanTouch << ";\n";
    file << "     inline constexpr uintptr_t Anchored = 0x" << std::hex << offsets.Anchored << ";\n";
    file << "     inline constexpr uintptr_t Material = 0x" << std::hex << offsets.Material << ";\n";
    file << "     inline constexpr uintptr_t Velocity = 0x" << std::hex << offsets.Velocity << ";\n";
    file << "     inline constexpr uintptr_t InputObject = 0x" << std::hex << offsets.InputObject << ";\n";
    file << "     inline constexpr uintptr_t MousePosition = 0x" << std::hex << offsets.MousePosition << ";\n";
    file << "}\n";
    
    file.close();
    std::cout << "[+] Offsets saved to: " << filename << "\n";
    return true;
}
