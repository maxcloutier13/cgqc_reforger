modded class PACT_ShotTimerComponent
{
    protected IEntity m_LastWeaponEntity;
    protected bool    m_bSuppressShot;
    protected float   m_fLastActualShot;
    protected bool    m_bListenersRegistered = false;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        InventoryItemComponent itemComp = InventoryItemComponent.Cast(
            owner.FindComponent(InventoryItemComponent)
        );
        if (itemComp)
            itemComp.m_OnParentSlotChangedInvoker.Insert(OnSlotChanged);
    }

    protected void OnSlotChanged(InventoryStorageSlot oldSlot, InventoryStorageSlot newSlot)
    {
        PlayerController pc = GetGame().GetPlayerController();
        if (!pc) return;
        ActionManager am = pc.GetActionManager();
        if (!am) return;

        if (newSlot && !m_bListenersRegistered)
        {
            am.AddActionListener("PACT_ArmTimer",   EActionTrigger.DOWN, OnArmKey);
            am.AddActionListener("PACT_ResetTimer", EActionTrigger.DOWN, OnResetKey);
            m_bListenersRegistered = true;
            Print("[PACT] Listeners registered");
        }
        else if (!newSlot && m_bListenersRegistered)
        {
            am.RemoveActionListener("PACT_ArmTimer",   EActionTrigger.DOWN, OnArmKey);
            am.RemoveActionListener("PACT_ResetTimer", EActionTrigger.DOWN, OnResetKey);
            m_bListenersRegistered = false;
            Print("[PACT] Listeners unregistered");
        }
    }

    protected void OnArmKey(float value, EActionTrigger reason)
    {
        Print("[PACT] OnArmKey fired");
        if (m_bRunning)   { StopTimer();               return; }
        if (m_bStopped)   { ResetTimer();              return; }
        if (m_bArmed)     { StartTimer(m_OwnerEntity); return; }
        ArmTimer();
    }

    protected void OnResetKey(float value, EActionTrigger reason)
    {
        Print("[PACT] OnResetKey fired");
        ResetTimer();
    }

    override void EOnFrame(IEntity owner, float timeSlice)
    {
        if (!m_PlayerEntity)
            ResolveLocalPlayer();

        if (m_bRunning)
        {
            DetectShot();
            UpdateHUD();
        }

        if (m_bRunning && m_PlayerEntity)
            CheckWeaponEvents();
    }

    override protected void PollHotkeys()
    {
        // Input now handled by action listeners — polling disabled
    }

    override void StartTimer(IEntity owner)
    {
        if (m_bRunning || m_bStartPending) return;

        m_bArmed = true;
        m_bRunning = false;
        m_bStopped = false;
        m_bStartPending = true;
        m_iLastAmmoCount = -1;
        m_LastWeaponEntity = null;
        m_bSuppressShot = false;
        m_fLastActualShot = 0.0;

        if (m_HUD) { m_HUD.SetState(PACT_TimerState.ARMED); m_HUD.SetStandby(); }

        AudioSystem.PlaySound("{5FFE043DE4AD561B}");
        int delay = Math.RandomInt(2500, 3001);
        GetGame().GetCallqueue().CallLater(BeginTimerAfterBeep, delay, false);
        Print(string.Format("[PACT ShotTimer] CGQC: STANDBY, beep in %1 ms.", delay));
    }

    override void BeginTimerAfterBeep()
    {
        if (!m_bStartPending) return;
        PlayBeep(m_OwnerEntity);
        GetGame().GetCallqueue().CallLater(CommitTimerStart, 975, false);
    }

    protected void CommitTimerStart()
    {
        if (!m_bStartPending) return;
        m_bStartPending = false;
        m_bRunning = true;
        m_bArmed = true;
        m_bStopped = false;
        m_fStartTime = System.GetTickCount();
        m_fLastShot = m_fStartTime;
        m_iLastAmmoCount = -1;
        m_LastWeaponEntity = null;
        m_bSuppressShot = false;
        m_fLastActualShot = 0.0;
        if (m_HUD) m_HUD.SetState(PACT_TimerState.RUNNING);
        Print("[PACT ShotTimer] CGQC: RUNNING.");
    }

    override float GetTotalTime()
    {
        if (m_fStartTime <= 0.0 || m_fLastActualShot <= 0.0) return 0.0;
        return m_fLastActualShot - m_fStartTime;
    }

    override protected void OnShotDetected()
    {
        if (m_bSuppressShot) { m_bSuppressShot = false; return; }
        super.OnShotDetected();
        m_fLastActualShot = m_fLastShot;
        if (m_HUD) m_HUD.SetTotal(m_fLastActualShot - m_fStartTime);
    }

    protected void CheckWeaponEvents()
    {
        if (!m_WeaponMgr) return;
        BaseWeaponComponent weaponComp = BaseWeaponComponent.Cast(m_WeaponMgr.GetCurrentWeapon());
        if (!weaponComp) return;
        IEntity currentWeapon = weaponComp.GetOwner();

        if (m_LastWeaponEntity && currentWeapon != m_LastWeaponEntity)
        {
            float now = System.GetTickCount();
            float split = now - m_fLastShot;
            m_fLastShot = now;
            float elapsed = now - m_fStartTime;
            if (m_HUD) m_HUD.AddEventRow("TRANSITION", split, elapsed);
            m_iLastAmmoCount = -1;
            m_LastWeaponEntity = currentWeapon;
            return;
        }

        m_LastWeaponEntity = currentWeapon;
        array<BaseMuzzleComponent> muzzles = {};
        weaponComp.GetMuzzlesList(muzzles);
        if (muzzles.Count() == 0 || !muzzles[0]) return;

        int currentAmmo = muzzles[0].GetAmmoCount();
        if (m_iLastAmmoCount >= 0)
        {
            if (currentAmmo > m_iLastAmmoCount)
            {
                float now = System.GetTickCount();
                float split = now - m_fLastShot;
                m_fLastShot = now;
                float elapsed = now - m_fStartTime;
                if (m_HUD) m_HUD.AddEventRow("REL", split, elapsed);
                Print(string.Format("[PACT ShotTimer] CGQC: Reload at %1s.", (elapsed / 1000.0).ToString(2)));
            }
            else if (currentAmmo < m_iLastAmmoCount && (m_iLastAmmoCount - currentAmmo) > 1)
                m_bSuppressShot = true;
        }
    }
}