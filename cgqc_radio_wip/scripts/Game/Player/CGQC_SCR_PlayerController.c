modded class SCR_PlayerController
{
	//------------------------------------------------------------------------------------------------
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
		
		// AUTO-ENABLE VON DIRECT TOGGLE
		AutoEnableVONDirect();
	}

	//------------------------------------------------------------------------------------------------
	// Auto-enable VON Direct toggle on spawn/respawn
	protected void AutoEnableVONDirect()
	{
		SCR_VONController vonController = SCR_VONController.Cast(FindComponent(SCR_VONController));
		if (!vonController)
			return;
		
		vonController.ForceEnableVONDirectToggle();
	}
}