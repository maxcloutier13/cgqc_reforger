modded class SCR_UniversalInventoryStorageComponent
{
	override bool CanStoreItem(IEntity item, int slotID)
	{
		if (!super.CanStoreItem(item, slotID))
			return false;

		IEntity owner = GetOwner();
		if (!owner)
			return true;

		if (!owner.FindComponent(CGQC_MedicalStorageTag))
			return true;

		if (item.FindComponent(SCR_ExplosiveChargeInventoryItemComponent))
			return false;

		return true;
	}
}