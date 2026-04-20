/**
 * CGQC_EntityCatalogGenerator.c
 *
 * Workbench ResourceManagerPlugin.
 * Select .et prefabs in the Resource Browser, right-click → Plugins → CGQC → Generate Catalog Entries.
 * The plugin classifies each prefab by probing its components, then prints ready-to-paste
 * SCR_EntityCatalogEntry blocks to the Workbench log console AND copies them to the clipboard.
 *
 * INSTALL:
 *   Drop this file into:   <your_mod>/scripts/WorkbenchGame/
 *   Reload WB scripts:     Plugins → Settings → Reload WB Scripts  (Ctrl+Shift+R)
 *
 * USAGE:
 *   1. Configure the target catalog type once via Plugins → Settings → CGQC → Generate Catalog Entries.
 *      (Optional — the plugin auto-detects per prefab if left on AUTO.)
 *   2. Select one or more .et files in the Resource Browser.
 *   3. Right-click → Plugins → CGQC → Generate Catalog Entries.
 *   4. Copy the output from the log (it is also sent to clipboard automatically).
 *   5. Open your target .conf in a text editor and paste inside the EntityList array.
 *
 * CLASSIFICATION LOGIC  (component probe, no spawning):
 *   CharacterControllerComponent  → CHARACTER
 *   VehicleControllerComponent    → VEHICLE
 *   WeaponComponent               → ITEM  (weapons are inventory items in the catalog)
 *   SCR_AIGroup  root class       → GROUP
 *   Anything else with
 *     InventoryItemComponent      → ITEM
 *   Fallback                      → ITEM  (with a warning)
 *
 * OUTPUT FORMAT per entry:
 *   {
 *       item SCR_EntityCatalogEntry
 *       {
 *           m_Prefab="{GUID}Path/To/Prefab.et"
 *       }
 *   }
 *
 * NOTE ON WRITING TO THE CONF:
 *   The Workbench plugin API cannot programmatically insert items into an existing
 *   conf array through BaseContainerTools — the container must already have the
 *   list field pre-allocated. Output-to-clipboard is therefore the correct pattern
 *   (same approach as BI's own sample plugins).
 */

//------------------------------------------------------------------------------------------------
[WorkbenchPluginAttribute(
	name: "Generate Catalog Entries",
	description: "Classifies selected .et prefabs and outputs SCR_EntityCatalogEntry blocks ready to paste into a catalog .conf.\nRight-click .et files → Plugins → CGQC → Generate Catalog Entries.",
	category: "CGQC",
	wbModules: { "ResourceManager" },
	resourceTypes: { "et" },
	awesomeFontCode: 0xF0CB   // fa-list-ul
)]
class CGQC_EntityCatalogGeneratorPlugin : ResourceManagerPlugin
{
	// ── Settings (editable via Plugins → Settings) ──────────────────────────────
	// Leave OVERRIDE_TYPE empty ("") to auto-detect per prefab.
	// Set to one of: CHARACTER / VEHICLE / ITEM / GROUP  to force all selected prefabs.
	[Attribute("", UIWidgets.EditBox,
		"Force catalog type for ALL selected prefabs.\nLeave empty for auto-detection per prefab.\nValid values: CHARACTER  VEHICLE  ITEM  GROUP")]
	protected string m_sOverrideType;

	// Entry class to emit. Usually SCR_EntityCatalogEntry.
	// Change to a subclass if your catalog uses one.
	[Attribute("SCR_EntityCatalogEntry", UIWidgets.EditBox,
		"Entry class name written into the output block.\nDefault: SCR_EntityCatalogEntry")]
	protected string m_sEntryClassName;

	// ── Public API ───────────────────────────────────────────────────────────────

	//------------------------------------------------------------------------------------------------
	override void Configure()
	{
		// Returning without empty body makes the Settings entry appear in the menu.
	}

	//------------------------------------------------------------------------------------------------
	override void OnResourceContextMenu(notnull array<ResourceName> resources)
	{
		if (resources.IsEmpty())
		{
			Print("[CGQC CatalogGen] No resources selected.", LogLevel.WARNING);
			return;
		}

		string output = "";
		int processed = 0;
		int skipped   = 0;

		foreach (ResourceName rn : resources)
		{
			if (!rn.EndsWith(".et"))
			{
				PrintFormat("[CGQC CatalogGen] Skipping non-.et resource: %1", rn);
				skipped++;
				continue;
			}

			string entryBlock = BuildEntryBlock(rn);
			if (entryBlock.IsEmpty())
			{
				skipped++;
				continue;
			}

			output += entryBlock + "\n";
			processed++;
		}

		if (output.IsEmpty())
		{
			Print("[CGQC CatalogGen] Nothing to output — all resources skipped.", LogLevel.WARNING);
			return;
		}

		string header = string.Format(
			"// ── CGQC Catalog Entries (%1 prefabs) ─────────────────────────────────────\n"
			+ "// Paste these blocks inside the EntityList array of your target .conf\n",
			processed);

		string full = header + output;

		// Print to log (always visible in Workbench console)
		Print("─────────────────────────────────────────────────────────────────────────");
		Print(full);
		Print("─────────────────────────────────────────────────────────────────────────");
		PrintFormat("[CGQC CatalogGen] Done: %1 entries generated, %2 skipped.", processed, skipped);

		// Copy to clipboard so it can be pasted directly
		System.ExportToClipboard(full);
		Print("[CGQC CatalogGen] Output copied to clipboard.");
	}

	// ── Private helpers ──────────────────────────────────────────────────────────

	//------------------------------------------------------------------------------------------------
	//! Classify the prefab, build and return the formatted entry block.
	protected string BuildEntryBlock(ResourceName rn)
	{
		Resource res = Resource.Load(rn);
		if (!res || !res.IsValid())
		{
			PrintFormat("[CGQC CatalogGen] WARNING: Could not load resource: %1", rn);
			return string.Empty;
		}

		// ── Determine catalog type ───────────────────────────────────────────────
		string catalogType;

		if (!m_sOverrideType.IsEmpty())
		{
			catalogType = m_sOverrideType;
		}
		else
		{
			catalogType = ClassifyPrefab(res, rn);
		}

		// ── Emit entry block ─────────────────────────────────────────────────────
		// Format matches what Workbench writes when you manually add an entry:
		//
		//   {
		//       item SCR_EntityCatalogEntry
		//       {
		//           m_Prefab="{GUID}path.et"
		//       }
		//   }
		//
		// m_Prefab must be the full ResourceName string (includes GUID if the file
		// is registered, or just the path if not yet registered).

		string prefabStr = rn;   // ResourceName implicitly converts to string

		string block =
			"{\n"
			+ string.Format("\titem %1\n", m_sEntryClassName)
			+ "\t{\n"
			+ string.Format("\t\tm_Prefab=\"%1\"\n", prefabStr)
			+ "\t\t// CatalogType hint: " + catalogType + "\n"
			+ "\t}\n"
			+ "}";

		return block;
	}

	//------------------------------------------------------------------------------------------------
	//! Probe prefab components to determine the most appropriate catalog type.
	protected string ClassifyPrefab(Resource res, ResourceName rn)
	{
		// Priority order: CHARACTER > VEHICLE > GROUP > ITEM

		// CharacterControllerComponent → CHARACTER
		if (SCR_BaseContainerTools.FindComponentSource(res, CharacterControllerComponent))
			return "CHARACTER";

		// VehicleControllerComponent → VEHICLE
		if (SCR_BaseContainerTools.FindComponentSource(res, VehicleControllerComponent))
			return "VEHICLE";

		// SCR_AIGroup: groups don't have a single unique component we can reliably
		// probe this way — check root entity class name via the container.
		// We fall back to checking the resource name for common group naming patterns.
		string rnStr = rn;
		rnStr.ToLower();
		if (rnStr.Contains("group_") || rnStr.Contains("_group") || rnStr.Contains("_squad") || rnStr.Contains("_fireteam"))
			return "GROUP";

		// WeaponComponent → ITEM (weapons live in the ITEM catalog)
		if (SCR_BaseContainerTools.FindComponentSource(res, WeaponComponent))
			return "ITEM";

		// InventoryItemComponent → ITEM (gear, attachments, etc.)
		if (SCR_BaseContainerTools.FindComponentSource(res, InventoryItemComponent))
			return "ITEM";

		// Fallback — emit as ITEM with a warning
		PrintFormat("[CGQC CatalogGen] WARNING: Could not classify '%1' — defaulting to ITEM. Check manually.", rn);
		return "ITEM";
	}
}