[ComponentEditorProps(category: "CGQC/GameMode", description: "Persists player radio frequencies across death and respawn.")]
class CGQC_RadioFrequencyPersistComponentClass : SCR_BaseGameModeComponentClass
{
}

class CGQC_RadioFrequencyPersistComponent : SCR_BaseGameModeComponent
{
	// playerId -> ordered list of frequencies (kHz) from all transceivers at time of death
	protected ref map<int, ref array<int>> m_mPlayerFrequencies = new map<int, ref array<int>>();

	//------------------------------------------------------------------------------------------------
	// Player killed -> Gather information while we have access to it
	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		int playerId = instigatorContextData.GetVictimPlayerID();
		IEntity playerEntity = instigatorContextData.GetVictimEntity();

		if (!playerEntity)
			return;

		array<int> freqs = new array<int>();
		CollectFrequencies(playerEntity, freqs);

		if (freqs.IsEmpty())
			return;

		m_mPlayerFrequencies.Set(playerId, freqs);
		PrintFormat("[CGQC_RadioFreq] Saved %1 freq(s) for player %2", freqs.Count(), playerId);
	}

	//------------------------------------------------------------------------------------------------
	// Runs once spawn initialisation is done
	override void OnPlayerSpawnFinalize_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnHandlerComponent handlerComponent, SCR_SpawnData data, IEntity entity)
	{
		if (!requestComponent || !entity)
			return;

		int playerId = requestComponent.GetPlayerId();

		array<int> freqs;
		if (!m_mPlayerFrequencies.Find(playerId, freqs) || freqs.IsEmpty())
			return;

		m_mPlayerFrequencies.Remove(playerId);

		// Defer apply — inventory may not be fully initialized at this exact moment
		GetGame().GetCallqueue().CallLater(ApplyFrequencies, 200, false, entity, freqs);
		PrintFormat("[CGQC_RadioFreq] Scheduled restore of %1 freq(s) for player %2", freqs.Count(), playerId);
	}

	//------------------------------------------------------------------------------------------------
	// Cleanup on player disconnect
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		m_mPlayerFrequencies.Remove(playerId);
	}

	//------------------------------------------------------------------------------------------------
	// Save all frequencies
	protected void CollectFrequencies(IEntity entity, out array<int> freqs)
	{
		ReadRadiosOnEntity(entity, freqs);

		InventoryStorageManagerComponent inv = InventoryStorageManagerComponent.Cast(
			entity.FindComponent(InventoryStorageManagerComponent)
		);
		if (!inv)
			return;

		array<IEntity> items = new array<IEntity>();
		inv.GetItems(items);
		foreach (IEntity item : items)
			ReadRadiosOnEntity(item, freqs);
	}

	//------------------------------------------------------------------------------------------------
	// Find all radios
	protected void ReadRadiosOnEntity(IEntity entity, out array<int> freqs)
	{
		array<Managed> comps = new array<Managed>();
		entity.FindComponents(BaseRadioComponent, comps);

		foreach (Managed m : comps)
		{
			BaseRadioComponent radio = BaseRadioComponent.Cast(m);
			if (!radio || radio.IsEditorRadio())
				continue;

			int count = radio.TransceiversCount();
			for (int i = 0; i < count; i++)
			{
				BaseTransceiver tsv = radio.GetTransceiver(i);
				if (tsv)
					freqs.Insert(tsv.GetFrequency());
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// Set the radios with saved frequencies
	protected void ApplyFrequencies(IEntity entity, array<int> freqs)
	{
		if (!entity || freqs.IsEmpty())
			return;

		array<BaseTransceiver> transceivers = new array<BaseTransceiver>();
		GatherTransceivers(entity, transceivers);

		InventoryStorageManagerComponent inv = InventoryStorageManagerComponent.Cast(
			entity.FindComponent(InventoryStorageManagerComponent)
		);
		if (inv)
		{
			array<IEntity> items = new array<IEntity>();
			inv.GetItems(items);
			foreach (IEntity item : items)
				GatherTransceivers(item, transceivers);
		}

		int count = Math.Min(transceivers.Count(), freqs.Count());
		for (int i = 0; i < count; i++)
			transceivers[i].SetFrequency(freqs[i]);

		PrintFormat("[CGQC_RadioFreq] Restored %1 freq(s) to respawned entity", count);
	}

	//------------------------------------------------------------------------------------------------
	// Find all actual channels
	protected void GatherTransceivers(IEntity entity, out array<BaseTransceiver> transceivers)
	{
		array<Managed> comps = new array<Managed>();
		entity.FindComponents(BaseRadioComponent, comps);

		foreach (Managed m : comps)
		{
			BaseRadioComponent radio = BaseRadioComponent.Cast(m);
			if (!radio || radio.IsEditorRadio())
				continue;

			int count = radio.TransceiversCount();
			for (int i = 0; i < count; i++)
			{
				BaseTransceiver tsv = radio.GetTransceiver(i);
				if (tsv)
					transceivers.Insert(tsv);
			}
		}
	}
}