//! Removes the RHS weapon light/laser indicator (e.g. "AN/PEQ-16") from the bottom right of the HUD.
//! The device keys are handled elsewhere by RHS and keep working.
modded class RHS_WeaponDeviceDisplay
{
	//------------------------------------------------------------------------------------------------
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		// No super call: the RHS method is what shows the indicator
		if (m_wRoot)
			m_wRoot.SetVisible(false);
	}
}
