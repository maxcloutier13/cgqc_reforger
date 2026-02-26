modded class CSI_SettingsManager : ScriptComponent
{	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		
		if (RplSession.Mode() != RplMode.Dedicated)
            return;
		
		UpdateAuthoritySetting(CSI_GameSettings.GROUP_IN_NAMETAG_VISIBLE, 0, true);
        UpdateAuthoritySetting(CSI_GameSettings.BEARING_VISIBLE, 0, true);
        UpdateAuthoritySetting(CSI_GameSettings.NAMETAG_RANGE, 5, true);
		UpdateAuthoritySetting(CSI_GameSettings.ROLE_IN_NAMETAG_VISIBLE, 0, true);
	}
}

/* Reference

	const static string COMPASS_VISIBLE = "m_bCompassVisible";
	const static string BEARING_VISIBLE = "m_bBearingVisible";
	const static string RADAR_VISIBLE = "m_bRadarVisible";
	const static string ONLY_RADAR_ICON_ARROWS_ROTATE = "m_bOnlyRadarIconArrowsRotate";
	const static string GROUP_VISIBLE = "m_bGroupVisible";
	const static string STAMINA_VISIBLE = "m_bStaminaVisible";
	const static string NAMETAG_VISIBLE = "m_bNametagVisible";
	const static string RANK_VISIBLE = "m_bRankVisible";
	const static string ROLE_IN_NAMETAG_VISIBLE = "m_bRoleIconInNametagVisible";
	const static string GROUP_IN_NAMETAG_VISIBLE = "m_bGroupInNametagVisible";
	const static string NAMETAG_LOS_VISIBLE = "m_bNametagLOSVisible";
	const static string AUTO_HIDE_HUD = "m_bAutoHideHUD";
	const static string ICON_THEME = "m_iIconTheme";
	const static string ICON_TYPE = "m_iIconType";
	const static string ARROW_THEME = "m_iArrowTheme";
	const static string COMPASS_THEME = "m_iCompassTheme";
	const static string NAMETAG_POSITION = "m_iNametagPosition";
	const static string NAMETAG_ROLE_ICON_POSITION = "m_iNametagRoleIconPosition";
	const static string NAMETAG_POSITION_OFFSET = "m_iNametagPositionOffset";
	const static string NAMETAG_RANGE = "m_iNametagRange";
	const static string NAMETAG_MAGNIFICATION_MULTIPLICATION = "m_iNametagMagnificationMultiplication";
	const static string RADAR_ICON_SIZE = "m_iRadarIconSize";
	
*/