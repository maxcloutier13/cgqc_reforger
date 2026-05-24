//------------------------------------------------------------------------------------------------
// CGQC Player Roster Config
// Define rank entries in Workbench via Config Editor.
// Each rank has its own .conf (CGQC_Roster_Soldats.conf, etc.)
// Drop rank conf GUIDs into CGQC_Roster.conf via m_aRankRosters.
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
class CGQC_RankRosterConfig
{
	[Attribute(desc: "Players in this rank group")]
	ref array<ref CGQC_PlayerEntry> m_aPlayers;
}

[BaseContainerProps(configRoot: true)]
class CGQC_PlayerRosterConfig
{
	[Attribute("", UIWidgets.Object, "Per-rank player lists")]
	ref array<ref CGQC_RankRosterConfig> m_aRankRosters;
}