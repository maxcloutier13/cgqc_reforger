/*
	CGQC_BandolierAutoFill.c

	Fills the bandolier based on type set in Workbench:
	  RIFLE  — mags from current weapon, ~180 rounds, max 46 rounds/mag
	  MG     — 2x belt from current weapon (no round limit)
	  UGL    — 6x40mm grenades detected from weapon's UGL muzzle
	  MIXED  — 180 rifle rounds + 2 pistol mags + 2 frags + 2 smokes
*/

enum CGQC_EBandolierType
{
	RIFLE,
	MG,
	UGL,
	MIXED
}

[ComponentEditorProps(category: "CGQC", description: "Auto-fills bandolier based on type")]
class CGQC_BandolierAutoFillClass : ScriptComponentClass {}

class CGQC_BandolierAutoFill : ScriptComponent
{
	[Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Bandolier type", enums: ParamEnumArray.FromEnum(CGQC_EBandolierType))]
	CGQC_EBandolierType m_eBandolierType;

	static const int    TARGET_ROUNDS    = 180;
	static const int    MG_BELT_COUNT    = 2;
	static const int    MIXED_PISTOL_MAGS = 2;
	static const int    MIXED_FRAGS       = 2;
	static const int    MIXED_SMOKES      = 2;
	static const int    UGL_FRAG      	  = 6;
	static const ResourceName FRAG_PREFAB  = "{E8F00BF730225B00}Prefabs/Weapons/Grenades/Grenade_M67.et";
	static const ResourceName SMOKE_PREFAB = "{9DB69176CEF0EE97}Prefabs/Weapons/Grenades/Smoke_ANM8HC.et";

	[RplProp()]
	bool   m_bFilled      = false;
	bool   m_bFillFailed  = false;
	string m_sFailMessage = string.Empty;

	//------------------------------------------------------------------
	// Get mag prefab from a specific weapon slot (primary or pistol)
	protected ResourceName GetMagFromWeapon(BaseWeaponComponent weapon)
	{
		if (!weapon)
			return ResourceName.Empty;

		BaseMuzzleComponent muzzle = weapon.GetCurrentMuzzle();
		if (!muzzle)
			return ResourceName.Empty;

		BaseMagazineComponent mag = muzzle.GetMagazine();
		if (mag)
		{
			IEntity magEnt = mag.GetOwner();
			if (magEnt)
			{
				EntityPrefabData pd = magEnt.GetPrefabData();
				if (pd)
					return pd.GetPrefabName();
			}
		}

		return muzzle.GetDefaultMagazineOrProjectileName();
	}

	//------------------------------------------------------------------
	// Get UGL grenade prefab — skip first muzzle (primary), take default from second (UGL)
	protected ResourceName GetUGLPrefab(IEntity userEntity)
	{
		BaseWeaponManagerComponent weaponMgr = BaseWeaponManagerComponent.Cast(
			userEntity.FindComponent(BaseWeaponManagerComponent)
		);
		if (!weaponMgr)
			return ResourceName.Empty;

		BaseWeaponComponent weapon = weaponMgr.GetCurrentWeapon();
		if (!weapon)
			return ResourceName.Empty;

		array<BaseMuzzleComponent> muzzles = {};
		weapon.GetMuzzlesList(muzzles);

		// Skip muzzle 0 (primary barrel), use muzzle 1 (UGL)
		if (muzzles.Count() < 2)
			return ResourceName.Empty;

		return muzzles[1].GetDefaultMagazineOrProjectileName();
	}

	//------------------------------------------------------------------
	// Spawn count of prefab into storage, return how many succeeded
	protected int SpawnInto(SCR_UniversalInventoryStorageComponent storage, SCR_InventoryStorageManagerComponent invMgr, ResourceName prefab, int count)
	{
		int spawned = 0;
		for (int i = 0; i < count; i++)
		{
			if (!invMgr.TrySpawnPrefabToStorage(prefab, storage))
				break;
			spawned++;
		}
		return spawned;
	}

	//------------------------------------------------------------------
	void Fill(IEntity userEntity)
	{
		m_bFillFailed = false;
		m_sFailMessage = string.Empty;

		SCR_UniversalInventoryStorageComponent uStorage = SCR_UniversalInventoryStorageComponent.Cast(
			GetOwner().FindComponent(SCR_UniversalInventoryStorageComponent)
		);
		SCR_InventoryStorageManagerComponent invMgr = SCR_InventoryStorageManagerComponent.Cast(
			userEntity.FindComponent(SCR_InventoryStorageManagerComponent)
		);

		if (!uStorage || !invMgr)
		{
			m_bFillFailed = true;
			m_sFailMessage = "Bandolier: Inventory error.";
			return;
		}

		BaseWeaponManagerComponent weaponMgr = BaseWeaponManagerComponent.Cast(
			userEntity.FindComponent(BaseWeaponManagerComponent)
		);
		
		// Capture base weight before spawning anything
		InventoryItemComponent invItem = InventoryItemComponent.Cast(
			GetOwner().FindComponent(InventoryItemComponent)
		);
		float baseWeight = 0;
		if (invItem)
			baseWeight = invItem.GetTotalWeight() - invItem.GetAdditionalWeight();


		switch (m_eBandolierType)
		{
			//------------------------------------------------------
			case CGQC_EBandolierType.RIFLE:
			{
				BaseWeaponComponent weapon = null;
				if (weaponMgr)
					weapon = weaponMgr.GetCurrentWeapon();

				ResourceName magPrefab = GetMagFromWeapon(weapon);
				if (magPrefab == ResourceName.Empty)
				{
					m_bFillFailed = true;
					m_sFailMessage = "Bandolier: Equip a rifle with a magazine first.";
					return;
				}

				// Check rounds per mag
				int magCount = 6;
				Resource res = Resource.Load(magPrefab);
				if (res && res.IsValid())
				{
					EntitySpawnParams p = new EntitySpawnParams();
					p.TransformMode = ETransformMode.WORLD;
					Math3D.MatrixIdentity4(p.Transform);
					IEntity temp = GetGame().SpawnEntityPrefab(res, null, p);
					if (temp)
					{
						BaseMagazineComponent mc = BaseMagazineComponent.Cast(temp.FindComponent(BaseMagazineComponent));
						if (mc)
						{
							int rnd = mc.GetAmmoCount();
							if (rnd > 46)
							{
								SCR_EntityHelper.DeleteEntityAndChildren(temp);
								m_bFillFailed = true;
								m_sFailMessage = "Bandolier: Magazine type not compatible. Use rifle bandolier for rifles only.";
								return;
							}
							if (rnd > 0)
								magCount = TARGET_ROUNDS / rnd;
						}
						SCR_EntityHelper.DeleteEntityAndChildren(temp);
					}
				}

				SpawnInto(uStorage, invMgr, magPrefab, magCount);
				
				m_bFilled = true;
				break;
			}

			//------------------------------------------------------
			case CGQC_EBandolierType.MG:
			{
				BaseWeaponComponent weapon = null;
				if (weaponMgr)
					weapon = weaponMgr.GetCurrentWeapon();

				ResourceName beltPrefab = GetMagFromWeapon(weapon);
				if (beltPrefab == ResourceName.Empty)
				{
					m_bFillFailed = true;
					m_sFailMessage = "Bandolier: Equip an MG with a belt first.";
					return;
				}
				
				// Check rounds per mag
				Resource res = Resource.Load(beltPrefab);
				if (res && res.IsValid())
				{
					EntitySpawnParams p = new EntitySpawnParams();
					p.TransformMode = ETransformMode.WORLD;
					Math3D.MatrixIdentity4(p.Transform);
					IEntity temp = GetGame().SpawnEntityPrefab(res, null, p);
					if (temp)
					{
						BaseMagazineComponent mc = BaseMagazineComponent.Cast(temp.FindComponent(BaseMagazineComponent));
						if (mc)
						{
							int rnd = mc.GetAmmoCount();
							if (rnd < 99)
							{
								SCR_EntityHelper.DeleteEntityAndChildren(temp);
								m_bFillFailed = true;
								m_sFailMessage = "Bandolier: BeltFed not compatible. Use MG bandolier for MG only.";
								return;
							}
						}
						SCR_EntityHelper.DeleteEntityAndChildren(temp);
					}
				}

				SpawnInto(uStorage, invMgr, beltPrefab, MG_BELT_COUNT);
				m_bFilled = true;
				break;
			}

			//------------------------------------------------------
			case CGQC_EBandolierType.UGL:
			{
				ResourceName grenadePrefab = GetUGLPrefab(userEntity);
				if (grenadePrefab == ResourceName.Empty)
				{
					m_bFillFailed = true;
					m_sFailMessage = "Bandolier: No UGL found on current weapon.";
					return;
				}

				// Fill with as many grenades as slots allow
				SpawnInto(uStorage, invMgr, grenadePrefab, UGL_FRAG);
				m_bFilled = true;
				break;
			}

			//------------------------------------------------------
			case CGQC_EBandolierType.MIXED:
			{
				// --- Rifle mags ---
				BaseWeaponComponent primaryWeapon = null;
				if (weaponMgr)
					primaryWeapon = weaponMgr.GetCurrentWeapon();

				ResourceName rifleMag = GetMagFromWeapon(primaryWeapon);
				if (rifleMag != ResourceName.Empty)
				{
					int magCount = 6;
					Resource res = Resource.Load(rifleMag);
					if (res && res.IsValid())
					{
						EntitySpawnParams p = new EntitySpawnParams();
						p.TransformMode = ETransformMode.WORLD;
						Math3D.MatrixIdentity4(p.Transform);
						IEntity temp = GetGame().SpawnEntityPrefab(res, null, p);
						if (temp)
						{
							BaseMagazineComponent mc = BaseMagazineComponent.Cast(temp.FindComponent(BaseMagazineComponent));
							if (mc && mc.GetAmmoCount() > 0)
								magCount = TARGET_ROUNDS / mc.GetAmmoCount();
							SCR_EntityHelper.DeleteEntityAndChildren(temp);
						}
					}
					SpawnInto(uStorage, invMgr, rifleMag, magCount);
				}

				// --- Pistol mags — find pistol in inventory ---
				ResourceName pistolMag = ResourceName.Empty;
				array<IEntity> allItems = {};
				invMgr.GetItems(allItems);
				foreach (IEntity item : allItems)
				{
					BaseWeaponComponent wComp = BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent));
					if (!wComp)
						continue;

					// Check if it's a pistol (not the primary) by checking it's not the current weapon
					if (primaryWeapon && wComp == primaryWeapon)
						continue;

					ResourceName candidate = GetMagFromWeapon(wComp);
					if (candidate != ResourceName.Empty)
					{
						pistolMag = candidate;
						break;
					}
				}

				if (pistolMag != ResourceName.Empty)
					SpawnInto(uStorage, invMgr, pistolMag, MIXED_PISTOL_MAGS);

				// --- Frags and smokes ---
				SpawnInto(uStorage, invMgr, FRAG_PREFAB,  MIXED_FRAGS);
				SpawnInto(uStorage, invMgr, SMOKE_PREFAB, MIXED_SMOKES);

				m_bFilled = true;
				break;
			}
		}
		// Set bandolier container to 0.3kg — mags add their weight on top
		if (m_bFilled && invItem)
			invItem.SetAdditionalWeight(0.3 - baseWeight);
	}
}

//----------------------------------------------------------------------
class CGQC_BandolierOpenAction : ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CGQC_BandolierAutoFill filler = CGQC_BandolierAutoFill.Cast(
			pOwnerEntity.FindComponent(CGQC_BandolierAutoFill)
		);
		if (!filler)
			return;

		if (Replication.IsServer())
			filler.Fill(pUserEntity);

		if (!Replication.IsServer())
		{
			if (filler.m_bFillFailed)
			{
				SCR_HintManagerComponent hintMgr = SCR_HintManagerComponent.GetInstance();
				if (hintMgr)
					hintMgr.ShowCustomHint(filler.m_sFailMessage, "Bandolier", 5.0);
				return;
			}
			// GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.Inventory20Menu);
		}
	}

	override bool CanBeShownScript(IEntity user)
	{
		CGQC_BandolierAutoFill filler = CGQC_BandolierAutoFill.Cast(
			GetOwner().FindComponent(CGQC_BandolierAutoFill)
		);
		return filler && !filler.m_bFilled;
	}

	override bool CanBePerformedScript(IEntity user) { return true; }
}