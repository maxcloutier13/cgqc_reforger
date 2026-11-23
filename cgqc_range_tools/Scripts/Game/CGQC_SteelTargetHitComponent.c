// ============================================================
// CGQC_SteelTargetHitComponent.c
// Simple hit feedback for steel targets.
// Shows distance + velocity via CGQC_BasicDisplay
// only when shooter is > 100 m away (closer hits are obvious).
// No zone detection, no reset action.
// ============================================================


// ----------------------------------------------------------------
// BaseProjectileEffect — fires CLIENT-SIDE on every bullet hit.
// No RPC needed: distance and speed are available locally.
// ----------------------------------------------------------------
class CGQC_SteelTargetHitEffect : BaseProjectileEffect
{
	override void OnEffect(IEntity pHitEntity, inout vector outMat[3], IEntity damageSource, notnull Instigator instigator, string colliderName, float speed)
	{
		if (!pHitEntity)
			return;

		// Only show feedback for the local player's own shots
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		IEntity localPlayer = pc.GetControlledEntity();
		if (!localPlayer || instigator.GetInstigatorEntity() != localPlayer)
			return;

		// Only fire on entities that carry CGQC_SteelTargetHitComponent
		CGQC_SteelTargetHitComponent hitComp =
			CGQC_SteelTargetHitComponent.Cast(pHitEntity.FindComponent(CGQC_SteelTargetHitComponent));
		if (!hitComp)
			return;

		// Subtract ~1 m to compensate for eye-level vs. entity origin offset
		float distance = vector.Distance(localPlayer.GetOrigin(), pHitEntity.GetOrigin()) - 1.0;

		// Skip feedback at close range — the hit sound is sufficient
		if (distance <= 100.0)
			return;

		int distM = (int)Math.Round(distance);
		int velMs = (int)Math.Round(speed);

		// --- Player name ---
		int pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(localPlayer);
		string playerName = GetGame().GetPlayerManager().GetPlayerName(pid);

		// --- Weapon and mag name ---
		string weaponName = "Unknown";
		string magName = "Unknown";
		BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(localPlayer.FindComponent(BaseWeaponManagerComponent));
		if (wm)
		{
			WeaponSlotComponent ws = wm.GetCurrentSlot();
			if (ws)
			{
				IEntity wEnt = ws.GetWeaponEntity();
				BaseWeaponComponent wComp = null;
				if (wEnt)
					wComp = BaseWeaponComponent.Cast(wEnt.FindComponent(BaseWeaponComponent));
				if (wComp)
				{
					UIInfo wUI = wComp.GetUIInfo();
					if (wUI)
						weaponName = wUI.GetName();
					BaseMagazineComponent mag = wComp.GetCurrentMagazine();
					if (mag)
					{
						InventoryItemComponent mInv = InventoryItemComponent.Cast(mag.GetOwner().FindComponent(InventoryItemComponent));
						if (mInv)
						{
							UIInfo mUI = mInv.GetUIInfo();
							if (mUI)
								magName = mUI.GetName();
						}
					}
				}
			}
		}

		// --- Date/time ---
		int year, month, day, hour, minute, second;
		System.GetHourMinuteSecond(hour, minute, second);
		System.GetYearMonthDay(year, month, day);
		string yy = (year % 100).ToString();
		if (year % 100 < 10) yy = "0" + yy;
		string mm = month.ToString();
		if (month < 10) mm = "0" + mm;
		string dd = day.ToString();
		if (day < 10) dd = "0" + dd;
		string hh = hour.ToString();
		if (hour < 10) hh = "0" + hh;
		string mn = minute.ToString();
		if (minute < 10) mn = "0" + mn;
		string datetime = yy + mm + dd + " - " + hh + ":" + mn;

		// --- Build body ---
		string sep = "-------------------------------------------";
		string label = hitComp.GetLabel();

		string body = playerName + " | " + weaponName;
		body = body + "\n" + datetime;
		body = body + "\n" + magName;
		body = body + "\n" + sep;
		if (!label.IsEmpty())
			body = body + "\n" + "<color rgba=\"255,80,80,255\"><b>" + label + "</b></color>";
		body = body + "\nDistance: " + distM.ToString() + " m\nVélocité: " + velMs.ToString() + " m/s";

		CGQC_BasicDisplay.Show("Impact!", body, 4.0);
	}
}


// ----------------------------------------------------------------
// Component class descriptor
// ----------------------------------------------------------------
class CGQC_SteelTargetHitComponentClass : ScriptComponentClass {}


// ----------------------------------------------------------------
// Marker component — place on the steel target prefab.
// Set m_sLabel in the editor per section/paddle (e.g. "Tête!", "Cœur!").
// Leave blank for no label.
// ----------------------------------------------------------------
class CGQC_SteelTargetHitComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox,
		desc: "Hit label shown as first line of body. Leave empty for no label. E.g.: Tete!, Coeur!")]
	protected string m_sLabel;

	string GetLabel() { return m_sLabel; }
}