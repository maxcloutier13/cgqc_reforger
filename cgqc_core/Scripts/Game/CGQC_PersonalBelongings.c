//------------------------------------------------------------------------------------------------
// CGQC Personal Belongings
// Writes the player's roster data (CGQC_PlayerEntry: name, birth date, place of birth, blood type,
// information) into the character's real identity data, so every UI that reads it agrees
// (inventory tooltip, GM tooltips, belongings on a dead body, other mods).
//
// Vanilla only replicates a random seed + bio index and each client re-rolls the values, so the
// server sends the roster values to everyone (RPC + RplSave/RplLoad for JIP) and each machine
// applies them to its local copy. Vanilla randomization only fills unset values, so the order
// between the two doesn't matter.
// Fields left empty / 0 / AUTO in the roster keep the vanilla value.
//------------------------------------------------------------------------------------------------

modded class SCR_IdentityBio
{
	//------------------------------------------------------------------------------------------------
	void CGQC_SetBioText(string bio)
	{
		m_sBio = bio;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_ExtendedCharacterIdentityComponent
{
	//! Shown for roster fields left empty
	static const string CGQC_REDACTED = "REDACTED";

	protected bool m_bCGQC_HasBio;
	protected string m_sCGQC_Name;
	protected int m_iCGQC_BirthDay;
	protected int m_iCGQC_BirthMonth;
	protected int m_iCGQC_BirthYear;
	protected string m_sCGQC_Place;
	protected int m_iCGQC_BloodType = SCR_EBloodType.AUTO;
	protected string m_sCGQC_Bio;

	//------------------------------------------------------------------------------------------------
	//! Server only. Pushes the roster entry's personal belongings data to everyone.
	void CGQC_SetRosterBio(notnull CGQC_PlayerEntry entry)
	{
		if (!Replication.IsServer())
			return;

		// Year is sent as 0-99, each machine resolves the century against the mission date. -1 = no date.
		int yearOfCentury = -1, month, day;
		if (!entry.GetBirthDate(yearOfCentury, month, day))
		{
			yearOfCentury = -1;
			month = 0;
			day = 0;
		}

		PrintFormat("[CGQC_PersonalBelongings] %1: name '%2', birth '%3', place '%4', blood %5, bio '%6'", entry.m_sLabel, entry.m_sBioName, entry.m_sBirthDate, entry.m_sPlaceOfBirth, entry.m_iBloodType, entry.m_sBio);

		CGQC_SetBioBroadcast(entry.m_sBioName, day, month, yearOfCentury, entry.m_sPlaceOfBirth, entry.m_iBloodType, entry.m_sBio);
		Rpc(CGQC_SetBioBroadcast, entry.m_sBioName, day, month, yearOfCentury, entry.m_sPlaceOfBirth, entry.m_iBloodType, entry.m_sBio);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void CGQC_SetBioBroadcast(string name, int day, int month, int year, string place, int bloodType, string bio)
	{
		m_bCGQC_HasBio = true;
		m_sCGQC_Name = name.Trim();
		m_iCGQC_BirthDay = day;
		m_iCGQC_BirthMonth = month;
		m_iCGQC_BirthYear = year;
		m_sCGQC_Place = place.Trim();
		m_iCGQC_BloodType = bloodType;
		m_sCGQC_Bio = bio.Trim();

		CGQC_ApplyBio();
	}

	//------------------------------------------------------------------------------------------------
	//! Writes the stored roster values into this machine's identity data
	protected void CGQC_ApplyBio()
	{
		SCR_ExtendedCharacterIdentity identity = SCR_ExtendedCharacterIdentity.Cast(m_ExtendedIdentity);
		if (!identity)
			return;

		if (m_iCGQC_BirthDay > 0)
			identity.SetDayOfCreation(m_iCGQC_BirthDay);

		if (m_iCGQC_BirthMonth > 0)
			identity.SetMonthOfCreation(m_iCGQC_BirthMonth);

		// The game stores an age, not a year, and shows "mission year - age" as the birth year.
		// m_iCGQC_BirthYear is YY (0-99): take the latest century that makes the character at least 16.
		if (m_iCGQC_BirthYear >= 0)
		{
			int curYear = CGQC_GetMissionYear();
			if (curYear > 0)
			{
				int birthYear = (curYear / 100) * 100 + m_iCGQC_BirthYear;
				while (birthYear > curYear - 16)
					birthYear -= 100;

				identity.SetAge(curYear - birthYear);
			}
		}

		if (m_iCGQC_BloodType != SCR_EBloodType.AUTO)
			identity.SetBloodType(m_iCGQC_BloodType);

		// Empty roster text fields show REDACTED instead of a random value
		string place = m_sCGQC_Place;
		if (place.IsEmpty())
			place = CGQC_REDACTED;

		identity.SetPlaceOfOrigin(SCR_UIInfo.CreateInfo(place), true);

		string bioText = m_sCGQC_Bio;
		if (bioText.IsEmpty())
			bioText = CGQC_REDACTED;

		SCR_IdentityBio identityBio = new SCR_IdentityBio();
		identityBio.CGQC_SetBioText(bioText);
		AssignBio(identityBio);

		CGQC_ApplyName();
	}

	//------------------------------------------------------------------------------------------------
	//! Character identity name = roster name (no alias / surname)
	protected void CGQC_ApplyName()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		CharacterIdentityComponent characterIdentity = CharacterIdentityComponent.Cast(owner.FindComponent(CharacterIdentityComponent));
		if (!characterIdentity)
			return;

		Identity identity = characterIdentity.GetIdentity();
		if (!identity)
			return;

		identity.SetName(CGQC_GetName());
		identity.SetAlias(string.Empty);
		identity.SetSurname(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	protected int CGQC_GetMissionYear()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return -1;

		TimeAndWeatherManagerEntity timeManager = world.GetTimeAndWeatherManager();
		if (!timeManager)
			return -1;

		int year, month, day;
		timeManager.GetDate(year, month, day);
		return year;
	}

	//------------------------------------------------------------------------------------------------
	//! Roster name, REDACTED if the roster doesn't set one
	string CGQC_GetName()
	{
		if (m_sCGQC_Name.IsEmpty())
			return CGQC_REDACTED;

		return m_sCGQC_Name;
	}

	//------------------------------------------------------------------------------------------------
	//! True once roster data was received for this character
	bool CGQC_HasBio()
	{
		return m_bCGQC_HasBio;
	}

	//------------------------------------------------------------------------------------------------
	bool CGQC_HasBirthDate()
	{
		return m_iCGQC_BirthYear >= 0 && m_iCGQC_BirthMonth > 0 && m_iCGQC_BirthDay > 0;
	}

	//------------------------------------------------------------------------------------------------
	bool CGQC_HasBloodType()
	{
		return m_iCGQC_BloodType != SCR_EBloodType.AUTO;
	}

	//------------------------------------------------------------------------------------------------
	override bool RplSave(ScriptBitWriter writer)
	{
		if (!super.RplSave(writer))
			return false;

		writer.WriteBool(m_bCGQC_HasBio);
		if (!m_bCGQC_HasBio)
			return true;

		writer.WriteString(m_sCGQC_Name);
		writer.WriteInt(m_iCGQC_BirthDay);
		writer.WriteInt(m_iCGQC_BirthMonth);
		writer.WriteInt(m_iCGQC_BirthYear);
		writer.WriteString(m_sCGQC_Place);
		writer.WriteInt(m_iCGQC_BloodType);
		writer.WriteString(m_sCGQC_Bio);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool RplLoad(ScriptBitReader reader)
	{
		if (!super.RplLoad(reader))
			return false;

		bool hasBio;
		reader.ReadBool(hasBio);
		if (!hasBio)
			return true;

		string name, place, bio;
		int day, month, year, bloodType;
		reader.ReadString(name);
		reader.ReadInt(day);
		reader.ReadInt(month);
		reader.ReadInt(year);
		reader.ReadString(place);
		reader.ReadInt(bloodType);
		reader.ReadString(bio);

		CGQC_SetBioBroadcast(name, day, month, year, place, bloodType, bio);
		return true;
	}
}

//------------------------------------------------------------------------------------------------
//! The tooltip shows the player's platform name before the identity name, so the roster name
//! has to replace it here. Same for REDACTED on an empty birth date / blood type (numbers in the
//! identity data). Everything else comes from the identity data.
modded class SCR_InventoryIdentityItemWidgetComponent
{
	//------------------------------------------------------------------------------------------------
	override void SetIdentityData(notnull SCR_IdentityInventoryItemComponent identityItemComponent, notnull SCR_ExtendedIdentityComponent linkedIdentity, SCR_IdentityBio identityBio)
	{
		super.SetIdentityData(identityItemComponent, linkedIdentity, identityBio);

		SCR_ExtendedCharacterIdentityComponent charIdentity = SCR_ExtendedCharacterIdentityComponent.Cast(linkedIdentity);
		if (!charIdentity || !charIdentity.CGQC_HasBio())
			return;

		Widget holder = GetRootWidget().FindAnyWidget(m_sCharacterIdentityHolder);
		if (!holder)
			return;

		CGQC_SetText(holder, m_sNameHolder, m_sNameText, charIdentity.CGQC_GetName());

		// Date and blood type are numbers in the identity data, so REDACTED can only be shown here
		if (!charIdentity.CGQC_HasBirthDate())
		{
			CGQC_SetText(holder, m_sCreationHolder, m_sCreationDateText, SCR_ExtendedCharacterIdentityComponent.CGQC_REDACTED);

			Widget birthdayIcon = holder.FindAnyWidget(m_sBirthdayIcon);
			if (birthdayIcon)
				birthdayIcon.SetVisible(false);
		}

		if (!charIdentity.CGQC_HasBloodType())
			CGQC_SetText(holder, m_sBloodTypeHolder, m_sBloodTypeText, SCR_ExtendedCharacterIdentityComponent.CGQC_REDACTED);
	}

	//------------------------------------------------------------------------------------------------
	protected void CGQC_SetText(notnull Widget holder, string holderName, string textName, string text)
	{
		Widget subHolder = holder.FindAnyWidget(holderName);
		if (!subHolder)
			return;

		TextWidget textWidget = TextWidget.Cast(subHolder.FindAnyWidget(textName));
		if (!textWidget)
			return;

		textWidget.SetText(text);
		textWidget.SetVisible(true);
		subHolder.SetVisible(true);
	}
}
