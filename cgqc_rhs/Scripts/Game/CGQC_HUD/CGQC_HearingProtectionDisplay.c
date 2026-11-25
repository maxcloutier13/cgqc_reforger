//! The RHS hearing protection indicator still lights up on a level change, then fades out completely
//! instead of staying dimmed on screen.
modded class RHS_HearingProtectionDisplay
{
	//------------------------------------------------------------------------------------------------
	override protected void OnStartDraw(IEntity owner)
	{
		super.OnStartDraw(owner);

		// Set here to win over the attribute value of the RHS player controller prefab
		m_fIdleOpacity = 0;
	}
}
