//------------------------------------------------------------------------------------------------
// CGQC Player Roster Config
// Define entries in Workbench via Config Editor, then drop the .conf GUID into CGQC_Scripts.
//------------------------------------------------------------------------------------------------

[BaseContainerProps()]
class CGQC_PlayerEntry
{
	[Attribute(defvalue: "", desc: "Player's Bohemia/platform identity ID (UUID)")]
	string m_sIdentityId;

	[Attribute(defvalue: "", desc: "Human-readable label (for logs only)")]
	string m_sLabel;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Rank to assign", enums: ParamEnumArray.FromEnum(SCR_ECharacterRank))]
	int m_iRank;

	[Attribute(defvalue: "", desc: "Admin tag (e.g. 'clou', 'lafo'). Leave empty for no admin features.")]
	string m_sAdminTag;

	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, desc: "Custom head prefab (.et). Leave empty for default.", params: "et")]
	ResourceName m_sHeadPrefab;

	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, desc: "Custom body prefab (.et). Leave empty for default.", params: "et")]
	ResourceName m_sBodyPrefab;
	
	[Attribute(defvalue: "{F20A13563AFED116}Prefabs/Rectangle_Patches/spartan_camo.et", uiwidget: UIWidgets.ResourcePickerThumbnail, desc: "Custom Patch (.et). Leave empty for default.", params: "et")]
	ResourceName m_sPatch;
}

[BaseContainerProps(configRoot: true)]
class CGQC_PlayerRosterConfig
{
	[Attribute(desc: "List of all recognized CGQC players")]
	ref array<ref CGQC_PlayerEntry> m_aPlayers;
}