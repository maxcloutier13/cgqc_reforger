modded class ACE_Overheating_BarrelComponentClass
{
	protected override float ComputeBarrelMass(IEntity weapon)
	{
		float baseWeight = super.ComputeBarrelMass(weapon);
		Print("[CGQC_HeatFix] ComputeBarrelMass called on " + weapon.GetPrefabData().GetPrefabName() + ", baseWeight = " + baseWeight);
		if (baseWeight > 0.1)
		{
			Print("[CGQC_HeatFix] Normal weapon, returning baseWeight = " + baseWeight);
			return baseWeight;
		}

		Print("[CGQC_HeatFix] Modular weapon detected, searching for barrel attachment");

		array<Managed> slots = {};
		weapon.FindComponents(AttachmentSlotComponent, slots);
		Print("[CGQC_HeatFix] Found " + slots.Count() + " attachment slots");

		foreach (Managed managed : slots)
		{
			AttachmentSlotComponent slot = AttachmentSlotComponent.Cast(managed);
			if (!slot)
				continue;

			IEntity attached = slot.GetAttachedEntity();
			if (!attached)
			{
				Print("[CGQC_HeatFix] Slot has no attached entity, skipping");
				continue;
			}

			string prefabName = attached.GetPrefabData().GetPrefabName();
			Print("[CGQC_HeatFix] Checking attachment: " + prefabName);

			if (!prefabName.Contains("Handguard"))
			{
				Print("[CGQC_HeatFix] Not a handguard, skipping");
				continue;
			}

			Print("[CGQC_HeatFix] Handguard found: " + prefabName);

			InventoryItemComponent invItem = InventoryItemComponent.Cast(attached.FindComponent(InventoryItemComponent));
			if (!invItem)
			{
				Print("[CGQC_HeatFix] No InventoryItemComponent, skipping");
				continue;
			}

			SCR_ItemAttributeCollection attrs = SCR_ItemAttributeCollection.Cast(invItem.GetAttributes());
			if (!attrs)
			{
				Print("[CGQC_HeatFix] No SCR_ItemAttributeCollection, skipping");
				continue;
			}

			float mass = attrs.GetWeight();
			Print("[CGQC_HeatFix] Handguard weight used directly as barrel mass = " + mass);

			if (mass > 0)
				return mass;
		}

		Print("[CGQC_HeatFix] No handguard found, returning FALLBACK_BARREL_MASS");
		return FALLBACK_BARREL_MASS;
	}
}