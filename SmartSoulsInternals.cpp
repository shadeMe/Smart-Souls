#include "SmartSoulsInternals.h"

IDebugLog		gLog("Smart Souls.log");
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

_DefinePatchHdlr(FindBestSoulGemVisitorVisitSizeCheck, 0x004B9ED0);
_DefineHookHdlr(DisplaySoulNameOnCapture, 0x007FA737);
_DefineHookHdlr(SentientSoulCheck, 0x004B9E60);

void SmartenSkyrimSouls(void)
{
	_MemHdlr(FindBestSoulGemVisitorVisitSizeCheck).WriteUInt8(0x75);
	_MemHdlr(DisplaySoulNameOnCapture).WriteJump();
	_MemHdlr(SentientSoulCheck).WriteJump();
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

#define _hhName	SentientSoulCheck
_hhBegin()
{
	_hhSetVar(Retn, 0x004B9E87);
	_hhSetVar(Skip, 0x004B9F27);
	_hhSetVar(Sentient, 0x004B9E68);
	_hhSetVar(Call, 0x00934480);		// TESForm::GetFlag
	__asm
	{
		movzx   eax, byte ptr [ebp - 0x9]
		test    eax, eax
		jnz		SENTIENT				// if non-sentient, make sure we aren't using a black soul gem

		mov		ecx, [ebp + 0x8]
		mov		ecx, [ecx]
		push	0x20000					// black soul gems are marked with the (TES4)dangerous flag
		call	[_hhGetVar(Call)]
		movzx	ecx, al
		test	ecx, ecx
		jz		NEXT

		jmp		SKIP
	SENTIENT:
		jmp		[_hhGetVar(Sentient)]
	SKIP:
		jmp		[_hhGetVar(Skip)]
	NEXT:
		jmp		[_hhGetVar(Retn)]
	}
}