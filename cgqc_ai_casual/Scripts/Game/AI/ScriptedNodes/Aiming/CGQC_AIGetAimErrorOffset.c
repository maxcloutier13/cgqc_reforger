modded class SCR_AIGetAimErrorOffset: AITaskScripted
{
	static const float AIMING_ERROR_FACTOR_MAX = 1.2;
	static const float MAXIMAL_TOLERANCE = 20.0;
	static const float MINIMAL_TOLERANCE = 0.006;
	
	//------------------------------------------------------------------------------------------------	
	override float GetAngularSpeedFactor(IEntity observer, IEntity enemy, out bool setBigTolerance)
	{
		vector enemyVelocity;
		IEntity enemyRoot = enemy.GetRootParent();
		Physics enemyPhysics = enemyRoot.GetPhysics();
		if (enemyPhysics)
			enemyVelocity = enemyPhysics.GetVelocity();
		
		vector observerVelocity;
		vector observerAngularVelocity;
		IEntity observerRoot = observer.GetRootParent();
		Physics observerPhysics = observerRoot.GetPhysics();
		if (observerPhysics)
		{
			observerVelocity = observerPhysics.GetVelocity();
			observerAngularVelocity = observerPhysics.GetAngularVelocity();
		}
		
		vector relativeVelocity = enemyVelocity - observerVelocity;
		
		vector positionVector = enemy.GetOrigin() - observer.GetOrigin();
		vector targetLocalAngularVelocity = observerAngularVelocity + (positionVector * relativeVelocity / positionVector.LengthSq());  // omega = (r x v) / ||r||^2 
		float totalTargetLocalAngularVelocity = targetLocalAngularVelocity.Length();			
		
		if (totalTargetLocalAngularVelocity < 0.07) // rougly 4 degs in radians
			return 1.0;
		else if (totalTargetLocalAngularVelocity < 0.17) // roughly 10 degs in radians
			return 4;
	
		setBigTolerance = true;
		return 0;
	}
};

