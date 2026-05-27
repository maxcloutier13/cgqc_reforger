// -----------------------------------------------------------------------
// CGQC_BorrowAction
//
// ScriptedUserAction added to every character prefab that can be a casualty.
// Shows only when the target is unconscious and has a borrowable kit.
// All inventory mutation is server-authoritative.
// -----------------------------------------------------------------------
class CGQC_BorrowAction : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		return CanBorrow(user);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBorrow(user);
	}

	protected bool CanBorrow(IEntity user)
	{
		IEntity casualty = GetOwner();
		if (!casualty || !user)
			return false;

		// Self-use not allowed
		if (user == casualty)
			return false;

		// Never on dead characters
		if (CGQC_MedCore.IsActorDead(casualty))
			return false;

		// Must be unconscious
		if (!CGQC_MedCore.IsCasualtyUnconscious(casualty))
			return false;

		CGQC_CasualtyComponent casualtyComp = CGQC_CasualtyComponent.Cast(
			casualty.FindComponent(CGQC_CasualtyComponent)
		);

		if (!casualtyComp)
			return false;

		// MP-safe: replicated [RplProp] flag; hides action after kit has been taken.
		if (!casualtyComp.IsBorrowable())
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		// All inventory operations must run on the server.
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
		{
			CGQC_MedCore.Log("BorrowAction: casualty is missing CGQC_CasualtyComponent. Add it to the character prefab.");
			return;
		}

		// Re-verify server truth: kit must exist right now.
		IEntity casualtyKit = CGQC_MedCore.FindMedKitOnCharacter(casualty);
		if (!casualtyKit)
		{
			CGQC_MedCore.Log("BorrowAction: no borrowable kit found on casualty (server check).");
			casualtyComp.Server_SetBorrowable(false);
			return;
		}

		IEntity borrowedKit;
		bool ok = CGQC_MedCore.ExecuteBorrow(casualty, borrower, casualtyKit, borrowedKit);
		if (!ok || !borrowedKit)
		{
			CGQC_MedCore.Log("BorrowAction: ExecuteBorrow failed.");

		    CGQC_CasualtyComponent borrowerComp = CGQC_CasualtyComponent.Cast(
		        borrower.FindComponent(CGQC_CasualtyComponent)
		    );
		    if (borrowerComp)
		        borrowerComp.ShowNotification("Inventaire plein - impossible d'emprunter le IFAK.");
		    return;
		}

		// Register the borrow record: sets up reconciliation snapshot + hides action (replicated).
		casualtyComp.RegisterBorrow(borrowedKit, borrower);
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = "Grab IFAK"; 
		return true;
	}
}