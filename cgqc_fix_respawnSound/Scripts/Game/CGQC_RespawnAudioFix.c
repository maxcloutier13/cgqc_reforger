modded class SCR_PlayerController
{
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);

		if (!to)
			return;

		if (GetPlayerId() != SCR_PlayerController.GetLocalPlayerId())
			return;

		// Reset CharacterLifeState global variable driving the unconscious LPF filter
		GameSignalsManager sigManager = GetGame().GetSignalsManager();
		if (sigManager)
		{
			int idx = sigManager.FindSignal("CharacterLifeState");
			if (idx >= 0)
				sigManager.SetSignalValue(idx, 0);
		}
	}
}