#include "SmartSoulsInternals.h"

IDebugLog		gLog("Smart Souls.log");
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

UInt32			g_ActorSoulLevelIncrement = 0;

_DefinePatchHdlr(FindBestSoulGemVisitorVisitSizeCheck, 0x004B9C60);
_DefineHookHdlr(ActorTrapSoul, 0x007F8D60);
_DefineHookHdlr(FindBestSoulGemVisitorVisitActorSoulLevel, 0x004B9BCF);

void SmartenSkyimSouls(void)
{
	_MemHdlr(FindBestSoulGemVisitorVisitSizeCheck).WriteUInt8(0x75);
//	_MemHdlr(ActorTrapSoul).WriteJump();									### bollocks! turns out the vanilla code already does this >:(
//	_MemHdlr(FindBestSoulGemVisitorVisitActorSoulLevel).WriteJump();			I mean, really; fucking hell...
}

#define _hhName	ActorTrapSoul
_hhBegin()
{
	_hhSetVar(Retn, 0x007F8D65);
	__asm
	{
		pop		edx

		pushad
		push	edx
		push	ecx
		call	ActorTrapSoulPerformContainerExtraDataVisit
		popad

		jmp		[_hhGetVar(Retn)]
	}
}

#define _hhName	FindBestSoulGemVisitorVisitActorSoulLevel
_hhBegin()
{
	_hhSetVar(Retn, 0x004B9BD4);
	_hhSetVar(Call, 0x007F8C60);
	__asm
	{
		call	[_hhGetVar(Call)]
		add		eax, g_ActorSoulLevelIncrement
		jmp		[_hhGetVar(Retn)]
	}
}

UInt32 GetActorSoulLevel(Actor* Actor)
{
	return thisCall<UInt32>(0x007F8C60, Actor);
}

void __stdcall ActorTrapSoulPerformContainerExtraDataVisit(ContainerExtraData* ExtraData, InventoryChanges::FindBestSoulGemVisitor* Visitor)
{
	UInt32 TargetActorSoulLevel = GetActorSoulLevel((Actor*)Visitor->targetContainer);

	for (int i = 0; TargetActorSoulLevel + i < 6; i++)
	{
		g_ActorSoulLevelIncrement = i;
		thisCall<void>(0x004B2730, ExtraData, Visitor);		// ContainerExtraData::Visit

		if (Visitor->foundSoulGem != NULL)
			break;
	}

	g_ActorSoulLevelIncrement = 0;
}