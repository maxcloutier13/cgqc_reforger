// ============================================================
// CGQC_Inspection.c  (cgqc_scripts)
// Vérification de l'Équipement — pre-mission loadout check.
//
// SETUP:
//   Add CGQC_InspectionAction to the character prefab's
//   ActionsManagerComponent in Workbench.
//   Point m_sConfigPath at your .conf file.
//
// USAGE:
//   TL looks at a soldier → activates action.
//   Result popup appears on both TL and soldier screens.
// ============================================================

class CGQC_InspectionAction : ScriptedUserAction
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Fichier de configuration (.conf)", "conf")]
	ResourceName m_sConfigPath;

	static const float DISPLAY_DURATION = 20.0;

	// ─────────────────────────────────────────────────────────────────────────

	override bool GetActionNameScript(out string outName)
	{
		outName = "Vérification Équipement";
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return false;
	
		int targetPlayerId = pm.GetPlayerIdFromControlledEntity(GetOwner());
		if (targetPlayerId <= 0)
			return false;
	
		int checkerPlayerId = pm.GetPlayerIdFromControlledEntity(user);
		if (checkerPlayerId <= 0)
			return false;
	
		// Self-inspection stays open to everyone
		if (checkerPlayerId == targetPlayerId)
			return true;
	
		// Inspecting someone else requires GM
		return pm.HasPlayerRole(checkerPlayerId, EPlayerRole.GAME_MASTER);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return true;
	}

	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}

	// ─────────────────────────────────────────────────────────────────────────

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		Print("[CGQC_Inspection] PerformAction called", LogLevel.NORMAL);

		CGQC_InspectionConfig config = LoadConfig();
		if (!config)
		{
			Print("[CGQC_Inspection] Failed to load config: " + m_sConfigPath, LogLevel.ERROR);
			return;
		}

		Print("[CGQC_Inspection] Config loaded OK", LogLevel.NORMAL);

		ChimeraCharacter targetChar = ChimeraCharacter.Cast(pOwnerEntity);
		if (!targetChar)
			return;

		SCR_InventoryStorageManagerComponent invMgr = SCR_InventoryStorageManagerComponent.Cast(
			targetChar.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!invMgr)
			return;

		SCR_CharacterInventoryStorageComponent charStorage = invMgr.GetCharacterStorage();
		if (!charStorage)
			return;

		// Gather all items once — reused by all category checks
		array<IEntity> allItems = new array<IEntity>();
		invMgr.GetItems(allItems);

		// ── POIDS ─────────────────────────────────────────────────────────────
		float weightKg = invMgr.GetTotalWeightOfAllStorages();
		int weightInt = Math.Round(weightKg);
		string weightColor;
		if (weightKg >= config.m_fWeightBlack)
			weightColor = "20,20,20,255";        // noir — surcharge
		else if (weightKg >= config.m_fWeightRed)
			weightColor = "240,90,90,255";       // rouge
		else if (weightKg >= config.m_fWeightOrange)
			weightColor = "245,150,40,255";      // orange
		else if (weightKg >= config.m_fWeightYellow)
			weightColor = "245,225,70,255";      // jaune
		else
			weightColor = "120,210,120,255";     // vert

		string result = "<color rgba=\"130,185,255,255\"><b>POIDS</b></color>  <color rgba=\"" + weightColor + "\"><b>" + weightInt + " kg</b></color>";

		// ── ARMES (une catégorie par emplacement) ─────────────────────────────
		BaseWeaponManagerComponent weapMgr = BaseWeaponManagerComponent.Cast(
			targetChar.FindComponent(BaseWeaponManagerComponent));

		WeaponSlotComponent slotPrimary = null;
		WeaponSlotComponent slotSecondary = null;
		WeaponSlotComponent slotHandgun = null;
		if (weapMgr)
		{
			array<WeaponSlotComponent> slots = new array<WeaponSlotComponent>();
			weapMgr.GetWeaponsSlots(slots);
			foreach (WeaponSlotComponent slot : slots)
			{
				if (!slot)
					continue;
				int sidx = slot.GetWeaponSlotIndex();
				if (sidx == 0)
					slotPrimary = slot;
				else if (sidx == 1)
					slotSecondary = slot;
				else if (sidx == 2)
					slotHandgun = slot;
			}
		}

		bool hasPrimary = SlotHasWeapon(slotPrimary);
		bool hasSecondary = SlotHasWeapon(slotSecondary);
		bool hasHandgun = SlotHasWeapon(slotHandgun);

		// Primaire: shown if present. If neither primary nor secondary exists → red alert header.
		if (hasPrimary)
			result = result + "\n" + BuildWeaponCategory("Primaire", slotPrimary, config.m_iPrimaryMagMin, invMgr);
		else if (!hasSecondary)
			result = result + "\n" + HdrCol("Primaire", "240,90,90,255");

		// Secondaire / Pistolet: shown only when present.
		if (hasSecondary)
			result = result + "\n" + BuildWeaponCategory("Secondaire", slotSecondary, config.m_iSecondaryMagMin, invMgr);

		if (hasHandgun)
			result = result + "\n" + BuildWeaponCategory("Pistolet", slotHandgun, config.m_iHandgunMagMin, invMgr);

		// ── COMBAT ────────────────────────────────────────────────────────────
		result = result + "\n" + Hdr("COMBAT");

		// Configurable combat items (grenades, smokes, knife, entrenching tool, etc.)
		result = result + BuildConfigLines(allItems, config.m_aCombatItems);

		// ── PROTECTION ────────────────────────────────────────────────────────
		result = result + "\n" + Hdr("PROTECTION");

		// Casque — must be an armored helmet, not soft cover (boonie, cap).
		// Helmets carry SCR_ArmorDamageManagerComponent; soft headgear does not. (BI wiki confirmed)
		IEntity headgear = charStorage.GetClothFromArea(LoadoutHeadCoverArea);
		bool helmetOk = false;
		if (headgear)
		{
			SCR_ArmorDamageManagerComponent armor = SCR_ArmorDamageManagerComponent.Cast(
				headgear.FindComponent(SCR_ArmorDamageManagerComponent));
			helmetOk = (armor != null);
		}
		result = result + "\n" + PresenceLine("Casque", helmetOk);

		// Configurable protection items (eyes, ears, etc.)
		result = result + BuildConfigLines(allItems, config.m_aProtectionItems);

		// ── MÉDICAL ───────────────────────────────────────────────────────────
		result = result + "\n" + Hdr("MÉDICAL");
		result = result + BuildConfigLines(allItems, config.m_aMedicalItems);

		// ── ADMIN ─────────────────────────────────────────────────────────────
		result = result + "\n" + Hdr("ADMIN");

		// Radio scan — listed informationally, red only if none found
		string radioBlock = "";
		bool foundRadio = false;

		foreach (IEntity item : allItems)
		{
			if (!item)
				continue;

			BaseRadioComponent radio = BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
			if (!radio)
				continue;

			foundRadio = true;
			EntityPrefabData rpd = item.GetPrefabData();
			string radioName;
			if (rpd)
				radioName = GetFileBaseName(rpd.GetPrefabName());
			else
				radioName = "Radio";

			if (!radio.IsPowered())
			{
				radioBlock = radioBlock + "\n  <color rgba=\"245,200,70,255\">" + radioName + " (éteinte)</color>";
				continue;
			}

			int tCount = radio.TransceiversCount();
			for (int i = 0; i < tCount; i++)
			{
				BaseTransceiver tsv = radio.GetTransceiver(i);
				if (!tsv)
					continue;
				float freqMHz = tsv.GetFrequency() / 1000.0;
				radioBlock = radioBlock + "\n  <color rgba=\"255,255,255,255\">" + radioName + "  —  " + FreqToString(freqMHz) + " MHz</color>";
			}
		}

		if (foundRadio)
			result = result + radioBlock;
		else
			result = result + "\n" + ColorLine("Radio: aucune", false);

		// Configurable admin items (map, compass, watch)
		result = result + BuildConfigLines(allItems, config.m_aAdminItems);

		// ── CATÉGORIES SUPPLÉMENTAIRES ────────────────────────────────────────
		if (config.m_aExtraCategories)
		{
			foreach (CGQC_InspectionCategory cat : config.m_aExtraCategories)
			{
				if (!cat || cat.m_sCategoryName.IsEmpty())
					continue;
				result = result + "\n" + Hdr(cat.m_sCategoryName);
				result = result + BuildConfigLines(allItems, cat.m_aItems);
			}
		}

		// ── Envoyer aux deux joueurs ───────────────────────────────────────────
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;

		int soldierPlayerId = pm.GetPlayerIdFromControlledEntity(pOwnerEntity);
		string soldierName = CGQC_Scripts.GetCustomPlayerName(soldierPlayerId);
		if (soldierName.IsEmpty())
			soldierName = "Soldat";

		string title = "<color rgba=\"240,240,240,255\"><b>Inspection: " + soldierName + "</b></color>";

		Print("[CGQC_Inspection] Sending result. checker=" + pUserEntity + " soldier=" + soldierName, LogLevel.NORMAL);

		// Send to checker (TL or self)
		int checkerPlayerId = pm.GetPlayerIdFromControlledEntity(pUserEntity);
		SCR_PlayerController checkerPC = SCR_PlayerController.Cast(pm.GetPlayerController(checkerPlayerId));
		if (checkerPC)
			checkerPC.CGQC_RpcDo_ShowInspection(title, result);

		// Send to soldier if different from checker
		if (soldierPlayerId != checkerPlayerId)
		{
			SCR_PlayerController soldierPC = SCR_PlayerController.Cast(pm.GetPlayerController(soldierPlayerId));
			if (soldierPC)
				soldierPC.CGQC_RpcDo_ShowInspection(title, result);
		}
	}

	// ─── Helpers ──────────────────────────────────────────────────────────────

	protected CGQC_InspectionConfig LoadConfig()
	{
		Resource res = Resource.Load(m_sConfigPath);
		if (!res.IsValid())
			return null;
		return CGQC_InspectionConfig.Cast(
			BaseContainerTools.CreateInstanceFromContainer(res.GetResource().ToBaseContainer()));
	}

	// Count how many items match ANY of the given prefab ResourceNames
	protected int CountItems(array<IEntity> allItems, array<ResourceName> prefabNames)
	{
		if (!prefabNames || prefabNames.IsEmpty())
			return 0;

		int count = 0;
		foreach (IEntity item : allItems)
		{
			if (!item)
				continue;
			EntityPrefabData pd = item.GetPrefabData();
			if (!pd)
				continue;
			if (prefabNames.Contains(pd.GetPrefabName()))
				count++;
		}
		return count;
	}

	// Build formatted lines for every item in a config list (white=ok, red=short)
	protected string BuildConfigLines(array<IEntity> allItems, array<ref CGQC_InspectionItem> items)
	{
		string lines = "";
		if (!items)
			return lines;

		foreach (CGQC_InspectionItem item : items)
		{
			if (!item || !item.m_aPrefabNames || item.m_aPrefabNames.IsEmpty())
				continue;

			int count = CountItems(allItems, item.m_aPrefabNames);
			bool ok = count >= item.m_iMinCount;

			string label = item.m_sDisplayName;
			if (item.m_iMinCount > 1)
				label = item.m_sDisplayName + "  " + count + "/" + item.m_iMinCount;

			lines = lines + "\n" + ColorLine(label, ok);
		}
		return lines;
	}

	// True if the slot exists and holds a weapon.
	protected bool SlotHasWeapon(WeaponSlotComponent slot)
	{
		if (!slot)
			return false;
		return slot.GetWeaponEntity() != null;
	}

	// One weapon slot as its own category: header + weapon/chamber line + mag line.
	// Caller guarantees the slot holds a weapon. Green = chargé, red = déchargé.
	protected string BuildWeaponCategory(string label, WeaponSlotComponent slot, int magMin, SCR_InventoryStorageManagerComponent invMgr)
	{
		IEntity weap = null;
		if (slot)
			weap = slot.GetWeaponEntity();

		if (!weap)
			return "";

		string section = Hdr(label);

		// Weapon name
		string weapName = GetItemDisplayName(weap);
		if (weapName == "")
			weapName = "Arme";

		// Chamber + ammo-per-mag from current barrel
		BaseWeaponComponent weapComp = BaseWeaponComponent.Cast(weap.FindComponent(BaseWeaponComponent));
		
		
		
		bool chambered = false;
		int ammoPerMag = 0;
		if (weapComp)
		{
		    array<BaseMuzzleComponent> muzzles = new array<BaseMuzzleComponent>();
		    weapComp.GetMuzzlesList(muzzles);
		    if (muzzles.Count() > 0)
		    {
		        ammoPerMag = muzzles[0].GetMaxAmmoCount();
		
		        // IsCurrentBarrelChambered() est fiable seulement sur l'arme active.
		        // Pour les armes en holster, on utilise GetAmmoCount() > 0 comme proxy.
		        // GetAmmoCount() inclut la chambre + le mag inséré, donc > 0 = opérationnel.
		        BaseMagazineComponent mag = muzzles[0].GetMagazine();
		        if (mag)
		            chambered = muzzles[0].IsCurrentBarrelChambered() || (muzzles[0].GetAmmoCount() > 0);
		        else
		            chambered = muzzles[0].IsCurrentBarrelChambered(); // pas de mag, seule la chambre compte
		    }
		}
		
		/*
		bool chambered = false;
		int ammoPerMag = 0;
		int s_check = 0;
		if (weapComp)
		{
			array<BaseMuzzleComponent> muzzles = new array<BaseMuzzleComponent>();
			weapComp.GetMuzzlesList(muzzles);
			if (muzzles.Count() > 0)
			{
				ammoPerMag = muzzles[0].GetMaxAmmoCount();
				s_check = muzzles[0].IsCurrentBarrelChambered();
				if (muzzles[0].IsCurrentBarrelChambered())
					chambered = true;
			}
		}*/

		string chColor;
		string chStatus;
		if (chambered)
		{
			chColor = "120,210,120,255";
			chStatus = "Chargé";
		}
		else
		{
			chColor = "240,90,90,255";
			chStatus = "Déchargé";
		}
		section = section + "\n  <color rgba=\"" + chColor + "\">" + weapName + " — " + chStatus + "</color>";

		// Spare magazines + total cartridges
		int magCount = 0;
		if (weapComp)
			magCount = invMgr.GetMagazineCountByWeapon(weapComp);
		int totalAmmo = magCount * ammoPerMag;
		string magLabel = "Chargeurs: " + magCount + "/" + magMin + " — " + totalAmmo + " cartouches";
		section = section + "\n" + ColorLine(magLabel, magCount >= magMin);

		return section;
	}

	// Arsenal display name of an item via UIInfo (BI pattern: GetAttributes().GetUIInfo().GetName())
	protected string GetItemDisplayName(IEntity item)
	{
		if (!item)
			return "";

		InventoryItemComponent invComp = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (!invComp)
			return "";

		ItemAttributeCollection attribs = invComp.GetAttributes();
		if (!attribs)
			return "";

		UIInfo info = attribs.GetUIInfo();
		if (!info)
			return "";

		return info.GetName();
	}

	// Section header — "-- NAME ----" padded with dashes to a fixed total width.
	protected string HdrCol(string name, string rgba)
	{
		int target = 28;
		int dashCount = target - 4 - name.Length();
		if (dashCount < 2)
			dashCount = 2;

		string dashes = "";
		int i;
		for (i = 0; i < dashCount; i++)
			dashes = dashes + "-";

		return "\n<color rgba=\"" + rgba + "\"><b>-- " + name + " " + dashes + "</b></color>";
	}

	// Category header — bold, accent blue
	protected string Hdr(string name)
	{
		return HdrCol(name, "130,185,255,255");
	}

	// Indented item line — white if ok, red if short. Color is the indicator.
	protected string ColorLine(string label, bool ok)
	{
		string color;
		if (ok)
			color = "255,255,255,255";
		else
			color = "240,90,90,255";
		return "  <color rgba=\"" + color + "\">" + label + "</color>";
	}

	// Presence-only line (helmet, chamber) — white if present, red if not
	protected string PresenceLine(string label, bool ok)
	{
		return ColorLine(label, ok);
	}

	// Extract filename without extension from a ResourceName path
	// e.g. "{GUID}Prefabs/.../Radio_ANPRC152.et" → "Radio_ANPRC152"
	protected string GetFileBaseName(string path)
	{
		int len = path.Length();
		int lastSlash = 0;

		for (int i = 0; i < len; i++)
		{
			string c = path.Substring(i, 1);
			if (c == "/" || c == "\\")
				lastSlash = i + 1;
		}

		string name = path.Substring(lastSlash, len - lastSlash);
		int nameLen = name.Length();

		for (int i = nameLen - 1; i >= 0; i--)
		{
			if (name.Substring(i, 1) == ".")
			{
				name = name.Substring(0, i);
				break;
			}
		}
		return name;
	}

	// Format float frequency to "XX.XXX" string
	// Input: 45.5 → "45.500" | 38.25 → "38.250"
	protected string FreqToString(float freq)
	{
		int whole = (int)freq;
		int frac = Math.Round((freq - whole) * 1000);
		string fracStr;
		if (frac < 10)
			fracStr = "00" + frac;
		else if (frac < 100)
			fracStr = "0" + frac;
		else
			fracStr = "" + frac;
		return whole.ToString() + "." + fracStr;
	}
}