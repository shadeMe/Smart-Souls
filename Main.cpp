#include "SmartSoulsInternals.h"

extern "C"
{
	bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
	{
		_MESSAGE("Smart Souls Initializing...");

		// populate info structure
		info->infoVersion =	PluginInfo::kInfoVersion;
		info->name =		"Smart Souls";
		info->version =		2;

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = skse->GetPluginHandle();

		if(skse->isEditor)
			return false;
		else if(skse->runtimeVersion != RUNTIME_VERSION_1_3_7_0)
		{
			_MESSAGE("Unsupported runtime version %08X", skse->runtimeVersion);
			return false;
		}

		// supported runtime version
		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse)
	{
		SmartenSkyrimSouls();

		_MESSAGE("Souls Smartened Successfully!");
		return true;
	}
};