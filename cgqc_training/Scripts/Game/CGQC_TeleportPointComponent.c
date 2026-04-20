modded class TELEP_TeleportPointComponent : ScriptComponent
{
    [Attribute("", UIWidgets.EditBox, "Editor-set display name")]
    protected string m_sEditorName;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        if (Replication.IsServer() && !m_sEditorName.IsEmpty())
            SetPointName(m_sEditorName);
    }
}