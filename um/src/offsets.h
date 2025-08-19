#pragma once
#include <Windows.h>

#define PlayerRebase(x) ((uintptr_t)GetModuleHandleA("RobloxPlayerBeta.exe") + x)
#define ByfronRebase(x) ((uintptr_t)GetModuleHandleA("RobloxPlayerBeta.dll") + x)

// Auto-generated offset header
// Latest Roblox Version: version-fe20d41d8fec4770 (this dump might not be based on the latest Roblox version)
namespace dump
{
	inline uintptr_t CFov = 0x168;
	inline uintptr_t CMaxZoom = 0x2B8;
	inline uintptr_t CMinZoom = 0x2BC;
	inline uintptr_t CPosition = 0x124;
	inline uintptr_t CRotation = 0x100;
	inline uintptr_t CSubject = 0xF0;
	inline uintptr_t CType = 0x2C0;
	inline uintptr_t DEngineContext = 0x80;
	inline uintptr_t DGameLoaded = 0x420; // no
	inline uintptr_t DGuiService = 0x390; // Starter..
	inline uintptr_t DJobId = 0x140;
	inline uintptr_t DPackService = 0x380; // Starter..
	inline uintptr_t DPlayerService = 0x3A0; // Starter..
	inline uintptr_t GCreatorId = 0x190;
	inline uintptr_t GPlaceId = 0x1A0;
	inline uintptr_t HHealth = 0x19C;
	inline uintptr_t HHipHeight = 0x1A8; // Visual Engine Fake DataModel to Actual DataModel
	inline uintptr_t HJumpHeight = 0x1B4;
	inline uintptr_t HJumpPower = 0x1B8;
	inline uintptr_t HMaxHealth = 0x1BC;
	inline uintptr_t HMaxSlopeAngle = 0x1C0;
	inline uintptr_t HPlatformStand = 0x1E7;
	inline uintptr_t HRigType = 0x1D0;
	inline uintptr_t HState = 0x870;
	inline uintptr_t HWalkSpeed = 0x1DC;
	inline uintptr_t HWalkSpeedCheck = 0x3B8;
	inline uintptr_t IAttributeList = 0x138;
	inline uintptr_t IAttributeNext = 0x70;
	inline uintptr_t IAttributeValue = 0x30;
	inline uintptr_t IChildren = 0x68;
	inline uintptr_t IChildrenEnd = 0x8;
	inline uintptr_t IClassDescriptor = 0x18;
	inline uintptr_t IClassName = 0x8;
	inline uintptr_t IName = 0x88;
	inline uintptr_t INameSize = 0x10;
	inline uintptr_t IOnDemandInstance = 0x30;
	inline uintptr_t IParent = 0x58;
	inline uintptr_t JJobName = 0x18;
	inline uintptr_t JJobsStart = 0x1D0;
	inline uintptr_t LAmbient = 0x110;
	inline uintptr_t LBrightness = 0x0;
	inline uintptr_t LClockTime = 0x1C0;
	inline uintptr_t LFogEnd = 0x13C;
	inline uintptr_t LFogStart = 0x140;
	inline uintptr_t MInputObject = 0x108;
	inline uintptr_t MMousePosition = 0xF4;
	inline uintptr_t PAfkDuration = 0x1F8;
	// no
	// noe
	// no
	inline uintptr_t PDisplayName = 0x118;
	inline uintptr_t PGravity = 0x120;
	inline uintptr_t PLocalPlayer = 0x128;
	inline uintptr_t PModelInstance = 0x328;
	inline uintptr_t PNewInactivityTimeout = PlayerRebase(0x5CFF4F4);
	inline uintptr_t PPlayerConfigurePtr = PlayerRebase(0x6E4F140);
	inline uintptr_t PPosition = 0x14C;
	inline uintptr_t PPrimitive = 0x178;
	inline uintptr_t PRotation = 0x130;
	inline uintptr_t PSize = 0x254;
	inline uintptr_t PTeam = 0x248;
	inline uintptr_t PTeamColor = 0xD8;
	inline uintptr_t PUserId = 0x270;
	inline uintptr_t PVelocity = 0x158;
	inline uintptr_t RFakeDataModel = 0x38; // Render Job to Fake DataModel
	inline uintptr_t RHeartbeatTask = 0xF8;
	inline uintptr_t RPhysicsJob = 0xE8;
	inline uintptr_t RRealDataModel = 0x1B0; // Render Job Fake DataModel to Real Datamodel
	inline uintptr_t RRenderView = 0x218;
	inline uintptr_t RawScheduler = PlayerRebase(0x6F42320);
	inline uintptr_t SBytecodeSize = 0x20;
	inline uintptr_t SFpsCap = 0x1B0;
	inline uintptr_t SLocalScriptEmbedded = 0x1B0;
	inline uintptr_t SModuleScriptEmbedded = 0x158;
	inline uintptr_t VDimensions = 0x720;
	inline uintptr_t VFakeDataModel = 0x700; // Visual Engine to Fake DataModel
	inline uintptr_t VRealDataModel = 0x1C0; // Visual Engine Fake DataModel to Actual DataModel
	inline uintptr_t VVD3D11 = 0x90;
	inline uintptr_t VViewMatrix = 0x4B0;
	inline uintptr_t Value = 0xD8;
	inline uintptr_t VisualEnginePtr = PlayerRebase(0x6BFD5C0);
	inline uintptr_t WGravity = 0x998;
}
