modded class ACE_Overheating_BarrelComponentClass
{
	protected override float ComputeBarrelMass(IEntity weapon)
	{
	    float baseWeight = super.ComputeBarrelMass(weapon);
	    if (baseWeight > 0.1)
	        return baseWeight;
	
	    return FALLBACK_BARREL_MASS;
	}
}