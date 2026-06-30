// ============================================================
// CGQC_EquipCheck_PlayerControllerRPCs.c  (cgqc_scripts)
// Adds equipment check display RPC to SCR_PlayerController.
// Fires on the owning client — both TL and soldier receive it.
// ============================================================

modded class SCR_PlayerController
{
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void CGQC_RpcDo_ShowInspection(string title, string body)
	{
		CGQC_BasicDisplay.Show(title, body, 20.0);
	}
}