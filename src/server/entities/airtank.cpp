/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

class CAirtank : public CGrenade
{
	bool Spawn() override;
	void Precache() override;
	void EXPORT TankThink();
	void EXPORT TankTouch(CBaseEntity* pOther);
	void Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	bool m_state;
};


LINK_ENTITY_TO_CLASS(item_airtank, CAirtank);
TYPEDESCRIPTION CAirtank::m_SaveData[] =
	{
		DEFINE_FIELD(CAirtank, m_state, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CAirtank, CGrenade);


bool CAirtank::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SetModel("models/w_oxygen.mdl");
	SetSize(Vector(-16, -16, 0), Vector(16, 16, 36));
	SetOrigin(pev->origin);

	SetTouch(&CAirtank::TankTouch);
	SetThink(&CAirtank::TankThink);

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_YES;
	pev->health = 20;
	pev->dmg = 50;
	m_state = true;

	return true;
}

void CAirtank::Precache()
{
	PRECACHE_MODEL("models/w_oxygen.mdl");
	PRECACHE_SOUND("doors/aliendoor3.wav");
}


void CAirtank::Killed(CBaseEntity* inflictor, CBaseEntity* attacker, int bitsDamageType)
{
	pev->owner = attacker->edict();

	Explode(pev->origin, Vector(0, 0, -1));
}


void CAirtank::TankThink()
{
	// Fire trigger
	m_state = true;
	UseTargets(this, USE_TOGGLE, 0);
}


void CAirtank::TankTouch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer())
		return;

	if (!m_state)
	{
		// "no oxygen" sound
		EmitSound("player/pl_swim2.wav", CHAN_BODY);
		return;
	}

	// give player 12 more seconds of air
	pOther->pev->air_finished = gpGlobals->time + 12;

	// suit recharge sound
	EmitSound("doors/aliendoor3.wav", CHAN_VOICE);

	// recharge airtank in 30 seconds
	pev->nextthink = gpGlobals->time + 30;
	m_state = false;
	UseTargets(this, USE_TOGGLE, 1);
}