class CGQC_LightSwitchAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CGQC_LightSwitchComponent switchComp = CGQC_LightSwitchComponent.Cast(
			pOwnerEntity.FindComponent(CGQC_LightSwitchComponent)
		);

		if (!switchComp)
			return;

		switchComp.ToggleLights();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user) { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript() { return false; }
}

//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "CGQC/Lights", description: "Toggles LightEntity children within radius via slot attachment")]
class CGQC_LightSwitchComponentClass : ScriptComponentClass {}

class CGQC_LightSwitchComponent : ScriptComponent
{
	[Attribute("40", UIWidgets.EditBox, "Search radius in meters")]
	protected float m_fRadius;

	protected bool m_bTurnOn;

	//------------------------------------------------------------------------------------------------
	void ToggleLights()
	{
		m_bTurnOn = true;

		// First pass: check current state
		GetOwner().GetWorld().QueryEntitiesBySphere(
			GetOwner().GetOrigin(),
			m_fRadius,
			CheckCurrentState,
			null,
			EQueryEntitiesFlags.STATIC | EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT
		);

		// Second pass: apply
		GetOwner().GetWorld().QueryEntitiesBySphere(
			GetOwner().GetOrigin(),
			m_fRadius,
			ApplyToEntity,
			null,
			EQueryEntitiesFlags.STATIC | EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT
		);
	}

	//------------------------------------------------------------------------------------------------
	protected LightEntity FindLightEntity(IEntity ent)
	{
		// Maybe it IS the LightEntity directly
		LightEntity light = LightEntity.Cast(ent);
		if (light)
			return light;

		// Check slot-attached children
		array<EntitySlotInfo> slots = {};
		EntitySlotInfo.GetSlotInfos(ent, slots);
		foreach (EntitySlotInfo slot : slots)
		{
			IEntity child = slot.GetAttachedEntity();
			if (!child)
				continue;

			light = LightEntity.Cast(child);
			if (light)
				return light;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CheckCurrentState(IEntity ent)
	{
		LightEntity light = FindLightEntity(ent);
		if (!light)
			return true;

		if (light.IsEnabled())
		{
			m_bTurnOn = false;
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ApplyToEntity(IEntity ent)
	{
		LightEntity light = FindLightEntity(ent);
		if (!light)
			return true;

		light.SetEnabled(m_bTurnOn);
		return true;
	}
}