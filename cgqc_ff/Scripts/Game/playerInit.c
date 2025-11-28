modded class SCR_PlayerController : PlayerController
{
    override void OnControlledEntityChanged(IEntity from, IEntity to)
    {
        super.OnControlledEntityChanged(from, to);

        // Check if we're taking control of a new entity (player spawn)
        if (to)
        {
			Print("[CGQC_FF_TakingControl] Waiting a bit");
			int playerId = GetPlayerId();
            GetGame().GetCallqueue().CallLater(CGQC_Scripts.initializePlayer, 10000, false, to, playerId);
            GetGame().GetCallqueue().CallLater(CGQC_Scripts.initializeFFPlayer, 6000, false, to);
        }
    }
}