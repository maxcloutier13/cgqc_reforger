class CGQC_ArsenalStashManagerComponentClass : ScriptComponentClass {}

class CGQC_ArsenalStashEntry
{
    ResourceName m_sPrefab;
    typename m_tAreaType;

    void CGQC_ArsenalStashEntry(ResourceName prefab, typename areaType)
    {
        m_sPrefab = prefab;
        m_tAreaType = areaType;
    }
}

class CGQC_ArsenalStashManagerComponent : ScriptComponent
{
    ref map<RplId, ref array<ref CGQC_ArsenalStashEntry>> m_mPlayerStashes =
        new map<RplId, ref array<ref CGQC_ArsenalStashEntry>>();

    bool HasItems(RplId playerId)
    {
        return m_mPlayerStashes.Contains(playerId) && !m_mPlayerStashes[playerId].IsEmpty();
    }

    array<ref CGQC_ArsenalStashEntry> GetStash(RplId playerId)
    {
        if (!m_mPlayerStashes.Contains(playerId))
            m_mPlayerStashes[playerId] = new array<ref CGQC_ArsenalStashEntry>();
        return m_mPlayerStashes[playerId];
    }

    void Clear(RplId playerId)
    {
        if (m_mPlayerStashes.Contains(playerId))
            m_mPlayerStashes[playerId].Clear();
    }
}