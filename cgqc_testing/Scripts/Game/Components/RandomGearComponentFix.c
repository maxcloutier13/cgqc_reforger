modded class RandomGearComponent : ScriptComponent
{
    override void SpawnItemAndQueueContents(IEntity owner, SCR_InventoryStorageManagerComponent invManager, SCR_GearPoolConfig config, map<ResourceName, ref array<ResourceName>> pendingPockets)
    {
        if (!config) return;
		if (config.m_aGearPool.IsEmpty() && config.m_aAdvancedPool.IsEmpty() && config.m_aGlobalItems.IsEmpty()) return;
		
		super.SpawnItemAndQueueContents(owner, invManager, config, pendingPockets);
	}
}