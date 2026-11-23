//! Registers the example faction via RegisterRaw (required for Workshop).
//! Configs/Factions/EXAMPLE.json is the editable reference — keep SF_ExampleFactionJson.c in sync.
//! FileIO RegisterDirectory works in Workbench only; published packs omit JSON file bodies.
modded class SF_FactionGroups
{
	override static void Init()
	{
		SF_FactionConfigLoader.RegisterRaw("PCExampleFaction/EXAMPLE.json", SF_ExampleFactionJson.Get());
		super.Init();
	}
}
