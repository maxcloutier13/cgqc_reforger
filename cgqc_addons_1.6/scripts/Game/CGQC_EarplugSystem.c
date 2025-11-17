modded class PREFIX_EarplugSystem extends WorldSystem {
    //------------------------------------------------------------------------------------------------
    override void OnInit() {
        super.OnInit();
        
        // Force the earplug volume to 50% after it's loaded
        BaseContainer earplugSettings = GameUserSettings.GetModule("PREFIX_EarplugSettings");
        if (earplugSettings) {
            earplugSettings.Set("EarplugsVolume", 50);
        }
        
        // Refresh the value
        EarplugsVolume = FetchEarplugsVolume();
    }
}