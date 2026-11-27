//------------------------------------------------------------------------------------------------
// CGQC Personal Patch - Arsenal
// When the local player closes an arsenal, asks the server to put their patch back if needed.
// Vanilla arsenal: SCR_InventoryMenuUI below.
// Bacon Loadout Editor: its menu is sealed and doesn't report closing, so it's watched while open.
//------------------------------------------------------------------------------------------------

class CGQC_PersonalPatchArsenal
{
	protected static const int WATCH_INTERVAL = 500;	// ms
	protected static const int REQUEST_DELAY = 1000;	// ms, lets the last arsenal changes finish

	//------------------------------------------------------------------------------------------------
	static void Register()
	{
		ScriptInvokerMenu onMenuOpen = SCR_MenuHelper.GetOnMenuOpen();
		onMenuOpen.Remove(OnMenuOpen);
		onMenuOpen.Insert(OnMenuOpen);
	}

	//------------------------------------------------------------------------------------------------
	protected static void OnMenuOpen(ChimeraMenuBase menu)
	{
		if (!Bacon_GunBuilderUI.Cast(menu))
			return;

		ScriptCallQueue queue = GetGame().GetCallqueue();
		queue.Remove(WatchBaconMenu);
		queue.CallLater(WatchBaconMenu, WATCH_INTERVAL, true);
	}

	//------------------------------------------------------------------------------------------------
	protected static void WatchBaconMenu()
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (menuManager && menuManager.FindMenuByPreset(ChimeraMenuPreset.Bacon_GunBuilderUI))
			return;

		GetGame().GetCallqueue().Remove(WatchBaconMenu);
		OnArsenalClosed();
	}

	//------------------------------------------------------------------------------------------------
	static void OnArsenalClosed()
	{
		ScriptCallQueue queue = GetGame().GetCallqueue();
		queue.Remove(SendRequest);
		queue.CallLater(SendRequest, REQUEST_DELAY);
	}

	//------------------------------------------------------------------------------------------------
	protected static void SendRequest()
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.CGQC_RequestPatchCheck();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_PlayerController
{
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		CGQC_PersonalPatchArsenal.Register();
	}

	//------------------------------------------------------------------------------------------------
	void CGQC_RequestPatchCheck()
	{
		Rpc(CGQC_RpcAsk_PatchCheck);
	}

	//------------------------------------------------------------------------------------------------
	//! The patch comes from the server's roster, never from the client
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void CGQC_RpcAsk_PatchCheck()
	{
		string identityId = GetGame().GetBackendApi().GetPlayerIdentityId(GetPlayerId());
		CGQC_PlayerEntry entry = CGQC_Scripts.FindRosterEntry(identityId);
		if (entry)
			CGQC_PersonalPatch.ApplyPatch(GetMainEntity(), entry.m_sPatch);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_InventoryMenuUI
{
	protected bool m_bCGQC_ArsenalOpened;

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		SCR_InventoryStorageBaseUI.GetOnArsenalEnter().Insert(CGQC_OnArsenalEnter);
		super.OnMenuOpen();
	}

	//------------------------------------------------------------------------------------------------
	//! Arsenal opened in the loot panel
	protected void CGQC_OnArsenalEnter()
	{
		m_bCGQC_ArsenalOpened = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Arsenal opened as its own container
	override void OpenStorageAsContainer(BaseInventoryStorageComponent storage, bool showVicinity = true, bool hideCloseButton = false)
	{
		if (IsStorageArsenal(storage))
			m_bCGQC_ArsenalOpened = true;

		super.OpenStorageAsContainer(storage, showVicinity, hideCloseButton);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		super.OnMenuClose();
		SCR_InventoryStorageBaseUI.GetOnArsenalEnter().Remove(CGQC_OnArsenalEnter);

		if (m_bCGQC_ArsenalOpened)
			CGQC_PersonalPatchArsenal.OnArsenalClosed();

		m_bCGQC_ArsenalOpened = false;
	}
}
