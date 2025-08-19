# Roblox Kernel Driver Memory Reader

This project contains a kernel driver and user-mode application for reading memory from Roblox processes.

## Project Structure

- `centipede-driver/` - Kernel driver source code
- `um/` - User-mode application source code
  - `src/main.cpp` - Main application that attaches to Roblox and reads memory
  - `src/offsets.h` - Roblox memory offsets for various game data
  - `src/offsets.cpp` - Original offsets file (now moved to header)

## Features

The modified `main.cpp` now:

1. **Attaches to Roblox**: Searches for `RobloxPlayerBeta.exe` process instead of notepad
2. **Reads Game Data**: Attempts to read various player statistics including:
   - Health and Max Health
   - Walk Speed
   - Jump Power
   - Player Position (X, Y, Z coordinates)
3. **Smart Memory Reading**: 
   - First tries to find the actual player instance by traversing the object tree
   - Falls back to direct memory reads if player instance isn't found
4. **Real-time Updates**: Continuously reads and displays data every second

## How to Use

1. **Build the kernel driver** (requires Windows Driver Kit)
2. **Install and load the driver** with appropriate permissions
3. **Build the user-mode application**:
   ```bash
   # Using Visual Studio or your preferred C++ compiler
   cl /EHsc um/src/main.cpp /Fe:roblox_reader.exe
   ```
4. **Run the application** while Roblox is running:
   ```bash
   roblox_reader.exe
   ```

## Important Notes

- **Offsets may be outdated**: The offsets in `offsets.h` are from a specific Roblox version and may need updating for newer versions
- **Administrator privileges required**: The application needs elevated privileges to access the kernel driver
- **Anti-cheat considerations**: This type of memory reading may be detected by Roblox's anti-cheat systems
- **Educational purposes only**: This code is provided for educational and research purposes

## Data Being Read

The application attempts to read the following data from Roblox:

- **Health**: Current player health value
- **Max Health**: Maximum health capacity
- **Walk Speed**: Player movement speed
- **Jump Power**: Jump strength/height
- **Position**: 3D coordinates in the game world

## Error Handling

The application includes error handling for:
- Process not found
- Driver connection failures
- Memory read errors (expected if offsets are outdated)
- Invalid memory addresses

## Building

Make sure you have:
- Visual Studio with C++ development tools
- Windows Driver Kit (for kernel driver)
- Appropriate development environment setup

The user-mode application can be built with standard C++ compilers, while the kernel driver requires the Windows Driver Kit.
