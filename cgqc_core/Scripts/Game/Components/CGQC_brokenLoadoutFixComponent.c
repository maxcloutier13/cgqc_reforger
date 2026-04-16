[ComponentEditorProps(category: "CGQC", description: "Forces belt and weapon spawn post-init for training units.")]
class CGQC_TrainingLoadoutFixComponentClass : ScriptComponentClass {}

class CGQC_TrainingLoadoutFixComponent : ScriptComponent
{
    [Attribute(desc: "Belt prefab to force spawn.", params: "et")]
    ResourceName m_BeltPrefab;

    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
    }

    override void EOnInit(IEntity owner)
    {
        GetGame().GetCallqueue().CallLater(SpawnBelt, 100, false, owner);
    }

    void SpawnBelt(IEntity owner)
    {
        if (!owner) return;

        SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
        if (!invManager) return;

        if (!m_BeltPrefab.IsEmpty())
            invManager.TrySpawnPrefabToStorage(m_BeltPrefab);
    }
}