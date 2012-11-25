#pragma warning(disable : 4005)
#include "SmartSoulsInternals.h"
#include <algorithm>

IDebugLog					gLog("Smart Souls.log");
PluginHandle				g_pluginHandle = kPluginHandle_Invalid;

INI::INIManager*			g_INIManager = new SmartSoulsINIManager();
const char*					SmartSoulsINIManager::AzuraImposterSectionName = "AzurasStarImposters";

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

	PopulateFromSection(AzuraImposterSectionName);
}

_DefineHookHdlr(FindBestSoulGemVisitorVisitSizeCheck, 0x004765A8);
_DefineHookHdlr(SentientSoulCheck, 0x00476563);
_DefineHookHdlr(DisplaySoulNameOnCapture, 0x006EC58C);
_DefineHookHdlr(DisplaySoulNameOnEscape, 0x006EC5F9);

void SmartenSkyrimSouls(void)
{
	_MemHdlr(FindBestSoulGemVisitorVisitSizeCheck).WriteJump();
	_MemHdlr(SentientSoulCheck).WriteJump();
	_MemHdlr(DisplaySoulNameOnCapture).WriteJump();
	_MemHdlr(DisplaySoulNameOnEscape).WriteJump();
}

bool __stdcall PerformSoulGemAzurasStarCheck(TESSoulGem* Soulgem)
{
	static UInt32 kAzurasStarFormID = 0x00063B27;
	static std::list<UInt32> kImposters;
	if (kImposters.size() == 0)
	{
		for (SME::INI::INIManagerIterator Itr(g_INIManager, SmartSoulsINIManager::AzuraImposterSectionName); Itr.GetDone() == false; Itr.GetNextSetting())
		{
			UInt32 FormID = Itr()->GetValueAsUnsignedInteger(true);

			if (FormID)
			{
				FormID = FormID & 0x00FFFFFF;		// sloblock to the mod index bit
				kImposters.push_back(FormID);
			}
		}
	}

	SME_ASSERT(Soulgem);

	if (Soulgem->formID == kAzurasStarFormID ||
		std::find(kImposters.begin(), kImposters.end(), Soulgem->formID & 0x00FFFFFF) != kImposters.end())
	{
		return true;
	}
	else
		return false;
}

#define _hhName	FindBestSoulGemVisitorVisitSizeCheck
_hhBegin()
{
	_hhSetVar(Retn, 0x004765AE);
	_hhSetVar(Skip, 0x004765E4);
	__asm
	{
		mov		eax, [esp + 0x10]
		pushad

		push	eax
		call	PerformSoulGemAzurasStarCheck
		test	al, al
		jnz		AZURASSTAR

		popad
		cmp		esi, [esp + 0x14]			// check soulgem capacity
		jnz		SKIP
		jmp		[_hhGetVar(Retn)]
	SKIP:
		jmp		[_hhGetVar(Skip)]
	AZURASSTAR:
		popad
		cmp		esi, [esp + 0x14]
		jb		SKIP
		jmp		[_hhGetVar(Retn)]
	}
}

#define _hhName	SentientSoulCheck
_hhBegin()
{
	_hhSetVar(Retn, 0x00476577);
	_hhSetVar(Skip, 0x004765E4);
	_hhSetVar(Sentient, 0x0047656A);
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
	bool IsSentient = thisCall<bool>(0x006A9D30, Actor);
	int Level = thisCall<UInt16>(0x006A6EC0, Actor);

	return cdeclCall<int>(0x0059B900, Level, IsSentient);
}

const char* GetSoulName(UInt32 SoulType)
{
	return cdeclCall<const char*>(0x004AAF10, SoulType);
}

const char* __stdcall ModifySoulTrapNotification(UInt8 NotificationType, const char* NotificationMessage, Actor* TrappedActor)
{
	static char s_NotificationMessageBuffer[0x200] = {0};

	static const bool kShowCapturedSoulQuality = g_INIManager->GetINIInt("ShowCapturedSoulQuality", "Notifications");
	static const bool kShowEscapedSoulQuality = g_INIManager->GetINIInt("ShowEscapedSoulQuality", "Notifications");

	UInt32 SoulType = GetActorSoulType(TrappedActor);
	const char* SoulName = GetSoulName(SoulType);

	if ((kShowCapturedSoulQuality == false && NotificationType == 1) ||
		(kShowEscapedSoulQuality == false && NotificationType == 0))
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
	_hhSetVar(Retn, 0x006EC591);
	_hhSetVar(Call, 0x00898E60);
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
	_hhSetVar(Retn, 0x006EC5FE);
	_hhSetVar(Call, 0x00898E60);
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