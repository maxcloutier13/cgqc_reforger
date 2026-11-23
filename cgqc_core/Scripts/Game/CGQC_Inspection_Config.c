// ============================================================
// CGQC_EquipCheck_Config.c  (cgqc_scripts)
// Data classes for CGQC_EquipCheck .conf file.
// Create the .conf in Workbench:
//   Right-click in Configs/CGQC/ → New → select CGQC_InspectionConfig
//
// Known vanilla prefab ResourceNames for reference:
//   Grenade M67:    {E8F00BF730225B00}Prefabs/Weapons/Grenades/Grenade_M67.et
//   Smoke ANM8HC:   {9DB69176CEF0EE97}Prefabs/Weapons/Grenades/Smoke_ANM8HC.et
//   Radio ANPRC68:  {73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et
//   Radio R148:     {E1A5D4B878AA8980}Prefabs/Items/Equipment/Radios/Radio_R148.et
//   Radio R148 FIA: {540C08AD5F21A5FA}Prefabs/Items/Equipment/Radios/Radio_R148_FIA.et
//   Radio ANPRC152: {C55821E8E86C074E}Prefabs/Items/Equipment/Radios/Radio_ANPRC152.et
// ============================================================

// ─── Single item check entry ─────────────────────────────────────────────────

[BaseContainerProps()]
class CGQC_InspectionItem
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Prefabs acceptés (alternatives — un seul suffit)", "et")]
	ref array<ResourceName> m_aPrefabNames;

	[Attribute("Item", UIWidgets.EditBox, "Nom affiché si absent/insuffisant")]
	string m_sDisplayName;

	[Attribute("1", UIWidgets.EditBox, "Quantité minimum requise (total des alternatives)")]
	int m_iMinCount;
}

// ─── Extra category (for modded / mission-specific additions) ─────────────────

[BaseContainerProps()]
class CGQC_InspectionCategory
{
	[Attribute("Catégorie", UIWidgets.EditBox, "Nom de la catégorie")]
	string m_sCategoryName;

	[Attribute("", UIWidgets.Auto, "Items à vérifier")]
	ref array<ref CGQC_InspectionItem> m_aItems;
}

// ─── Root config ─────────────────────────────────────────────────────────────

[BaseContainerProps(configRoot: true)]
class CGQC_InspectionConfig
{
	// ── Poids ────────────────────────────────────────────────────────────────
	[Attribute("20", UIWidgets.EditBox, "Poids JAUNE (kg) — au-dessus = jaune")]
	float m_fWeightYellow;

	[Attribute("28", UIWidgets.EditBox, "Poids ORANGE (kg) — au-dessus = orange")]
	float m_fWeightOrange;

	[Attribute("35", UIWidgets.EditBox, "Poids ROUGE (kg) — au-dessus = rouge")]
	float m_fWeightRed;

	[Attribute("45", UIWidgets.EditBox, "Poids NOIR (kg) — au-dessus = noir (surcharge)")]
	float m_fWeightBlack;

	// ── Armes: chargeurs minimum par emplacement ──────────────────────────────
	[Attribute("4", UIWidgets.EditBox, "Chargeurs minimum — arme primaire")]
	int m_iPrimaryMagMin;

	[Attribute("0", UIWidgets.EditBox, "Chargeurs minimum — arme secondaire")]
	int m_iSecondaryMagMin;

	[Attribute("2", UIWidgets.EditBox, "Chargeurs minimum — pistolet")]
	int m_iHandgunMagMin;

	// ── PROTECTION: items configurables (yeux, oreilles...) ──────────────────
	// Helmet et chambre sont hardcodés — ajouter ici protection oculaire/auditive.
	[Attribute("", UIWidgets.Auto, "Items PROTECTION")]
	ref array<ref CGQC_InspectionItem> m_aProtectionItems;

	// ── MÉDICAL ───────────────────────────────────────────────────────────────
	[Attribute("", UIWidgets.Auto, "Items MÉDICAL")]
	ref array<ref CGQC_InspectionItem> m_aMedicalItems;

	// ── COMBAT: items configurables (grenades, fumigènes, couteau...) ─────────
	// Chargeurs primaires sont hardcodés — ajouter ici le reste.
	[Attribute("", UIWidgets.Auto, "Items COMBAT")]
	ref array<ref CGQC_InspectionItem> m_aCombatItems;

	// ── ADMIN: items configurables (carte, boussole, montre...) ──────────────
	// Radio est hardcodée — ajouter ici carte, boussole, montre.
	[Attribute("", UIWidgets.Auto, "Items ADMIN")]
	ref array<ref CGQC_InspectionItem> m_aAdminItems;

	// ── Catégories supplémentaires (Capacités Spéciales etc.) ─────────────────
	[Attribute("", UIWidgets.Auto, "Catégories supplémentaires")]
	ref array<ref CGQC_InspectionCategory> m_aExtraCategories;
}