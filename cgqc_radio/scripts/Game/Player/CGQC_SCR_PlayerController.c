/*
modded class SCR_PlayerController
{
	//------------------------------------------------------------------------------------------------
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
		
		// Auto-Enable voice after 5secs
		GetGame().GetCallqueue().CallLater(AutoEnableVONDirect, 5000, false);
		// Checks again after 30secs in case
		GetGame().GetCallqueue().CallLater(AutoEnableVONDirect, 30000, false);
		// Checks again after 60secs in case
		GetGame().GetCallqueue().CallLater(AutoEnableVONDirect, 60000, false);
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
}*/