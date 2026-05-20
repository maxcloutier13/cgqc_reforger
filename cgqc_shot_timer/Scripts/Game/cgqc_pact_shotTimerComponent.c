modded class PACT_ShotTimerComponent
{
    protected IEntity m_LastWeaponEntity;
    protected bool    m_bSuppressShot;
    protected float   m_fLastActualShot;

    override void StartTimer(IEntity owner)
    {
        if (m_bRunning || m_bStartPending)
            return;

        m_bArmed = true;
        m_bRunning = false;
        m_bStopped = false;
        m_bStartPending = true;
        m_iLastAmmoCount = -1;
        m_LastWeaponEntity = null;
        m_bSuppressShot = false;
        m_fLastActualShot = 0.0;

        if (m_HUD)
        {
            m_HUD.SetState(PACT_TimerState.ARMED);
            m_HUD.SetStandby();
        }

        AudioSystem.PlaySound("{5FFE043DE4AD561B}");

        int delay = Math.RandomInt(2500, 3001);
        GetGame().GetCallqueue().CallLater(BeginTimerAfterBeep, delay, false);
        Print(string.Format("[PACT ShotTimer] CGQC: STANDBY, beep in %1 ms.", delay));
    }

    override void BeginTimerAfterBeep()
    {
        if (!m_bStartPending)
            return;

        PlayBeep(m_OwnerEntity);
        GetGame().GetCallqueue().CallLater(CommitTimerStart, 975, false);
    }

    protected void CommitTimerStart()
    {
        if (!m_bStartPending)
            return;

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

        if (m_HUD)
            m_HUD.SetState(PACT_TimerState.RUNNING);

        Print("[PACT ShotTimer] CGQC: RUNNING.");
    }

    // Bypass whatever PACT uses internally for total — always return elapsed at last real shot
    override float GetTotalTime()
    {
        if (m_fStartTime <= 0.0 || m_fLastActualShot <= 0.0)
            return 0.0;
        return m_fLastActualShot - m_fStartTime;
    }

    override void EOnFrame(IEntity owner, float timeSlice)
    {
        if (m_bRunning && m_PlayerEntity)
            CheckWeaponEvents();

        super.EOnFrame(owner, timeSlice);
    }

    override protected void OnShotDetected()
    {
        if (m_bSuppressShot)
        {
            m_bSuppressShot = false;
            return;
        }

        super.OnShotDetected();

        // Capture timestamp after base updates m_fLastShot
        m_fLastActualShot = m_fLastShot;

        // Set TotalLabel directly — don't rely on PACT calling GetTotalTime()
        if (m_HUD)
            m_HUD.SetTotal(m_fLastActualShot - m_fStartTime);
    }

    protected void CheckWeaponEvents()
    {
        if (!m_WeaponMgr) return;

        BaseWeaponComponent weaponComp = BaseWeaponComponent.Cast(m_WeaponMgr.GetCurrentWeapon());
        if (!weaponComp) return;

        IEntity currentWeapon = weaponComp.GetOwner();

        // --- Transition detection ---
        if (m_LastWeaponEntity && currentWeapon != m_LastWeaponEntity)
        {
            float now = System.GetTickCount();
            float split = now - m_fLastShot;
            m_fLastShot = now;
            float elapsed = now - m_fStartTime;

            if (m_HUD)
                m_HUD.AddEventRow("TRANSITION", split, elapsed);

            m_iLastAmmoCount = -1;
            m_LastWeaponEntity = currentWeapon;
            return;
        }

        m_LastWeaponEntity = currentWeapon;

        // --- Ammo change detection ---
        array<BaseMuzzleComponent> muzzles = new array<BaseMuzzleComponent>();
        weaponComp.GetMuzzlesList(muzzles);
        if (muzzles.Count() == 0 || !muzzles[0]) return;

        int currentAmmo = muzzles[0].GetAmmoCount();

        if (m_iLastAmmoCount >= 0)
        {
            if (currentAmmo > m_iLastAmmoCount)
            {
                // Reload complete
                float now = System.GetTickCount();
                float split = now - m_fLastShot;
                m_fLastShot = now;
                float elapsed = now - m_fStartTime;

                if (m_HUD)
                    m_HUD.AddEventRow("REL", split, elapsed);

                Print(string.Format("[PACT ShotTimer] CGQC: Reload at %1s.", (elapsed / 1000.0).ToString(2)));
            }
            else if (currentAmmo < m_iLastAmmoCount && (m_iLastAmmoCount - currentAmmo) > 1)
            {
                // Magazine ejected — suppress the false shot base DetectShot will fire
                m_bSuppressShot = true;
            }
        }
        // m_iLastAmmoCount updated by super.EOnFrame's DetectShot
    }
}