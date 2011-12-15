#include "SmartSoulsInternals.h"

IDebugLog		gLog("Smart Souls.log");
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

_DefinePatchHdlr(FindBestSoulGemVisitorVisitSizeCheck, 0x004B9ED0);
_DefineHookHdlr(DisplaySoulNameOnCapture, 0x007FA737);

void SmartenSkyrimSouls(void)
{
	_MemHdlr(FindBestSoulGemVisitorVisitSizeCheck).WriteUInt8(0x75);
	_MemHdlr(DisplaySoulNameOnCapture).WriteJump();
}

UInt32 GetActorSoulType(Actor* Actor)
{
	bool IsSentient = thisCall<bool>(0x007DD710, Actor);
	int Level = thisCall<UInt16>(0x007A42C0, Actor);

	return cdeclCall<int>(0x00635A20, Level, IsSentient);
}

const char* DoDisplaySoulNameOnCaptureHook(const char* CaptureMessage, Actor* TrappedActor)
{
	static char s_CaptureMessageBuffer[0x200] = {0};

	UInt32 SoulType = GetActorSoulType(TrappedActor);
	const char* SoulName = cdeclCall<const char*>(0x004F26B0, SoulType);

	sprintf_s(s_CaptureMessageBuffer, sizeof(s_CaptureMessageBuffer), "%s %s", SoulName, CaptureMessage);
	return s_CaptureMessageBuffer;
}

#define _hhName	DisplaySoulNameOnCapture
_hhBegin()
{
	_hhSetVar(Retn, 0x007FA73C);
	_hhSetVar(Call, 0x0040E990);
	__asm
	{
		mov		eax, [ebp + 0x8]
		push	eax
		call	[_hhGetVar(Call)]
		push	eax
		call	DoDisplaySoulNameOnCaptureHook
		jmp		[_hhGetVar(Retn)]
	}
}