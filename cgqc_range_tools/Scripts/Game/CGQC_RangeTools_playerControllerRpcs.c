// ============================================================
// CGQC_RangeTools_PlayerControllerRPCs.c
//
// Single modded SCR_PlayerController block for all CGQC
// RangeTools RPCs. Keep all RPC declarations here to avoid
// multiple modded class blocks for the same class across files.
//
// RPCs defined here:
//   CGQC_Rpc_AskCheck          – Client → Server
//   CGQC_Rpc_AskReset          – Client → Server
//   CGQC_Rpc_ShowTargetReport  – Server → Owner  (IPSC + Zero shared)
//   CGQC_Rpc_ShowResetHint     – Server → Owner  (IPSC + Zero + Mover shared)
//   CGQC_Rpc_ShowHitHint       – Server → Owner  (IPSC target)
//   CGQC_Rpc_ShowZeroHitHint   – Server → Owner  (Zero target)
//   CGQC_Rpc_ShowMoverHint     – Server → Owner  (Target mover distance/check)
// ============================================================

modded class SCR_PlayerController
{
	// ----------------------------------------------------------------
	// Client → Server: request Check on an IPSC target by RplId
	// ----------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void CGQC_Rpc_AskCheck(RplId targetId)
	{
		IEntity targetEnt = IEntity.Cast(Replication.FindItem(targetId));
		if (!targetEnt) return;

		CGQC_RangeTools_TargetHitComponent hitComp =
			CGQC_RangeTools_TargetHitComponent.Cast(
				targetEnt.FindComponent(CGQC_RangeTools_TargetHitComponent));
		if (!hitComp) return;

		int pid = -1;
		IEntity controlled = GetControlledEntity();
		if (controlled)
			pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(controlled);

		if (pid > 0)
			hitComp.Server_Check(pid);
	}

	// ----------------------------------------------------------------
	// Client → Server: request Reset on an IPSC target by RplId
	// ----------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void CGQC_Rpc_AskReset(RplId targetId)
	{
		IEntity targetEnt = IEntity.Cast(Replication.FindItem(targetId));
		if (!targetEnt) return;

		CGQC_RangeTools_TargetHitComponent hitComp =
			CGQC_RangeTools_TargetHitComponent.Cast(
				targetEnt.FindComponent(CGQC_RangeTools_TargetHitComponent));
		if (!hitComp) return;

		int pid = -1;
		IEntity controlled = GetControlledEntity();
		if (controlled)
			pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(controlled);

		hitComp.Server_Reset(pid);
	}

	// ----------------------------------------------------------------
	// Server → Owner: show full target report (IPSC and Zero shared)
	// ----------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void CGQC_Rpc_ShowTargetReport(string reportText, string reportTitle)
	{
		Print("[CGQC_Check] " + reportTitle + "\n" + reportText, LogLevel.NORMAL);
		CGQC_BasicDisplay.Show(reportTitle, reportText, 15.0);
	}

	// ----------------------------------------------------------------
	// Server → Owner: reset confirmation (IPSC, Zero, and Mover shared)
	// ----------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void CGQC_Rpc_ShowResetHint()
	{
		CGQC_BasicDisplay.Show("Reset", "Cible remise a zero.", 2.5);
	}

	// ----------------------------------------------------------------
	// Server → Owner: per-hit hint for IPSC target (distance-gated)
	// ----------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void CGQC_Rpc_ShowHitHint(int hitNum, string zone, int points, int totalPoints,
	                           int distM, int velMs, bool isFull)
	{
		SCR_HintManagerComponent hm = SCR_HintManagerComponent.GetInstance();
		if (!hm) return;

		string title;
		float duration;
		if (isFull)
		{
			title    = "Impact! - Cible pleine";
			duration = 4.0;
		}
		else if (hitNum == 0)
		{
			title    = "Impact!";
			duration = 2.5;
		}
		else
		{
			title    = "Impact! Hit " + hitNum.ToString();
			duration = 2.5;
		}

		string body = "Distance: " + distM.ToString() + "m";
		body = body + "\nVelocite: " + velMs.ToString() + " m/s";
		body = body + "\nZone: " + zone + " | " + points.ToString() + " pts";
		body = body + "\nTotal: " + totalPoints.ToString() + "/" + (hitNum * 5).ToString() + " pts";

		CGQC_BasicDisplay.Show(title, body, duration);
	}

	// ----------------------------------------------------------------
	// Server → Owner: per-hit hint for Zero target
	// ----------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void CGQC_Rpc_ShowZeroHitHint(int hitNum, string mradStr, string cmStr, int distM, int velMs)
	{
		string title = "Impact! #" + hitNum.ToString();

		string body = "Distance: " + distM.ToString() + "m";
		body = body + "\nVelocite: " + velMs.ToString() + " m/s";
		body = body + "\nOffset: " + mradStr;
		body = body + "\n        " + cmStr;

		CGQC_BasicDisplay.Show(title, body, 3.0);
	}

	// ----------------------------------------------------------------
	// Server → Owner: quick hint for target mover distance/check actions
	// ----------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void CGQC_Rpc_ShowMoverHint(string title, string body)
	{
		CGQC_BasicDisplay.Show(title, body, 3.0);
	}
}