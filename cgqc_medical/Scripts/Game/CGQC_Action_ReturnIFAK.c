// -----------------------------------------------------------------------
// CGQC_ReturnAction
//
// ScriptedUserAction added to every character prefab alongside CGQC_BorrowAction.
// Visible only to the player who currently has an active borrow on this casualty,
// allowing them to manually return the kit instead of walking away or waiting.
// -----------------------------------------------------------------------
class CGQC_ReturnAction : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		return CanReturn(user);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanReturn(user);
	}

	protected bool CanReturn(IEntity user)
	{
		IEntity casualty = GetOwner();
		if (!casualty || !user)
			return false;

		if (user == casualty)
			return false;

		if (CGQC_MedCore.IsActorDead(casualty))
			return false;

		CGQC_CasualtyComponent casualtyComp = CGQC_CasualtyComponent.Cast(
			casualty.FindComponent(CGQC_CasualtyComponent)
		);

		if (!casualtyComp)
			return false;

		// Only show to the specific player who has an active borrow on this casualty.
		return casualtyComp.HasActiveBorrowByEntity(user);
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		IEntity casualty = pOwnerEntity;
		IEntity borrower = pUserEntity;

		if (!casualty || !borrower)
			return;

		CGQC_CasualtyComponent casualtyComp = CGQC_CasualtyComponent.Cast(
			casualty.FindComponent(CGQC_CasualtyComponent)
		);

		if (!casualtyComp)
			return;

		casualtyComp.RequestReturn(borrower);
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = "Remettre le IFAK";
		return true;
	}
}