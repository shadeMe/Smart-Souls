#pragma warning(disable : 4005)
#include "SmartSoulsInternals.h"
#include <algorithm>

IDebugLog					gLog("Smart Souls.log");
PluginHandle				g_pluginHandle = kPluginHandle_Invalid;

SmartSoulsINIManager		SmartSoulsINIManager::Instance;
const char*					SmartSoulsINIManager::AzuraImposterSectionName = "AzurasStarImposters";

SME::INI::INISetting		kForceSoulSoulGemSizeMatch("ForceSoulSoulGemSizeMatch",
												"General",
												"Soul and soul gem sizes should match for a successful capture",
												(SInt32)1);

SME::INI::INISetting		kLimitBlackGemsToBlackSouls("LimitBlackGemsToBlackSouls",
												   "General", 
												   "Prevents non-NPC souls from being held in black soulgems",
												   (SInt32)1);

SME::INI::INISetting		kShowCapturedSoulQuality("ShowCapturedSoulQuality",
												   "Notifications", 
												   "Displays the quality of captured souls",
												   (SInt32)1);

SME::INI::INISetting		kShowEscapedSoulQuality("ShowEscapedSoulQuality",
												   "Notifications", 
												   "Displays the quality of escaped souls",
												   (SInt32)1);

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

	RegisterSetting(&kForceSoulSoulGemSizeMatch);
	RegisterSetting(&kLimitBlackGemsToBlackSouls);
	RegisterSetting(&kShowCapturedSoulQuality);
	RegisterSetting(&kShowEscapedSoulQuality);

	if (CreateINI)
		Save();

	PopulateFromSection(AzuraImposterSectionName);
}

_DefineHookHdlr(FindBestSoulGemVisitorVisitSizeCheck, 0x004767A8);
_DefineHookHdlr(SentientSoulCheck, 0x00476763);
_DefineHookHdlr(DisplaySoulNameOnCapture, 0x006ECABC);
_DefineHookHdlr(DisplaySoulNameOnEscape, 0x006ECB29);

void SmartenSkyrimSouls(void)
{
	_MemHdlr(FindBestSoulGemVisitorVisitSizeCheck).WriteJump();
	_MemHdlr(SentientSoulCheck).WriteJump();
	_MemHdlr(DisplaySoulNameOnCapture).WriteJump();
	_MemHdlr(DisplaySoulNameOnEscape).WriteJump();
}

bool __stdcall PerformSoulGemSizeCheck(TESSoulGem* Soulgem)
{
	static UInt32 kAzurasStarFormID = 0x00063B27;
	static std::list<UInt32> kImposters;

	if (kImposters.size() == 0)
	{
		for (SME::INI::INIManagerIterator Itr(&SmartSoulsINIManager::Instance, SmartSoulsINIManager::AzuraImposterSectionName);
										Itr.GetDone() == false;
										Itr.GetNextSetting())
		{
			UInt32 FormID = 0;
			SME_ASSERT(Itr()->GetType() == SME::INI::INISetting::kType_String);

			sscanf_s(Itr()->GetData().s, "%08X", &FormID);
#ifndef NDEBUG
			_MESSAGE("Imposter = %s", Itr()->GetData().s);
#endif

			if (FormID)
			{
				FormID = FormID & 0x00FFFFFF;		// sloblock to the mod index bit
				kImposters.push_back(FormID);
			}
		}
	}

	SME_ASSERT(Soulgem);

	if (kForceSoulSoulGemSizeMatch.GetData().i == 0 ||
		Soulgem->formID == kAzurasStarFormID ||
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
	_hhSetVar(Retn, 0x004767AE);
	_hhSetVar(Skip, 0x004767E4);
	__asm
	{
		mov		eax, [esp + 0x10]
		pushad

		push	eax
		call	PerformSoulGemSizeCheck
		test	al, al
		jnz		SIZEOK

		popad
		cmp		esi, [esp + 0x14]			// skip if soul != soul gem
		jnz		SKIP
		jmp		[_hhGetVar(Retn)]
	SKIP:
		jmp		[_hhGetVar(Skip)]
	SIZEOK:
		popad
		cmp		esi, [esp + 0x14]			// skip if soul > soul gem
		jb		SKIP
		jmp		[_hhGetVar(Retn)]
	}
}

bool __stdcall PerformBlackSoulGemCheck(TESSoulGem* Soulgem)
{
	SME_ASSERT(Soulgem);

	if (kLimitBlackGemsToBlackSouls.GetData().i && (Soulgem->flags & 0x20000))
		return true;
	else
		return false;
}

#define _hhName	SentientSoulCheck
_hhBegin()
{
	_hhSetVar(Retn, 0x00476777);
	_hhSetVar(Skip, 0x004767E4);
	_hhSetVar(Sentient, 0x0047676A);
	__asm
	{
		cmp     byte ptr [esp + 0x1C], 0
		jnz		SENTIENT

		mov		ecx, [edi]
		pushad
		push	ecx							// non-sentient, so check if black soul gem
		call	PerformBlackSoulGemCheck
		test    al, al
		jz		NEXT

		jmp		SKIP
	SENTIENT:
		jmp		[_hhGetVar(Sentient)]
	SKIP:
		popad
		jmp		[_hhGetVar(Skip)]
	NEXT:
		popad
		jmp		[_hhGetVar(Retn)]
	}
}

UInt32 GetActorSoulType(Actor* Actor)
{
	bool IsSentient = thisCall<bool>(0x006AA140, Actor);
	int Level = thisCall<UInt16>(0x006A7320, Actor);

	return cdeclCall<int>(0x0059C090, Level, IsSentient);
}

const char* GetSoulName(UInt32 SoulType)
{
	return cdeclCall<const char*>(0x004AB8D0, SoulType);
}

const char* __stdcall ModifySoulTrapNotification(UInt8 NotificationType, const char* NotificationMessage, Actor* TrappedActor)
{
	static char s_NotificationMessageBuffer[0x200] = {0};

	UInt32 SoulType = GetActorSoulType(TrappedActor);
	const char* SoulName = GetSoulName(SoulType);

	if ((kShowCapturedSoulQuality.GetData().i == 0 && NotificationType == 1) ||
		(kShowEscapedSoulQuality.GetData().i == 0 && NotificationType == 0))
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
	_hhSetVar(Retn, 0x006ECAC1);
	_hhSetVar(Call, 0x008997A0);
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
	_hhSetVar(Retn, 0x006ECB2E);
	_hhSetVar(Call, 0x008997A0);
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