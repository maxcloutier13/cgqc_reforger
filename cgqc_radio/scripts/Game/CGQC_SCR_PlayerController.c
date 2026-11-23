/*
modded class SCR_PlayerController
{
    protected bool m_bCGQC_EditorSubscribed = false;

    //------------------------------------------------------------------------------------------------
    override void OnControlledEntityChanged(IEntity from, IEntity to)
    {
        super.OnControlledEntityChanged(from, to);

        // Subscribe to GM close event once
        if (!m_bCGQC_EditorSubscribed)
        {
            SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
            if (editorManager)
            {
                editorManager.GetOnClosed().Insert(CGQC_OnEditorClosed);
                m_bCGQC_EditorSubscribed = true;
            }
        }

        // Auto-Enable voice after 5secs
        GetGame().GetCallqueue().CallLater(AutoEnableVONDirect, 5000, false);
    }

    //------------------------------------------------------------------------------------------------
    protected void CGQC_OnEditorClosed()
    {
        // Small delay to let the game fully return control to the character
        GetGame().GetCallqueue().CallLater(AutoEnableVONDirect, 2000, false);
    }

    //------------------------------------------------------------------------------------------------
    protected void AutoEnableVONDirect()
    {
        SCR_VONController vonController = SCR_VONController.Cast(FindComponent(SCR_VONController));
        if (!vonController)
            return;

        vonController.ForceEnableVONDirectToggle();
    }
}*/