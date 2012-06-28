#pragma once

#include "skse/PluginAPI.h"
#include "skse/skse_version.h"

#include "[Libraries]\SME Sundries\SME_Prefix.h"
#include "[Libraries]\SME Sundries\MemoryHandler.h"
#include "[Libraries]\SME Sundries\INIManager.h"

using namespace SME;
using namespace SME::MemoryHandler;

extern IDebugLog		gLog;
extern PluginHandle		g_pluginHandle;

class ContainerExtraData;
class ContainerExtraEntry;
class TESContainer;
class TESSoulGem;
class Actor;

extern SME::INI::INIManager*						g_INIManager;

class SmartSoulsINIManager : public INI::INIManager
{
public:
	void								Initialize(const char* INIPath, void* Paramenter);
};

// 00
class InventoryChanges
{
public:
	// 04
	class IItemChangeVisitor
	{
	public:
		//     /*00*/ void**			vtbl;
		// no additional members

		// methods
		virtual void*					Dtor(bool ReleaseMemory);
		virtual SInt32					Visit(ContainerExtraEntry* ExtraEntry) = 0;
		virtual bool					VFn08(void* Arg1) { return true;}
		virtual SInt32					VFn0C(void* Arg1, ContainerExtraEntry* Arg2, UInt8* Arg3);		// sets Arg3 to 1 and calls Visit with Arg2
	};
	STATIC_ASSERT(sizeof(IItemChangeVisitor) == 0x4);

	// 0C
	class FindBestSoulGemVisitor : public IItemChangeVisitor
	{
	public:
		// members
		//     /*00*/ void**			vtbl;
		/*04*/ TESContainer*			targetContainer;	// might be a different class, indigenous to Skyrim
		/*08*/ TESSoulGem*				foundSoulGem;		// set to NULL in ctor and when none are found

		// methods
		virtual void*					Dtor(bool ReleaseMemory);
		virtual SInt32					Visit(ContainerExtraEntry* ExtraEntry);
	};
	STATIC_ASSERT(sizeof(FindBestSoulGemVisitor) == 0xC);
};

_DeclareMemHdlr(FindBestSoulGemVisitorVisitSizeCheck, "keeps soul and soul gem capacity levels from mismatching");
_DeclareMemHdlr(DisplaySoulNameOnCapture, "displays the size of the captured soul in the capture notification");
_DeclareMemHdlr(SentientSoulCheck, "fixes a bug that caused non-humanoid souls to get trapped inside black soulgems under certain circumstances");
_DeclareMemHdlr(DisplaySoulNameOnEscape, "displays the size of the escaped soul in the escape notification");

void SmartenSkyrimSouls(void);