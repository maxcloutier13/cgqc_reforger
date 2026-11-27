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
	static Color Parchment()	{ return Color.FromSRGBA(0xE8, 0xDC, 0xBB, 255); }	// #E8DCBB
	static Color Ink()			{ return Color.FromSRGBA(0x1C, 0x17, 0x0D, 255); }	// titles
	static Color InkSoft()		{ return Color.FromSRGBA(0x2A, 0x24, 0x18, 255); }	// body
	static Color InkMuted()		{ return Color.FromSRGBA(0x52, 0x48, 0x3A, 255); }	// captions
	static Color Rule()			{ return Color.FromSRGBA(0x3A, 0x31, 0x23, 255); }	// ruled boxes #3A3123
	static Color PaperButton()	{ return Translucent(0x3A, 0x31, 0x23, 0.16); }	// plain button on paper
	static Color PaperText()	{ return Color.FromSRGBA(0xF4, 0xED, 0xD9, 255); }	// text on blue / red
	static Color Field()		{ return Color.FromSRGBA(0xF3, 0xEB, 0xD3, 255); }	// edit field on paper
	static Color Clear()		{ Color c = Color.FromSRGBA(0, 0, 0, 255); c.SetA(0); return c; }
	static Color StatusYellow()	{ return Color.FromSRGBA(0xB0, 0x86, 0x0E, 255); }	// INACTIF, dark enough for parchment

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
}
