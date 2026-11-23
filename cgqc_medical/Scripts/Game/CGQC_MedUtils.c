class CGQC_MedUtils
{
	// -----------------------------------------------------------------------
	// Medical consumables
	// -----------------------------------------------------------------------

	// Vanilla / base
	static const ResourceName MED_MORPHINE				= "{0D9A5DCF89AE7AA9}Prefabs/Items/Medicine/MorphineInjection_01/MorphineInjection_01.et";
	static const ResourceName MED_EPINEPHRINE			= "{5B2FD067D70C1E8F}Prefabs/Items/Medicine/EpinephrineInjection/ACE_Medical_EpinephrineInjection.et";
	static const ResourceName MED_FIELDDRESSING			= "{A81F501D3EF6F38E}Prefabs/Items/Medicine/FieldDressing_01/FieldDressing_US_01.et";
	static const ResourceName MED_SALINEBAG				= "{00E36F41CA310E2A}Prefabs/Items/Medicine/SalineBag_01/SalineBag_US_01.et";
	static const ResourceName MED_TOURNIQUET			= "{D70216B1B2889129}Prefabs/Items/Medicine/Tourniquet_01/Tourniquet_US_01.et";

	// MedGEAR tourniquet (different GUID, same path – treat as distinct prefab)
	static const ResourceName MED_TOURNIQUET_MEDGEAR	= "{D104114A2E261A7F}Prefabs/Items/Medicine/Tourniquet_01/Tourniquet_US_01.et";

	// ACE Medical
	static const ResourceName MED_AMMONIUM				= "{58CF3AB87C441295}Prefabs/Items/Medicine/AmmoniumCarbonatePackage/ACE_Medical_AmmoniumCarbonatePackage.et";
	static const ResourceName MED_NALOXONE				= "{02DD34077F51F65E}Prefabs/Items/Medicine/NaloxoneInjection/ACE_Medical_NaloxoneInjection.et";
	static const ResourceName MED_METOPROLOL			= "{D0434D5215B54181}Prefabs/Items/Medicine/MetoprololInjection/ACE_Medical_MetoprololInjection.et";
	static const ResourceName MED_PHENYLEPHRINE			= "{9BBA766CD869002C}Prefabs/Items/Medicine/PhenylephrineInjection/ACE_Medical_PhenylephrineInjection.et";

	// ACE Breathing
	static const ResourceName MED_KINGLT				= "{498EBFCE4936F47B}Prefabs/Items/Medicine/KingLT/ACE_Medical_KingLT.et";
	static const ResourceName MED_OXYGENMASK			= "{FDFA3EAC0D60C65F}Prefabs/Items/Medicine/OxygenMask/ACE_Medical_OxygenMask.et";
	static const ResourceName MED_NCDKIT				= "{661744F5F88B401C}Prefabs/Items/Medicine/NCDKit/ACE_Medical_NCDKit.et";
	static const ResourceName MED_CHESTSEAL				= "{79379D4E43B29B88}Prefabs/Items/Medicine/ChestSeal/ACE_Medical_ChestSeal.et";

	// -----------------------------------------------------------------------
	// Med kit containers (all three are borrowable)
	// -----------------------------------------------------------------------
	static const ResourceName KIT_IFAK					= "{CE262EF537F2E47A}Prefabs/Items/Equipment/Accessories/IFAK/IFAK.et";
	static const ResourceName KIT_IFAK_TAN				= "{25569C2962C8F381}Prefabs/Items/Equipment/Accessories/IFAK/Trauma_IFAK_Tan.et";
	static const ResourceName KIT_MEDICALKIT			= "{AE578EEA4244D41F}Prefabs/Items/Equipment/Kits/MedicalKit_01/MedicalKit_01_US.et";

	// -----------------------------------------------------------------------
	// Helpers
	// -----------------------------------------------------------------------

	static ResourceName GetPrefabName(IEntity item)
	{
		if (!item)
			return "";

		EntityPrefabData pd = item.GetPrefabData();
		if (!pd)
			return "";

		return pd.GetPrefabName();
	}

	//------------------------------------------------------------------------------------------------
	// Returns true for any tracked medical consumable.
	// Used by reconciliation to determine what the provider "spent" from their own kit.
	static bool IsMedItem(ResourceName rn)
	{
		if (rn == MED_MORPHINE)				return true;
		if (rn == MED_EPINEPHRINE)			return true;
		if (rn == MED_FIELDDRESSING)		return true;
		if (rn == MED_SALINEBAG)			return true;
		if (rn == MED_TOURNIQUET)			return true;
		if (rn == MED_TOURNIQUET_MEDGEAR)	return true;
		if (rn == MED_AMMONIUM)				return true;
		if (rn == MED_NALOXONE)				return true;
		if (rn == MED_METOPROLOL)			return true;
		if (rn == MED_PHENYLEPHRINE)		return true;
		if (rn == MED_KINGLT)				return true;
		if (rn == MED_OXYGENMASK)			return true;
		if (rn == MED_NCDKIT)				return true;
		if (rn == MED_CHESTSEAL)			return true;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Returns true if the prefab is one of the three borrowable kit containers.
	static bool IsMedKitContainer(ResourceName rn)
	{
		if (rn == KIT_IFAK)			return true;
		if (rn == KIT_IFAK_TAN)		return true;
		if (rn == KIT_MEDICALKIT)	return true;
		return false;
	}
}