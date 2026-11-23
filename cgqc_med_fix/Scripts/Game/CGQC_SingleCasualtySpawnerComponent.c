// scripts/GameCode/CGQC_SingleCasualtySpawnerComponent.c

modded class AF_SingleCasualtySpawnerComponent : ScriptComponent
{
    // Stores pending CGQC scenario between SpawnTourniquetDrill() and ExecuteSingleSpawn()
    protected int m_NextCGQCScenario = 0;
    protected CGQC_EDifficulty m_NextCGQCDifficulty = CGQC_EDifficulty.EASY;

    void SpawnTourniquetDrill(CGQC_EDifficulty difficulty = CGQC_EDifficulty.EASY)
    {
        float now = GetGame().GetWorld().GetWorldTime();
        if ((now - m_fLastSpawnTime) < COOLDOWN_MS) return;
        m_fLastSpawnTime = now;

        ClearSingle();
        auto mascasComp = AF_MASCASSpawnerComponent.Cast(GetOwner().FindComponent(AF_MASCASSpawnerComponent));
        if (mascasComp) mascasComp.ClearMASCASGroup();

        m_NextScenario = AF_ETrainingScenario.NONE;
        m_NextCGQCScenario = CGQC_SCENARIO_TOURNIQUET_DRILL;
        m_NextCGQCDifficulty = difficulty;
        GetGame().GetCallqueue().CallLater(ExecuteSingleSpawn, 400, false);
    }

    override protected void ExecuteSingleSpawn()
    {
        if (m_CasualtyPrefab == string.Empty) return;

        Resource res = Resource.Load(m_CasualtyPrefab);
        vector mat[4];

        vector ownerPos = GetOwner().GetOrigin();
        float spawnX = ownerPos[0] + 2;
        float spawnZ = ownerPos[2];
        float groundY = GetGame().GetWorld().GetSurfaceY(spawnX, spawnZ);
        vector pos = Vector(spawnX, groundY, spawnZ);

        Math3D.AnglesToMatrix(Vector(Math.RandomFloat(0, 360), 0, 0), mat);
        mat[3] = pos;

        EntitySpawnParams sp = new EntitySpawnParams();
        sp.TransformMode = ETransformMode.WORLD;
        sp.Transform = mat;

        m_LastSpawned = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), sp);

        if (m_LastSpawned)
        {
            AF_CasualtyDamageComponent damageComp = AF_CasualtyDamageComponent.Cast(GetOwner().FindComponent(AF_CasualtyDamageComponent));
            if (damageComp)
            {
                if (m_NextCGQCScenario != 0)
                {
                    damageComp.ApplyCGQCScenario(m_LastSpawned, m_NextCGQCScenario, m_NextCGQCDifficulty);
                    m_NextCGQCScenario = 0;
                }
                else if (m_NextScenario != AF_ETrainingScenario.NONE)
                {
                    damageComp.ApplyTrainingScenario(m_LastSpawned, m_NextScenario);
                }
                else
                {
                    damageComp.ApplyToCasualty(m_LastSpawned, GetOwner());
                }
            }
        }
    }
}