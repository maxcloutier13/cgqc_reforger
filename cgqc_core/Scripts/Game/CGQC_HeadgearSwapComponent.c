[BaseContainerProps()]
class CGQC_HeadgearSwapComponentClass : ScriptComponentClass {}

class CGQC_HeadgearSwapComponent : ScriptComponent
{
	[RplProp()]
	protected ResourceName m_StashedHelmet;

	[RplProp()]
	protected bool m_bBeretWorn;

	[RplProp()]
	protected ResourceName m_AssignedBeretPrefab;

	void SetAssignedBeretPrefab(ResourceName beretPrefab)
	{
		m_AssignedBeretPrefab = beretPrefab;
		Replication.BumpMe();
	}

	ResourceName GetAssignedBeretPrefab()
	{
		return m_AssignedBeretPrefab;
	}

	void RequestToggleBeret(ResourceName beretPrefab)
	{
		Rpc(RpcAsk_ToggleBeret, beretPrefab);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleBeret(ResourceName beretPrefab)
	{
		
		IEntity playerEntity = GetOwner();
		if (!playerEntity)
		{
			return;
		}

		EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(playerEntity.FindComponent(EquipedLoadoutStorageComponent));
		SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!loadout)
		{
			return;
		}
		if (!invManager)
		{
			return;
		}

		IEntity currentHead = loadout.GetClothFromArea(LoadoutHeadCoverArea);

		if (!m_bBeretWorn)
		{
			// Helmet -> beret
			if (currentHead)
			{
				EntityPrefabData pd = currentHead.GetPrefabData();
				if (pd)
				{
					m_StashedHelmet = pd.GetPrefabName();

				}

				bool removed = invManager.TryRemoveItemFromInventory(currentHead);
				SCR_EntityHelper.DeleteEntityAndChildren(currentHead);
			}

			Resource beretRes = Resource.Load(beretPrefab);
			if (!beretRes || !beretRes.IsValid())
			{
				return;
			}

			EntitySpawnParams sp = new EntitySpawnParams();
			sp.TransformMode = ETransformMode.WORLD;
			playerEntity.GetTransform(sp.Transform);
			IEntity beret = GetGame().SpawnEntityPrefab(beretRes, GetGame().GetWorld(), sp);
			if (!beret)
			{
				return;
			}

			bool inserted = invManager.EquipAny(invManager.GetCharacterStorage(), beret);

			m_bBeretWorn = true;
		}
		else
		{
			// Beret -> helmet
			if (currentHead)
			{

				SCR_EntityHelper.DeleteEntityAndChildren(currentHead);
			}


			if (m_StashedHelmet.IsEmpty())
			{
		
				return;
			}

			Resource helmetRes = Resource.Load(m_StashedHelmet);
			if (!helmetRes || !helmetRes.IsValid())
			{
	
				return;
			}

			EntitySpawnParams sp = new EntitySpawnParams();
			sp.TransformMode = ETransformMode.WORLD;
			playerEntity.GetTransform(sp.Transform);
			IEntity helmet = GetGame().SpawnEntityPrefab(helmetRes, GetGame().GetWorld(), sp);
			if (!helmet)
			{
	
				return;
			}
	

			bool inserted = invManager.EquipAny(invManager.GetCharacterStorage(), helmet);
	

			m_bBeretWorn = false;
		}

		Replication.BumpMe();

	}
}