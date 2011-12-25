#pragma warning(disable : 4005)
#include "SmartSoulsInternals.h"

IDebugLog					gLog("Smart Souls.log");
PluginHandle				g_pluginHandle = kPluginHandle_Invalid;
INI::INIManager*			g_INIManager = new SmartSoulsINIManager();
std::string					g_INIPath;

void SmartSoulsINIManager::Initialize()
{
	_MESSAGE("INI Path: %s", INIFile.c_str());
	std::fstream INIStream(INIFile.c_str(), std::fstream::in);
	bool CreateINI = false;

	if (INIStream.fail())
	{
		_MESSAGE("INI File not found; Creating one...");
		CreateINI = true;
	}

	INIStream.close();
	INIStream.clear();

	RegisterSetting(new SME::INI::INISetting(this, "ShowCapturedSoulQuality", "Notifications", "1", "Displays the quality of captured soul"), (CreateINI == false));
	RegisterSetting(new SME::INI::INISetting(this, "ShowEscapedSoulQuality", "Notifications", "1", "Displays the quality of escaped soul"), (CreateINI == false));

	if (CreateINI)		SaveSettingsToINI();
	else				ReadSettingsFromINI();
}

_DefinePatchHdlr(FindBestSoulGemVisitorVisitSizeCheck, 0x004BACD0);
_DefineHookHdlr(DisplaySoulNameOnCapture, 0x007FC187);
_DefineHookHdlr(SentientSoulCheck, 0x004BAC60);
_DefineHookHdlr(DisplaySoulNameOnEscape, 0x007FC21B);

void SmartenSkyrimSouls(void)
{
	_MemHdlr(FindBestSoulGemVisitorVisitSizeCheck).WriteUInt8(0x75);
	_MemHdlr(DisplaySoulNameOnCapture).WriteJump();
	_MemHdlr(SentientSoulCheck).WriteJump();
	_MemHdlr(DisplaySoulNameOnEscape).WriteJump();
}

UInt32 GetActorSoulType(Character* Actor)
{
	bool IsSentient = thisCall<bool>(0x007DF1A0, Actor);
	int Level = thisCall<UInt16>(0x007A5CC0, Actor);

	return cdeclCall<int>(0x00636DF0, Level, IsSentient);
}

const char* __stdcall ModifySoulTrapNotification(UInt8 NotificationType, const char* NotificationMessage, Character* TrappedActor)
{
	static char s_NotificationMessageBuffer[0x200] = {0};

	UInt32 SoulType = GetActorSoulType(TrappedActor);
	const char* SoulName = cdeclCall<const char*>(0x004F4050, SoulType);

	if ((g_INIManager->GetINIInt("ShowCapturedSoulQuality", "Notifications") == 0 && NotificationType == 1) ||
		(g_INIManager->GetINIInt("ShowEscapedSoulQuality", "Notifications") == 0 && NotificationType == 0))
	{
		return NotificationMessage;
	}		

	if (NotificationType == 1)
		sprintf_s(s_NotificationMessageBuffer, sizeof(s_NotificationMessageBuffer), "%s %s", SoulName, NotificationMessage);
	else
		sprintf_s(s_NotificationMessageBuffer, sizeof(s_NotificationMessageBuffer), "%s (%s)", NotificationMessage, SoulName);

	return s_NotificationMessageBuffer;
}

#define _hhName	DisplaySoulNameOnCapture
_hhBegin()
{
	_hhSetVar(Retn, 0x007FC18C);
	_hhSetVar(Call, 0x0040E810);
	__asm
	{
		call	[_hhGetVar(Call)]
		push	[ebp + 0x8]
		push	eax
		push	1			// captured
		call	ModifySoulTrapNotification
		jmp		[_hhGetVar(Retn)]
	}
}

#define _hhName	SentientSoulCheck
_hhBegin()
{
	_hhSetVar(Retn, 0x004BAC87);
	_hhSetVar(Skip, 0x004BAD27);
	_hhSetVar(Sentient, 0x004BAC68);
	_hhSetVar(Call, 0x00935C00);		// TESForm::GetFlag
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

#define _hhName	DisplaySoulNameOnEscape
_hhBegin()
{
	_hhSetVar(Retn, 0x007FC220);
	_hhSetVar(Call, 0x0040E810);
	__asm
	{
		call	[_hhGetVar(Call)]
		push	[ebp + 0x8]
		push	eax
		push	0			// escaped
		call	ModifySoulTrapNotification
		jmp		[_hhGetVar(Retn)]
	}
}