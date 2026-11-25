//------------------------------------------------------------------------------------------------
//!
//! \Author: 		silent1
//! \Last Updated: 	2026-11-25
//! \Source: 		Coalition Squad Interface + CGQC
//!	\Description: 	Hides the compass's N/S/E/W letters while the locally controlled character carries no
//! 				compass or navigation item. The letters are part of the compass image, so a circular mask
//! 				crops them off and leaves the ring. Client only, nothing is replicated.
//!
//------------------------------------------------------------------------------------------------
modded class CSI_Compass
{
	//! Alpha 0.2 inside the ring's outer edge, 0.85 outside it, where the letters are
	protected static const ResourceName CGQC_LETTER_MASK = "{2F6E6A52FC1F97E0}UI/Textures/HUD/CGQC_CompassMask.edds";

	// Mask pixels with alpha below the progress show, fading out over the transition width above it
	protected static const float CGQC_MASK_PROGRESS_NO_LETTERS = 0.35;
	protected static const float CGQC_MASK_PROGRESS_ALL = 1;
	protected static const float CGQC_MASK_TRANSITION_WIDTH = 0.3;

	protected static const int CGQC_CHECK_INTERVAL = 200;	//!< ms between inventory checks

	//! Items that count besides compass and GPS gadgets; prefabs inheriting from these count too
	protected static const ref array<string> CGQC_NAVIGATION_PREFABS = {
		"Prefabs/Items/Equipment/Watches/Watch_Garmin",		// every RHS Garmin
		"Prefabs/Accessories/GRS_Garmin.et",
		"Prefabs/Accessories/GRS_Atak_Phone.et"
	};

	protected bool m_bCGQC_MaskLoaded;
	protected int m_iCGQC_LastCheck;							//!< tick of the last inventory check
	protected SCR_ChimeraCharacter m_CGQC_CheckedCharacter;		//!< character of the last inventory check
	protected ref array<IEntity> m_aCGQC_Items = {};			//!< reused each check

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		if (!m_wCompass)
			return;

		m_bCGQC_MaskLoaded = m_wCompass.LoadMaskTexture(CGQC_LETTER_MASK);
		if (!m_bCGQC_MaskLoaded)
		{
			Print("CSI_Compass: CGQC letter mask not loaded, the letters will always show", LogLevel.WARNING);
			return;
		}

		// Everything shows until Update says otherwise, as in the settings tab's HUD example
		m_wCompass.SetMaskMode(ImageMaskMode.REGULAR);
		m_wCompass.SetMaskTransitionWidth(CGQC_MASK_TRANSITION_WIDTH);
		m_wCompass.SetMaskProgress(CGQC_MASK_PROGRESS_ALL);
	}

	//------------------------------------------------------------------------------------------------
	//! Called every frame by CSI_HUD.
	override void Update()
	{
		super.Update();

		if (!m_bCGQC_MaskLoaded || !m_wCompass || !m_HUDSystem)
			return;

		// A new character (respawn, possession) is checked right away
		SCR_ChimeraCharacter character = m_HUDSystem.GetLocalPlayerCharacter();
		int now = System.GetTickCount();
		if (character == m_CGQC_CheckedCharacter && now - m_iCGQC_LastCheck < CGQC_CHECK_INTERVAL)
			return;

		m_CGQC_CheckedCharacter = character;
		m_iCGQC_LastCheck = now;

		float maskProgress = CGQC_MASK_PROGRESS_NO_LETTERS;
		if (CGQC_HasNavigationItem(character))
			maskProgress = CGQC_MASK_PROGRESS_ALL;

		m_wCompass.SetMaskProgress(maskProgress);
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] character
	//! \return true if the living character has a navigation item in hand or anywhere in their inventory
	protected bool CGQC_HasNavigationItem(SCR_ChimeraCharacter character)
	{
		if (!character)
			return false;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller || controller.GetLifeState() == ECharacterLifeState.DEAD)
			return false;

		SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.GetGadgetManager(character);
		if (gadgetManager && CGQC_IsNavigationItem(gadgetManager.GetHeldGadget()))
			return true;

		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		if (!inventory)
			return false;

		m_aCGQC_Items.Clear();
		inventory.GetItems(m_aCGQC_Items);

		foreach (IEntity item : m_aCGQC_Items)
		{
			if (CGQC_IsNavigationItem(item))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] item
	//! \return true for a compass or GPS gadget, or one of CGQC_NAVIGATION_PREFABS
	protected bool CGQC_IsNavigationItem(IEntity item)
	{
		if (!item)
			return false;

		SCR_GadgetComponent gadget = SCR_GadgetComponent.Cast(item.FindComponent(SCR_GadgetComponent));
		if (gadget && (gadget.GetType() == EGadgetType.COMPASS || gadget.GetType() == EGadgetType.GPS))
			return true;

		EntityPrefabData prefabData = item.GetPrefabData();
		if (!prefabData)
			return false;

		BaseContainer prefab = prefabData.GetPrefab();
		while (prefab)
		{
			string prefabName = prefab.GetResourceName();
			foreach (string navigationPrefab : CGQC_NAVIGATION_PREFABS)
			{
				if (prefabName.Contains(navigationPrefab))
					return true;
			}

			prefab = prefab.GetAncestor();
		}

		return false;
	}
}
