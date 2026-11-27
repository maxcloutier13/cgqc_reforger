// Gestion d'unité - visual identity (spec §2). Colours are given in sRGB, as in the mockup,
// and converted to the engine's linear space by Color.FromSRGBA.

class CGQC_GU_Style
{
	static const ResourceName LAYOUT_BUTTON = "{4B7C7538BC40D683}UI/layouts/CGQC_GU_Button.layout";
	static const ResourceName LAYOUT_PAPER_SHELL = "{1CC44738E0579BA8}UI/layouts/CGQC_GU_PaperShell.layout";
	static const ResourceName LAYOUT_TEXT = "{BA879D12B15AC857}UI/layouts/CGQC_GU_Text.layout";
	static const ResourceName LAYOUT_VBOX = "{3F2645D4E313E40A}UI/layouts/CGQC_GU_VBox.layout";
	static const ResourceName LAYOUT_HBOX = "{78BFD66710562FBF}UI/layouts/CGQC_GU_HBox.layout";
	static const ResourceName LAYOUT_SIZED = "{19FAFB32D5A0F210}UI/layouts/CGQC_GU_Sized.layout";
	static const ResourceName LAYOUT_CANVAS = "{58401A6B0C8EF961}UI/layouts/CGQC_GU_Canvas.layout";
	static const ResourceName LAYOUT_RECT = "{9023B45C8EC97503}UI/layouts/CGQC_GU_Rect.layout";
	static const ResourceName LAYOUT_PANEL = "{9EB86D476345F06B}UI/layouts/CGQC_GU_Panel.layout";
	static const ResourceName LAYOUT_FIELD = "{124F7C180CFF788E}UI/layouts/CGQC_GU_Field.layout";
	static const ResourceName LAYOUT_FIELD_MULTI = "{850ECED9FE9A92B5}UI/layouts/CGQC_GU_FieldMulti.layout";
	static const ResourceName LAYOUT_CHECK = "{4291E7CFA9DE912F}UI/layouts/CGQC_GU_Check.layout";
	static const ResourceName LAYOUT_SCROLL = "{2DEC8C574D490DD4}UI/layouts/CGQC_GU_Scroll.layout";

	// Fonts available in the base game (owner decision: closest fonts for now).
	static const ResourceName FONT_TITLE = "{EABA4FE9D014CCEF}UI/Fonts/RobotoCondensed/RobotoCondensed_Bold.fnt";
	static const ResourceName FONT_LABEL = "{3E7733BAC8C831F6}UI/Fonts/RobotoCondensed/RobotoCondensed_Regular.fnt";
	static const ResourceName FONT_VALUE = "{E2CBA6F76AAA42AF}UI/Fonts/Roboto/Roboto_Regular.fnt";

	static const string CONFIRM_TEXT = "APPUYER DE NOUVEAU";
	static const float CONFIRM_TIMEOUT = 3.0;	// seconds before an armed button resets

	//------------------------------------------------------------------------------------------------
	// Accent
	static Color Blue()			{ return Color.FromSRGBA(0x23, 0x43, 0x9C, 255); }	// CGQC blue #23439C
	static Color BlueLight()	{ return Color.FromSRGBA(0x5B, 0x82, 0xE0, 255); }	// #5B82E0 on dark
	static Color BlueLighter()	{ return Color.FromSRGBA(0x8E, 0xAA, 0xF0, 255); }	// #8EAAF0 on dark
	static Color Red()			{ return Color.FromSRGBA(0x8E, 0x2A, 0x22, 255); }	// refusals / retakes #8E2A22
	static Color ConfirmRed()	{ return Color.FromSRGBA(0x9E, 0x2A, 0x26, 255); }	// armed double-press

	// Paper
	static Color Parchment()	{ return Color.FromSRGBA(0xC9, 0xBF, 0xA6, 255); }	// toned down from the mockup #E8DCBB (owner, end of Phase 2: too bright)
	static Color Ink()			{ return Color.FromSRGBA(0x1C, 0x17, 0x0D, 255); }	// titles
	static Color InkSoft()		{ return Color.FromSRGBA(0x2A, 0x24, 0x18, 255); }	// body
	static Color InkMuted()		{ return Color.FromSRGBA(0x52, 0x48, 0x3A, 255); }	// captions
	static Color Rule()			{ return Color.FromSRGBA(0x3A, 0x31, 0x23, 255); }	// ruled boxes #3A3123
	static Color PaperButton()	{ return Translucent(0x3A, 0x31, 0x23, 0.16); }	// plain button on paper
	static Color PaperText()	{ return Color.FromSRGBA(0xF4, 0xED, 0xD9, 255); }	// text on blue / red
	static Color Field()		{ return Color.FromSRGBA(0xD9, 0xD1, 0xBC, 255); }	// edit field on paper (was #F3EBD3)
	static Color Clear()		{ Color c = Color.FromSRGBA(0, 0, 0, 255); c.SetA(0); return c; }
	static Color StatusYellow()	{ return Color.FromSRGBA(0xB0, 0x86, 0x0E, 255); }	// INACTIF, dark enough for parchment

	// Catalogue lists (mockup)
	static Color RowEdge()		{ return Color.FromSRGBA(0x8B, 0x7B, 0x5A, 255); }	// table and list rows #8B7B5A
	static Color RowSelected()	{ return Translucent(0x23, 0x43, 0x9C, 0.15); }	// chosen row
	static Color PaperLight()	{ return Translucent(0xFF, 0xFA, 0xEB, 0.35); }	// detail box
	static Color BadgeDim()		{ return Color.FromSRGBA(0x7A, 0x6E, 0x55, 255); }	// badge « à venir » #7A6E55

	static Color PpColor(int pp)
	{
		switch (pp)
		{
			case 1: return Color.FromSRGBA(0x2F, 0x7A, 0x55, 255);	// PP1 #2F7A55
			case 2: return Color.FromSRGBA(0x5B, 0x4F, 0xA8, 255);	// PP2 #5B4FA8
		}

		return Color.FromSRGBA(0xB5, 0x70, 0x1D, 255);	// PP3 #B5701D
	}

	//! Member status colour code (owner): ACTIF green, INACTIF yellow, MIA red, KIA black.
	static Color StatusColor(string status)
	{
		switch (status)
		{
			case "actif":	return Online();
			case "inactif":	return StatusYellow();
			case "mia":		return Red();
			case "kia":		return Ink();
		}

		return InkSoft();
	}

	static Color Online()		{ return Color.FromSRGBA(0x3E, 0x7B, 0x3A, 255); }	// online dot / LIÉ
	static Color Offline()		{ return Color.FromSRGBA(0x9A, 0x8F, 0x7A, 255); }
	static Color Slip()			{ return Color.FromSRGBA(0x44, 0x47, 0x33, 255); }	// rank slip-on (olive)
	static Color SlipMark()		{ return Color.FromSRGBA(0xD9, 0xCB, 0x94, 255); }	// embroidery on the slip-on
	static Color Gorget()		{ return Color.FromSRGBA(0xB0, 0x22, 0x22, 255); }	// colonel's scarlet

	// Dark
	static Color DarkPanel()	{ return Translucent(0x16, 0x19, 0x1D, 0.65); }	// mockup .55, a bit more for bright terrain
	static Color PhaseTag()		{ return Translucent(0x3D, 0x4A, 0x5C, 0.8); }	// rgba(61,74,92,.8)
	static Color NoticeBlue()	{ return Translucent(0x5B, 0x82, 0xE0, 0.92); }	// rgba(91,130,224,.92)
	static Color White()		{ return Color.FromSRGBA(255, 255, 255, 255); }
	static Color DarkText()		{ return Color.FromSRGBA(0xEC, 0xED, 0xEE, 255); }	// #ECEDEE
	static Color DarkMuted()	{ return Color.FromSRGBA(0xA3, 0xA7, 0xAA, 255); }	// #A3A7AA

	//------------------------------------------------------------------------------------------------
	//! sRGB colour with an opacity as a browser would show it. The UI blends in linear space, where a dark
	//! veil darkens far less than the same opacity in sRGB: the opacity is converted so (1 - a) matches.
	static Color Translucent(int r, int g, int b, float opacity)
	{
		Color c = Color.FromSRGBA(r, g, b, 255);
		c.SetA(1 - Math.Pow(1 - opacity, 2.2));
		return c;
	}

	static const ResourceName LOGO = "{CCB8D746EACD6C7F}UI/Textures/CGQC_GU/CGQC_GU_Logo.edds";
	static const ResourceName BACKGROUND = "{3394876F197C7A18}UI/Textures/CGQC_GU/CGQC_GU_Background.edds";	// owner's 1920×1080
	static const ResourceName LAYOUT_DARK_SHELL = "{B0C2202C98D29830}UI/layouts/CGQC_GU_DarkShell.layout";

	// Dark screens (Gestion d'unité menu, Situation, Calendrier)
	// Values from the approved mockup (NavGU, Situation, Calendrier).
	// Opacities lowered twice on the owner's request (2c, then 2e: "more transparency all around").
	static Color DarkVeil()			{ return Translucent(0x0A, 0x0D, 0x12, 0.55); }	// mockup gradient .45 → .8
	static Color DarkNav()			{ return Translucent(0x18, 0x1B, 0x1F, 0.66); }	// mockup .88
	static Color DarkCard()			{ return Translucent(0x1A, 0x1D, 0x21, 0.45); }	// mockup .86
	static Color DarkTile()			{ return Translucent(0x2C, 0x30, 0x35, 0.5); }	// mockup .85
	static Color DarkActiveCard()	{ return Translucent(0x1E, 0x2A, 0x4A, 0.65); }	// current phase / today
	static Color DarkButton()		{ return Color.FromSRGBA(0x3A, 0x3E, 0x40, 255); }	// #3A3E40
	static Color DarkOnBlue()		{ return Color.FromSRGBA(0x1E, 0x1E, 0x1E, 255); }	// text on #5B82E0
	static Color DarkLine()			{ return Color.FromSRGBA(0x4A, 0x4E, 0x50, 255); }	// separators #4A4E50
	static Color DarkSoft()			{ return Color.FromSRGBA(0xC9, 0xCB, 0xCD, 255); }	// body text #C9CBCD
	static Color DarkDim()			{ return Color.FromSRGBA(0x6C, 0x70, 0x74, 255); }	// entries of later phases
	static Color Training()			{ return Color.FromSRGBA(0x4E, 0x7A, 0x3A, 255); }	// calendar: training / FTX
}
