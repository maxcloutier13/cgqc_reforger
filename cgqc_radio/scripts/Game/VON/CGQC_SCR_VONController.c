modded class SCR_VONController
{
	////------------------------------------------------------------------------------------------------
	// Check si le call est pour désactiver le VON. Si oui: ignore
	
	/* Skip. Unneeded?
	override void SetVONProximityToggle (bool activate)
	{
        if (!activate)
        {
            // Prevent turning off: ignore
            return;
        }

        // If it's turning on, allow default behaviour
        super.SetVONProximityToggle(activate);
    }
	*/

	
	//------------------------------------------------------------------------------------------------
	//! Force les comms directe
	void ForceEnableVONDirectToggle()
	{
		SetVONProximityToggle(true);
	}
}
