//! High intensity (HI) Green and Red modes for the RHS Hel-Star.
//! Each HI mode uses an ELightType the original Hel-Star leaves free; the slots are defined in the
//! cgqcrhs override of Helstar.et. Mode selection and replication are left to RHS.
modded class RHS_HelstarComponent
{
	protected static const ELightType CGQC_GREEN_HI = ELightType.SearchLight;
	protected static const ELightType CGQC_GREEN_HI_FLASH = ELightType.Dashboard;
	protected static const ELightType CGQC_RED_HI = ELightType.HiBeam;
	protected static const ELightType CGQC_RED_HI_FLASH = ELightType.Presence;

	protected static const int CGQC_FLASH_ON_MS = 150;
	protected static const int CGQC_FLASH_OFF_MS = 850;

	//! Casing glow strength in HI modes
	protected static const int CGQC_CASING_EMISSIVE = 1;

	//------------------------------------------------------------------------------------------------
	override void StrobeEffect(bool pNewState = true)
	{
		if (!m_bIsTurnedOn)
		{
			CGQC_SetCasingGlow(false);
			return;
		}

		// A flash call still queued from the previous mode must not switch off the new one
		GetGame().GetCallqueue().Remove(StrobeEffect);

		bool isGreen = m_eSelectedLightType == CGQC_GREEN_HI || m_eSelectedLightType == CGQC_GREEN_HI_FLASH;
		bool isRed = m_eSelectedLightType == CGQC_RED_HI || m_eSelectedLightType == CGQC_RED_HI_FLASH;

		if (!isGreen && !isRed)
		{
			super.StrobeEffect(pNewState);

			// The original modes have no casing glow
			CGQC_SetCasingGlow(false);
			return;
		}

		bool isFlashing = m_eSelectedLightType == CGQC_GREEN_HI_FLASH || m_eSelectedLightType == CGQC_RED_HI_FLASH;
		if (!isFlashing)
			pNewState = true;

		ChangeLightVisibilityByType(m_eSelectedLightType, pNewState);

		int color = Color.RED;
		if (isGreen)
			color = Color.GREEN;

		CGQC_SetCasingGlow(pNewState, color);

		if (!isFlashing)
			return;

		int timeDelay = CGQC_FLASH_ON_MS;
		if (!pNewState)
			timeDelay = CGQC_FLASH_OFF_MS;

		GetGame().GetCallqueue().CallLater(StrobeEffect, timeDelay, param1: !pNewState);
	}

	//------------------------------------------------------------------------------------------------
	override void SwitchDeviceState(bool pNewState = false, bool pPlaySound = true)
	{
		super.SwitchDeviceState(pNewState, pPlaySound);

		if (!m_bIsTurnedOn)
			CGQC_SetCasingGlow(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void CGQC_SetCasingGlow(bool state, int color = Color.BLACK)
	{
		if (!m_ParamMatInstComponent)
		{
			m_ParamMatInstComponent = ParametricMaterialInstanceComponent.Cast(GetOwner().FindComponent(ParametricMaterialInstanceComponent));
			if (!m_ParamMatInstComponent)
				return;
		}

		if (!state)
		{
			m_ParamMatInstComponent.SetEmissiveMultiplier(0);
			return;
		}

		m_ParamMatInstComponent.SetEmissiveColor(color);
		m_ParamMatInstComponent.SetEmissiveMultiplier(CGQC_CASING_EMISSIVE);
	}
}
