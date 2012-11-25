#include "SmartSoulsInternals.h"
#include "VersionInfo.h"

extern "C"
{
	bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
	{
		_MESSAGE("Smart Souls Initializing...");

		info->infoVersion =	PluginInfo::kInfoVersion;
		info->name =		"Smart Souls";
		info->version =		PACKED_SME_VERSION;

		g_pluginHandle = skse->GetPluginHandle();

		if(skse->isEditor)
			return false;
		else if(skse->runtimeVersion != RUNTIME_VERSION_1_8_151_0)
		{
			_MESSAGE("Unsupported runtime version %08X", skse->runtimeVersion);
			return false;
		}

		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse)
	{
		_MESSAGE("Initializing INI Manager");
		g_INIManager->Initialize("Data\\SKSE\\Plugins\\Smart Souls.ini", NULL);

		SmartenSkyrimSouls();

		_MESSAGE("Souls Smartened Successfully!");
		return true;
	}
};