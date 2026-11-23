modded class ACE_Overheating_BarrelComponentClass
{
	protected override float ComputeBarrelMass(IEntity weapon)
	{
		float baseWeight = super.ComputeBarrelMass(weapon);
		if (baseWeight > 0.1)
			return baseWeight;


		array<Managed> slots = {};
		weapon.FindComponents(AttachmentSlotComponent, slots);

		foreach (Managed managed : slots)
		{
			AttachmentSlotComponent slot = AttachmentSlotComponent.Cast(managed);
			if (!slot)
				continue;

			IEntity attached = slot.GetAttachedEntity();
			if (!attached)
				continue;

			string prefabName = attached.GetPrefabData().GetPrefabName();

			if (!prefabName.Contains("Handguard"))
				continue;

			InventoryItemComponent invItem = InventoryItemComponent.Cast(attached.FindComponent(InventoryItemComponent));
			if (!invItem)
				continue;

			SCR_ItemAttributeCollection attrs = SCR_ItemAttributeCollection.Cast(invItem.GetAttributes());
			if (!attrs)
				continue;

			float mass = attrs.GetWeight();
			if (mass > 0)
				return mass;
		}

		// Just use the mod's fallback mass then (1)
		return FALLBACK_BARREL_MASS;
	}
}