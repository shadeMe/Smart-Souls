#include "SmartSoulsInternals.h"

extern "C"
{
	bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
	{
		_MESSAGE("Smart Souls Initializing...");

		// populate info structure
		info->infoVersion =	PluginInfo::kInfoVersion;
		info->name =		"Smart Souls";
		info->version =		5;

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = skse->GetPluginHandle();
		g_INIPath = "Data\\SKSE\\Plugins\\Smart Souls.ini";

		if(skse->isEditor)
			return false;
		else if(skse->runtimeVersion != RUNTIME_VERSION_1_5_26_0)
		{
			_MESSAGE("Unsupported runtime version %08X", skse->runtimeVersion);
			return false;
		}

		// supported runtime version
		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse)
	{
		_MESSAGE("Initializing INI Manager");
		g_INIManager->Initialize(g_INIPath.c_str(), NULL);

		SmartenSkyrimSouls();

		_MESSAGE("Souls Smartened Successfully!");
		return true;
	}
};