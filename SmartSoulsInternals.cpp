#include "SmartSoulsInternals.h"

IDebugLog					gLog("Smart Souls.log");
PluginHandle				g_pluginHandle = kPluginHandle_Invalid;
INI::INIManager*			g_INIManager = new SmartSoulsINIManager();
std::string					g_INIPath;

void SmartSoulsINIManager::Initialize(const char* INIPath, void* Paramenter)
{
	this->INIFilePath = INIPath;
	_MESSAGE("INI Path: %s", INIPath);

	std::fstream INIStream(INIPath, std::fstream::in);
	bool CreateINI = false;

	if (INIStream.fail())
	{
		_MESSAGE("INI File not found; Creating one...");
		CreateINI = true;
	}

	INIStream.close();
	INIStream.clear();

	RegisterSetting("ShowCapturedSoulQuality", "Notifications", "1", "Displays the quality of captured soul");
	RegisterSetting("ShowEscapedSoulQuality", "Notifications", "1", "Displays the quality of escaped soul");

	if (CreateINI)
		Save();
	else
		Load();
}

_DefineHookHdlr(FindBestSoulGemVisitorVisitSizeCheck, 0x00472FF8);
_DefineHookHdlr(SentientSoulCheck, 0x00472FB3);
_DefineHookHdlr(DisplaySoulNameOnCapture, 0x006E41AC);
_DefineHookHdlr(DisplaySoulNameOnEscape, 0x006E4219);

void SmartenSkyrimSouls(void)
{
	_MemHdlr(FindBestSoulGemVisitorVisitSizeCheck).WriteJump();
	_MemHdlr(SentientSoulCheck).WriteJump();
	_MemHdlr(DisplaySoulNameOnCapture).WriteJump();
	_MemHdlr(DisplaySoulNameOnEscape).WriteJump();
}

#pragma warning(disable : 4005)

#define _hhName	FindBestSoulGemVisitorVisitSizeCheck
_hhBegin()
{
	_hhSetVar(Retn, 0x00472FFE);
	_hhSetVar(Skip, 0x00473034);
	__asm
	{
		mov		eax, [esp + 0x10]
		mov     eax, [eax + 0xC]
		cmp		eax, 0x00063B27				// perform formID check for Azura's Star, in which case allow lesser soul storage
		jz		AZURASSTAR

		cmp		esi, [esp + 0x14]			// check soulgem capacity
		jnz		SKIP
		jmp		[_hhGetVar(Retn)]
	SKIP:
		jmp		[_hhGetVar(Skip)]
	AZURASSTAR:
		cmp		esi, [esp + 0x14]
		jb		SKIP
		jmp		[_hhGetVar(Retn)]
	}
}

#define _hhName	SentientSoulCheck
_hhBegin()
{
	_hhSetVar(Retn, 0x00472FC7);
	_hhSetVar(Skip, 0x00473034);
	_hhSetVar(Sentient, 0x00472FBA);
	__asm
	{
		cmp     byte ptr [esp + 0x1C], 0
		jnz		SENTIENT

		mov		ecx, [edi]					// if non-sentient, make sure we aren't using a black soul gem
		mov     edx, [ecx + 0x8]
		and		edx, 0x20000
		setnz	al
		test    al, al
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

UInt32 GetActorSoulType(Actor* Actor)
{
	bool IsSentient = thisCall<bool>(0x006A2A90, Actor);
	int Level = thisCall<UInt16>(0x0069FC40, Actor);

	return cdeclCall<int>(0x00595BE0, Level, IsSentient);
}

const char* __stdcall ModifySoulTrapNotification(UInt8 NotificationType, const char* NotificationMessage, Actor* TrappedActor)
{
	static char s_NotificationMessageBuffer[0x200] = {0};

	UInt32 SoulType = GetActorSoulType(TrappedActor);
	const char* SoulName = cdeclCall<const char*>(0x004A6F80, SoulType);

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
	_hhSetVar(Retn, 0x006E41B1);
	_hhSetVar(Call, 0x00885EB0);
	__asm
	{
		pop		edx
		push	ebp
		push	edx
		push	1			// captured
		call	ModifySoulTrapNotification
		push	eax
		call	[_hhGetVar(Call)]
		jmp		[_hhGetVar(Retn)]
	}
}

#define _hhName	DisplaySoulNameOnEscape
_hhBegin()
{
	_hhSetVar(Retn, 0x006E421E);
	_hhSetVar(Call, 0x00885EB0);
	__asm
	{
		pop		ecx
		push	ebp
		push	ecx
		push	0			// escaped
		call	ModifySoulTrapNotification
		push	eax
		call	[_hhGetVar(Call)]
		jmp		[_hhGetVar(Retn)]
	}
}