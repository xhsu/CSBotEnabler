import eiface;

import engine_api;

import Plugin;

import UtlHook;


extern bool __cdecl UTIL_IsGame(const char* String1) noexcept;
inline constexpr unsigned char SIG[] = "\xCC\x55\x8B\xEC\x81\xEC\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x56\x8B\x75\x08\x8D\x85";
inline FunctionHook HOOKFN_UTIL_IsGame{ &UTIL_IsGame };

bool __cdecl UTIL_IsGame(const char* String1) noexcept
{
	if (!strcmp(String1, "czero"))
		return true;

	return HOOKFN_UTIL_IsGame(String1);
}

// The hook here must be extremely early.
// UTIL_IsGame() is used early.
static void DeployHook() noexcept
{
	static bool bHooked = false;

	[[likely]]
	if (bHooked)
		return;

	HOOKFN_UTIL_IsGame.ApplyOn(
		(decltype(&UTIL_IsGame))UTIL_SearchPattern("mp.dll", 1, SIG)
	);

	bHooked = true;
}

// From SDK dlls/h_export.cpp:

//! Holds engine functionality callbacks
inline enginefuncs_t g_engfuncs = {};
inline globalvars_t *gpGlobals = nullptr;

// Receive engine function table from engine.
// This appears to be the _first_ DLL routine called by the engine, so we do some setup operations here.
void __stdcall GiveFnptrsToDll(enginefuncs_t *pengfuncsFromEngine, globalvars_t *pGlobals) noexcept
{
	memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
	gpGlobals = pGlobals;

	DeployHook();
}

// Must provide at least one of these..
inline constexpr META_FUNCTIONS gMetaFunctionTable =
{
	.pfnGetEntityAPI			= nullptr,	// HL SDK; called before game DLL
	.pfnGetEntityAPI_Post		= nullptr,	// META; called after game DLL
	.pfnGetEntityAPI2			= nullptr,	// HL SDK2; called before game DLL
	.pfnGetEntityAPI2_Post		= nullptr,	// META; called after game DLL
	.pfnGetNewDLLFunctions		= nullptr,	// HL SDK2; called before game DLL
	.pfnGetNewDLLFunctions_Post	= nullptr,	// META; called after game DLL
	.pfnGetEngineFunctions		= nullptr,	// META; called before HL engine
	.pfnGetEngineFunctions_Post	= nullptr,	// META; called after HL engine
};

// Metamod requesting info about this plugin:
//  ifvers			(given) interface_version metamod is using
//  pPlugInfo		(requested) struct with info about plugin
//  pMetaUtilFuncs	(given) table of utility functions provided by metamod
int Meta_Query(const char *pszInterfaceVersion, plugin_info_t const **pPlugInfo, mutil_funcs_t *pMetaUtilFuncs) noexcept
{
	*pPlugInfo = PLID;
	gpMetaUtilFuncs = pMetaUtilFuncs;

	return true;
}
static_assert(std::same_as<decltype(&Meta_Query), META_QUERY_FN>);

// Metamod attaching plugin to the server.
//  now				(given) current phase, ie during map, during changelevel, or at startup
//  pFunctionTable	(requested) table of function tables this plugin catches
//  pMGlobals		(given) global vars from metamod
//  pGamedllFuncs	(given) copy of function tables from game dll
int Meta_Attach(PLUG_LOADTIME iCurrentPhase, META_FUNCTIONS *pFunctionTable, meta_globals_t *pMGlobals, gamedll_funcs_t *pGamedllFuncs) noexcept
{
	if (!pMGlobals) [[unlikely]]
	{
		gpMetaUtilFuncs->pfnLogError(PLID, "Function 'Meta_Attach' called with null 'pMGlobals' parameter.");
		return false;
	}

	gpMetaGlobals = pMGlobals;

	if (!pFunctionTable) [[unlikely]]
	{
		gpMetaUtilFuncs->pfnLogError(PLID, "Function 'Meta_Attach' called with null 'pFunctionTable' parameter.");
		return false;
	}

	memcpy(pFunctionTable, &gMetaFunctionTable, sizeof(META_FUNCTIONS));
	gpGamedllFuncs = pGamedllFuncs;
	return true;
}
static_assert(std::same_as<decltype(&Meta_Attach), META_ATTACH_FN>);

// Metamod detaching plugin from the server.
// now		(given) current phase, ie during map, etc
// reason	(given) why detaching (refresh, console unload, forced unload, etc)
int Meta_Detach(PLUG_LOADTIME iCurrentPhase, PL_UNLOAD_REASON iReason) noexcept
{
	return true;
}
static_assert(std::same_as<decltype(&Meta_Detach), META_DETACH_FN>);
