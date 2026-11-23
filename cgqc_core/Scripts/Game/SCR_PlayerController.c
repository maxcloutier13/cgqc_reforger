modded class SCR_PlayerController : PlayerController
{
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);


		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			inputManager.AddActionListener("CGQC_ToggleBeret", EActionTrigger.DOWN, CGQC_OnToggleBeretInput);
		}

	}

	protected void CGQC_OnToggleBeretInput()
	{

		IEntity playerEntity = GetControlledEntity();
		if (!playerEntity)
		{
			return;
		}

		CGQC_HeadgearSwapComponent swap = CGQC_HeadgearSwapComponent.Cast(playerEntity.FindComponent(CGQC_HeadgearSwapComponent));
		if (!swap)
		{
			
			return;
		}

		ResourceName beretPrefab = swap.GetAssignedBeretPrefab();
	

		if (beretPrefab.IsEmpty())
		{

			return;
		}

		swap.RequestToggleBeret(beretPrefab);
	}
}