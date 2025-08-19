# Roblox Offset Dumper & Reader

A comprehensive Roblox offset dumper and game data reader that integrates with a custom kernel driver for memory access. This project allows you to dump Roblox offsets dynamically and read game data such as player health, position, and more.

## Features

### 🎯 Offset Dumping
- **Dynamic Pattern Scanning**: Automatically finds offsets using memory pattern scanning
- **Version Detection**: Detects Roblox version and adapts patterns accordingly
- **Comprehensive Coverage**: Dumps offsets for health, position, camera, players, and more
- **Export Functionality**: Saves offsets to C++ header files for use in other projects

### 🎮 Game Data Reading
- **Player Information**: Read health, position, walk speed, user ID, and more
- **Instance Data**: Access workspace instances, their properties, and hierarchy
- **Camera Data**: Get camera position, rotation, field of view, and view matrix
- **Game Information**: Retrieve place ID, game ID, creator ID, and game state

### 🛠️ Interactive Features
- **Real-time Data**: Live reading of game data
- **Memory Modification**: Change player health, walk speed, and position
- **Teleportation**: Teleport players to specific coordinates
- **ESP-like Features**: Get instances in radius and visibility information

## Project Structure

```
kernel-driver/
├── centipede-driver/          # Kernel driver source
│   ├── src/
│   │   └── main.cpp          # Driver implementation
│   └── centipede-driver.inf  # Driver installation file
├── um/                       # User-mode application
│   ├── src/
│   │   ├── main.cpp          # Main application
│   │   ├── roblox_dumper.h   # Offset dumper header
│   │   ├── roblox_dumper.cpp # Offset dumper implementation
│   │   ├── roblox_reader.h   # Game data reader header
│   │   └── roblox_reader.cpp # Game data reader implementation
│   └── um.vcxproj           # Visual Studio project file
└── x64build/                # Build output directory
```

## Dumped Offsets

The dumper extracts the following offsets:

### Core Pointers
- `VisualEnginePointer` - Pointer to the visual engine
- `FakeDataModelPointer` - Pointer to the data model
- `DataModelOffset` - Offset to data model in memory

### Player Offsets
- `Health` - Player health value
- `MaxHealth` - Maximum health value
- `Position` - Player position (Vector3)
- `Velocity` - Player velocity (Vector3)
- `Walkspeed` - Player walk speed
- `JumpPower` - Player jump power
- `UserId` - Player user ID
- `Team` - Player team information

### Instance Offsets
- `Name` - Instance name
- `ClassName` - Instance class name
- `Parent` - Parent instance
- `ChildrenStart/End` - Children array
- `Transparency` - Instance transparency
- `CanCollide` - Collision flag
- `Anchored` - Anchored flag

### Camera Offsets
- `CameraPosition` - Camera position
- `CameraRotation` - Camera rotation
- `FieldOfView` - Field of view
- `ViewMatrix` - View matrix for rendering

## Building the Project

### Prerequisites
- Visual Studio 2019 or later
- Windows Driver Kit (WDK) 10
- Windows 10/11 (for kernel driver)
- Administrator privileges (for driver installation)

### Build Steps

1. **Build the Kernel Driver**:
   ```bash
   cd centipede-driver
   # Open in Visual Studio and build
   # Or use MSBuild from command line
   ```

2. **Build the User-Mode Application**:
   ```bash
   cd um
   # Open in Visual Studio and build
   # Or use MSBuild from command line
   ```

### Driver Installation

1. **Enable Test Mode** (if not already enabled):
   ```cmd
   bcdedit /set testsigning on
   ```

2. **Install the Driver**:
   ```cmd
   # Run as Administrator
   pnputil /add-driver centipede-driver.inf /install
   ```

3. **Load the Driver**:
   ```cmd
   # Run as Administrator
   sc create centipede type= kernel binPath= "C:\path\to\centipede-driver.sys"
   sc start centipede
   ```

## Usage

### Basic Usage

1. **Start Roblox** and join a game
2. **Run the application** as Administrator:
   ```cmd
   um.exe
   ```

3. **The application will**:
   - Automatically find the Roblox process
   - Dump all relevant offsets
   - Save offsets to `roblox_offsets.h`
   - Display game information
   - Provide an interactive menu

### Interactive Menu Options

1. **Set Local Player Health** - Modify your health value
2. **Set Local Player Walk Speed** - Change your movement speed
3. **Teleport to Position** - Teleport to specific coordinates
4. **Refresh Player List** - Update the list of players
5. **Exit** - Close the application

### Example Output

```
=== Roblox Offset Dumper & Reader ===
By Centipede Driver

[+] Driver handle created successfully
[+] Found Roblox process (PID: 12345)
[+] Successfully attached to Roblox process
[+] Roblox Version: version-225e87fdb7254f64
[+] Base Address: 0x7ff6b8c00000
[+] Module Size: 12345678 bytes
[+] Starting offset dump...
[+] VisualEnginePointer: 0x6676bc8
[+] FakeDataModelPointer: 0x6833728
[+] Offset dump completed successfully!
[+] Offsets saved to: roblox_offsets.h

=== Roblox Game Information ===
Place ID: 123456789
Game ID: 987654321
Creator ID: 555666777
Game Loaded: Yes

=== Local Player Information ===
Name: PlayerName
User ID: 123456789
Health: 100/100
Walk Speed: 16
Position: (10.5, 5.2, -3.1)

=== All Players (5) ===
Name: PlayerName (Local) | Health: 100/100 | Position: (10.5, 5.2, -3.1)
Name: Player2 | Health: 85/100 | Position: (15.2, 5.2, 8.7)
...
```

## Advanced Usage

### Using the Dumped Offsets

The dumped offsets are saved in `roblox_offsets.h` and can be used in other projects:

```cpp
#include "roblox_offsets.h"

// Read player health
float health = read_memory<float>(player_address + Offsets::Health);

// Read player position
Vector3 position = read_memory<Vector3>(player_address + Offsets::Position);

// Set player walk speed
float new_speed = 50.0f;
write_memory<float>(player_address + Offsets::Walkspeed, new_speed);
```

### Custom Pattern Scanning

You can add custom patterns to find specific offsets:

```cpp
// Add to roblox_dumper.cpp
uintptr_t RobloxDumper::find_custom_offset() {
    std::string pattern = "\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0";
    std::string mask = "xxx????xxx";
    
    uintptr_t result = find_pattern_in_module(pattern, mask, "RobloxPlayerBeta.exe");
    if (result != 0) {
        int32_t offset = read_memory<int32_t>(result + 3);
        return result + 7 + offset;
    }
    return 0;
}
```

## Security Considerations

⚠️ **Important**: This tool is for educational and research purposes only.

- **Anti-Cheat Detection**: Roblox has anti-cheat systems that may detect memory modifications
- **Terms of Service**: Using this tool may violate Roblox's Terms of Service
- **Account Risk**: Using memory modification tools can result in account bans
- **Legal Compliance**: Ensure compliance with local laws and regulations

## Troubleshooting

### Common Issues

1. **"Failed to create driver handle"**
   - Ensure the kernel driver is loaded
   - Run as Administrator
   - Check if driver service is running

2. **"Failed to find Roblox process"**
   - Make sure Roblox is running
   - Check if using correct process name
   - Try alternative process names

3. **"Failed to dump offsets"**
   - Roblox version may have changed
   - Update pattern signatures
   - Check if offsets are still valid

4. **"Access denied" errors**
   - Run as Administrator
   - Check Windows Defender settings
   - Verify driver permissions

### Debug Information

Enable debug output by modifying the driver:

```cpp
// In centipede-driver/src/main.cpp
void debug_print(PCSTR msg) {
    KdPrintEx((
        DPFLTR_IHVDRIVER_ID,
        DPFLTR_INFO_LEVEL,
        "Centipede: %s\n",
        msg
    ));
}
```

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for:

- New offset patterns
- Bug fixes
- Feature improvements
- Documentation updates

## License

This project is provided for educational purposes. Use at your own risk.

## Acknowledgments

- Inspired by existing Roblox dumpers like [Deni210/Roblox-Dumper](https://github.com/Deni210/Roblox-Dumper)
- Built on the foundation of kernel driver development techniques
- Uses pattern scanning techniques from reverse engineering community

## Disclaimer

This tool is for educational and research purposes only. The authors are not responsible for any misuse of this software. Users are responsible for complying with all applicable laws and terms of service.
