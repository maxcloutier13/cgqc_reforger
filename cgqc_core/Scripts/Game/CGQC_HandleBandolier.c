enum CGQC_EBandolierType
{
	RIFLE,	// max 46 rounds per mag, ~180 rounds total
	MG		// no round limit, fixed 1 mag
}


[ComponentEditorProps(category: "CGQC", description: "Auto-fills bandolier with player's mag type on first open")]
class CGQC_BandolierAutoFillClass : ScriptComponentClass {}

class CGQC_BandolierAutoFill : ScriptComponent
{
	[Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Bandolier type", enums: ParamEnumArray.FromEnum(CGQC_EBandolierType))]
	CGQC_EBandolierType m_eBandolierType;
	
	// Setting: target round for Rifle setups
	static const int TARGET_ROUNDS = 180;
	
	bool m_bFilled = false;

	//------------------------------------------------------------------
	protected ResourceName GetMagPrefabFromPlayer(IEntity userEntity)
	{
		BaseWeaponManagerComponent weaponMgr = BaseWeaponManagerComponent.Cast(
			userEntity.FindComponent(BaseWeaponManagerComponent)
		);
		if (!weaponMgr)
			return ResourceName.Empty;

		BaseWeaponComponent weapon = weaponMgr.GetCurrentWeapon();
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
				EntityPrefabData prefabData = magEnt.GetPrefabData();
				if (prefabData)
					return prefabData.GetPrefabName();
			}
		}

		ResourceName defaultMag = muzzle.GetDefaultMagazineOrProjectileName();
		if (!defaultMag.IsEmpty())
			return defaultMag;

		return ResourceName.Empty;
	}

	//------------------------------------------------------------------
	// Called server-side — fills the bandolier, sets result flags
	void Fill(IEntity userEntity)
	{
		ResourceName magPrefab = GetMagPrefabFromPlayer(userEntity);
		if (magPrefab == ResourceName.Empty)
		{
			PrintFormat("[CGQC_Bandolier] No mag found.");
			return;
		}

		int magCount = 6;
		Resource magRes = Resource.Load(magPrefab);
		if (magRes && magRes.IsValid())
		{
			EntitySpawnParams p = new EntitySpawnParams();
			p.TransformMode = ETransformMode.WORLD;
			Math3D.MatrixIdentity4(p.Transform);
			IEntity tempMag = GetGame().SpawnEntityPrefab(magRes, null, p);
			if (tempMag)
			{
				BaseMagazineComponent mc = BaseMagazineComponent.Cast(tempMag.FindComponent(BaseMagazineComponent));
				if (mc)
				{
					int rnd = mc.GetAmmoCount();
					if (rnd > 46)
					{
						SCR_EntityHelper.DeleteEntityAndChildren(tempMag);
						return;
					}
					if (rnd > 0)
						magCount = TARGET_ROUNDS / rnd;
				}
				SCR_EntityHelper.DeleteEntityAndChildren(tempMag);
			}
		}

		SCR_UniversalInventoryStorageComponent uStorage = SCR_UniversalInventoryStorageComponent.Cast(
			GetOwner().FindComponent(SCR_UniversalInventoryStorageComponent)
		);
		SCR_InventoryStorageManagerComponent invMgr = SCR_InventoryStorageManagerComponent.Cast(
			userEntity.FindComponent(SCR_InventoryStorageManagerComponent)
		);

		if (uStorage && invMgr)
		{
			for (int i = 0; i < magCount; i++)
			{
				if (!invMgr.TrySpawnPrefabToStorage(magPrefab, uStorage))
					break;
			}
			m_bFilled = true;
		}
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