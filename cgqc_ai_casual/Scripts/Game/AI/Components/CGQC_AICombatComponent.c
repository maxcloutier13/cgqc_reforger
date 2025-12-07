modded class SCR_AICombatComponent : ScriptComponent
{

	protected static const float TARGET_SCORE_RETREAT = 50.0;				//!< Threshold for target score for retreating from that target
	
	// Perception factors
	protected const float PERCEPTION_FACTOR_SAFE = 1.0;			//!< We are safe and are good at recognising enemies
	protected const float PERCEPTION_FACTOR_VIGILANT = 2.0;		//!< When vigilant and alert we are very good at recognising enemies
	protected const float PERCEPTION_FACTOR_ALERTED = 2.5;
	protected const float PERCEPTION_FACTOR_THREATENED = 0.2;	// We are suppressed and are bad at recognizing enemies
	
	//! Within this distance AI considers combat as 'close range', used in firing times
	static const float CLOSE_RANGE_COMBAT_DISTANCE = 20.0;
	
	//! Beyond this distance AI considers combat as 'long range', used for danger events and firing times
	static const float LONG_RANGE_COMBAT_DISTANCE = 200.0;
	
	protected const float WEAPON_TARGET_UPDATE_PERIOD_MS = 500;
	
	
	protected const float FRAG_GRENADE_MAX_THREAT = 0.7; // Max threat measure soldier is allowed to have to be able to use frag grenades

		
}
