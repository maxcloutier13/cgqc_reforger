// ============================================================
// CGQC_RangeTools_ChronographComponent.c
// ============================================================

class CGQC_RangeTools_ChronographComponentClass : ScriptComponentClass {}

class CGQC_RangeTools_ChronographComponent : ScriptComponent
{
    protected float m_fLastMuzzleVelocity = 0.0;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        if (!GetGame().InPlayMode())
            return;

        RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
        if (rpl && rpl.IsProxy())
            return;

        EventHandlerManagerComponent ehm = EventHandlerManagerComponent.Cast(
            owner.FindComponent(EventHandlerManagerComponent));
        if (!ehm)
        {    
            return;
        }

        ehm.RegisterScriptHandler("OnProjectileShot", this, OnProjectileShot);      
    }

    override void OnDelete(IEntity owner)
    {
        EventHandlerManagerComponent ehm = EventHandlerManagerComponent.Cast(
            owner.FindComponent(EventHandlerManagerComponent));
        if (ehm)
            ehm.RemoveScriptHandler("OnProjectileShot", this, OnProjectileShot);

        super.OnDelete(owner);
    }

    protected void OnProjectileShot(int playerID, BaseWeaponComponent weapon, IEntity projectile)
    {
        if (!projectile)
            return;

        ProjectileMoveComponent pmc = ProjectileMoveComponent.Cast(
            projectile.FindComponent(ProjectileMoveComponent));
        if (!pmc)
        {
            Print("[CGQC_Chrono] ERROR: No ProjectileMoveComponent on projectile", LogLevel.ERROR);
            return;
        }

        m_fLastMuzzleVelocity = pmc.GetVelocity().Length();
    }

    float GetLastMuzzleVelocity()
    {
        return m_fLastMuzzleVelocity;
    }
}