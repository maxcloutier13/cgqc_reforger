/*
	CGQC_BandolierAutoFill.c

	Fills the bandolier with 6x STANAG mags on first use, then opens inventory.
	Add CGQC_BandolierAutoFill (ScriptComponent) and CGQC_BandolierOpenAction
	(ScriptedUserAction) to Bandolier_Ammo.et.
*/

[ComponentEditorProps(category: "CGQC", description: "Auto-fills bandolier with STANAG mags on first open")]
class CGQC_BandolierAutoFillClass : ScriptComponentClass {}

class CGQC_BandolierAutoFill : ScriptComponent
{
	static const int          MAG_COUNT  = 6;
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

		// Try the currently loaded mag first
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

		// Fallback: get the weapon's default mag directly from the muzzle
		ResourceName defaultMag = muzzle.GetDefaultMagazineOrProjectileName();
		if (!defaultMag.IsEmpty())
		{
			PrintFormat("[CGQC_Bandolier] No loaded mag, using muzzle default: %1", defaultMag);
			return defaultMag;
		}
		return ResourceName.Empty;
	}

	//------------------------------------------------------------------
	void FillAndOpen(IEntity userEntity)
	{
		if (!m_bFilled)
		{
			ResourceName MAG_PREFAB = GetMagPrefabFromPlayer(userEntity);
			if (MAG_PREFAB == ResourceName.Empty)
			{
				PrintFormat("[CGQC_Bandolier] No mag found in player's current weapon, aborting fill.");
				return;
			}
			PrintFormat("[CGQC_Bandolier] Using mag prefab: %1", MAG_PREFAB);
			
			SCR_UniversalInventoryStorageComponent uStorage = SCR_UniversalInventoryStorageComponent.Cast(
				GetOwner().FindComponent(SCR_UniversalInventoryStorageComponent)
			);

			SCR_InventoryStorageManagerComponent invMgr = SCR_InventoryStorageManagerComponent.Cast(
				userEntity.FindComponent(SCR_InventoryStorageManagerComponent)
			);

			if (uStorage && invMgr)
			{
				for (int i = 0; i < MAG_COUNT; i++)
				{
					if (!invMgr.TrySpawnPrefabToStorage(MAG_PREFAB, uStorage))
						break;
				}
				m_bFilled = true;
			}
		}

		int playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(userEntity);
		Rpc(RpcDo_OpenInventory, playerID);
	}

	//------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_OpenInventory(int playerID)
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc || pc.GetPlayerId() != playerID)
			return;

		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.Inventory20Menu);
	}
}

//----------------------------------------------------------------------
class CGQC_BandolierOpenAction : ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		CGQC_BandolierAutoFill filler = CGQC_BandolierAutoFill.Cast(
			pOwnerEntity.FindComponent(CGQC_BandolierAutoFill)
		);
		if (filler)
			filler.FillAndOpen(pUserEntity);
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