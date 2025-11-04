modded class SCR_VonDisplay
{
	//------------------------------------------------------------------------------------------------
	// Hide the entire VON display on startup
	override void DisplayStartDraw(IEntity owner)
	{
		super.DisplayStartDraw(owner);
		
		// Hide the root widget = hides everything
		if (m_wRoot)
			m_wRoot.SetVisible(false);
	}
	
	//------------------------------------------------------------------------------------------------
	// Keep it hidden during updates
	override void DisplayUpdate(IEntity owner, float timeSlice)
	{
		// Keep the root hidden
		if (m_wRoot)
			m_wRoot.SetVisible(false);
		
		// Still call super to keep internal state working
		super.DisplayUpdate(owner, timeSlice);
	}
	
	//------------------------------------------------------------------------------------------------
	// Hide on entity change too
	override void DisplayControlledEntityChanged(IEntity from, IEntity to)
	{
		super.DisplayControlledEntityChanged(from, to);
		
		if (m_wRoot)
			m_wRoot.SetVisible(false);
	}
}