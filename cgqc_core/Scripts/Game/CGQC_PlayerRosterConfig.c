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

	// Personal belongings (identity item). Empty / 0 / AUTO keeps the random vanilla value.
	[Attribute(defvalue: "", category: "Personal belongings", desc: "Name shown on personal belongings. Empty = REDACTED.")]
	string m_sBioName;

	[Attribute(defvalue: "", category: "Personal belongings", desc: "Birth date as YYMMDD (e.g. 640414 = 14 April 1964). Century is picked so the character is at least 16 at the mission date. Empty = random.")]
	string m_sBirthDate;

	[Attribute(defvalue: "", category: "Personal belongings", desc: "Place of birth (e.g. Montréal). Empty = random.")]
	string m_sPlaceOfBirth;

	[Attribute(defvalue: "-1", category: "Personal belongings", uiwidget: UIWidgets.ComboBox, desc: "Blood type. AUTO = random.", enums: ParamEnumArray.FromEnum(SCR_EBloodType))]
	int m_iBloodType;

	[Attribute(defvalue: "", category: "Personal belongings", uiwidget: UIWidgets.EditBoxMultiline, desc: "Information text. Empty = random vanilla bio.")]
	string m_sBio;

	//------------------------------------------------------------------------------------------------
	//! Parses m_sBirthDate (YYMMDD). yearOfCentury is 0-99, the century is resolved against the mission date.
	//! \return false if empty or invalid
	bool GetBirthDate(out int yearOfCentury, out int month, out int day)
	{
		string date = m_sBirthDate.Trim();
		if (date.Length() != 6)
		{
			if (!date.IsEmpty())
				PrintFormat("[CGQC] Roster %1: invalid birth date '%2', expected YYMMDD", m_sLabel, date);

			return false;
		}

		for (int i = 0; i < 6; i++)
		{
			int ascii = date.ToAscii(i);
			if (ascii < 48 || ascii > 57) // '0'-'9'
			{
				PrintFormat("[CGQC] Roster %1: invalid birth date '%2', expected YYMMDD", m_sLabel, date);
				return false;
			}
		}

		yearOfCentury = date.Substring(0, 2).ToInt();
		month = date.Substring(2, 2).ToInt();
		day = date.Substring(4, 2).ToInt();

		if (month < 1 || month > 12 || day < 1 || day > 31)
		{
			PrintFormat("[CGQC] Roster %1: invalid birth date '%2', expected YYMMDD", m_sLabel, date);
			return false;
		}

		return true;
	}
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