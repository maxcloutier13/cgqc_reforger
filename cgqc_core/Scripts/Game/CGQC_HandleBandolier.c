/*
	CGQC_BandolierAutoFill.c

	NOTE: Num Slots on the MultiSlotConfiguration is currently 2.
	Either increase it to 6 in the prefab, or keep MAG_COUNT = 2.
*/

class CGQC_BandolierOpenAction : ScriptedUserAction
{
	static const int          MAG_COUNT  = 6;
	static const ResourceName MAG_PREFAB = "{7ECA478D7C80ACC0}Prefabs/Weapons/Magazines/Magazine_556x45_STANAG_30rnd_M193_M196_Last_5Tracer.et";

	//------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		PrintFormat("[CGQC_Bandolier] PerformAction fired. IsServer=%1", Replication.IsServer());

		if (Replication.IsServer())
			FillBandolier(pOwnerEntity, pUserEntity);
		else
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.Inventory20Menu);
	}

	//------------------------------------------------------------------
	protected void FillBandolier(IEntity ownerEntity, IEntity userEntity)
	{
		SCR_UniversalInventoryStorageComponent uStorage = SCR_UniversalInventoryStorageComponent.Cast(
			ownerEntity.FindComponent(SCR_UniversalInventoryStorageComponent)
		);
		if (!uStorage)
		{
			PrintFormat("[CGQC_Bandolier] ERROR: No SCR_UniversalInventoryStorageComponent found.");
			return;
		}
		PrintFormat("[CGQC_Bandolier] Storage found. Slots=%1", uStorage.GetSlotsCount());

		SCR_InventoryStorageManagerComponent invMgr = SCR_InventoryStorageManagerComponent.Cast(
			userEntity.FindComponent(SCR_InventoryStorageManagerComponent)
		);
		if (!invMgr)
		{
			PrintFormat("[CGQC_Bandolier] ERROR: No SCR_InventoryStorageManagerComponent on user.");
			return;
		}

		int spawned = 0;
		for (int i = 0; i < MAG_COUNT; i++)
		{
			bool ok = invMgr.TrySpawnPrefabToStorage(MAG_PREFAB, uStorage);
			PrintFormat("[CGQC_Bandolier] Spawn %1 result=%2", i, ok);
			if (!ok)
				break;
			spawned++;
		}

		PrintFormat("[CGQC_Bandolier] Done. Spawned %1/%2 mags.", spawned, MAG_COUNT);
	}

	//------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
}