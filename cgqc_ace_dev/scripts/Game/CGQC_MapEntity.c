/* Writes declination. Unnecessary since ACE added their own
modded class SCR_MapEntity
{
    override protected void OnMapOpen(MapConfiguration config)
    {
        super.OnMapOpen(config);
        
        #ifdef ACE_COMPASS
        ShowDeclinationHint();
        #endif
    }

    #ifdef ACE_COMPASS
    protected void ShowDeclinationHint()
    {
        ChimeraWorld world = GetGame().GetWorld();
        if (!world)
            return;
        
        TimeAndWeatherManagerEntity twm = world.GetTimeAndWeatherManager();
        if (!twm)
            return;
        
        float declination = twm.ACE_GetMagneticDeclination();
        
        float absDecl = Math.AbsFloat(declination);
        int degrees = (int)absDecl;
        int hundredths = Math.Round((absDecl - degrees) * 100);
        
        string centStr;
        if (hundredths < 10)
            centStr = "0" + hundredths;
        else
            centStr = "" + hundredths;
        
        string sign;
        if (declination >= 0)
            sign = "+";
        else
            sign = "-";
        
        string text = string.Format(
            "Déclinaison magnétique: %1%2.%3°",
            sign,
            degrees,
            centStr
        );
        
        SCR_HintManagerComponent.ShowCustomHint(text, "", 5.0);
    }
    #endif
}*/