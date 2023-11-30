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
//
// teamplay_gamerules.cpp
//
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "items.h"
#include "UserMessages.h"

//=========================================================
//=========================================================
CHalfLifeRules::CHalfLifeRules()
{
	RefreshSkillData();
}

//=========================================================
//=========================================================
void CHalfLifeRules::Think()
{
}

//=========================================================
//=========================================================
bool CHalfLifeRules::IsMultiplayer()
{
	return false;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::IsDeathmatch()
{
	return false;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::IsCoOp()
{
	return false;
}


//=========================================================
//=========================================================
bool CHalfLifeRules::FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon)
{
	if (!pPlayer->m_pActiveWeapon)
	{
		// player doesn't have an active weapon!
		return true;
	}

	if (!pPlayer->m_pActiveWeapon->CanHolster())
	{
		return false;
	}

	return true;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::GetNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pCurrentWeapon, bool alwaysSearch)
{
	//If this is an exhaustible weapon and it's out of ammo, always try to switch even in singleplayer.
	if (alwaysSearch || ((pCurrentWeapon->iFlags() & WEAPON_FLAG_EXHAUSTIBLE) != 0 && pCurrentWeapon->iAmmo1() > AMMO_NONE && pPlayer->m_rgAmmo[pCurrentWeapon->iAmmo1()] == 0))
	{
		return CGameRules::GetNextBestWeapon(pPlayer, pCurrentWeapon);
	}

	return false;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	return true;
}

void CHalfLifeRules::ClientPutInServer(CBasePlayer* pPlayer)
{
}

//=========================================================
//=========================================================
void CHalfLifeRules::ClientDisconnected(edict_t* pClient)
{
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlPlayerFallDamage(CBasePlayer* pPlayer)
{
	// subtract off the speed at which a player is allowed to fall without being hurt,
	// so damage will be based on speed beyond that, not the entire fall
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerSpawn(CBasePlayer* pPlayer)
{
}

//=========================================================
//=========================================================
bool CHalfLifeRules::AllowAutoTargetCrosshair()
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerThink(CBasePlayer* pPlayer)
{
}


//=========================================================
//=========================================================
bool CHalfLifeRules::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	return true;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlPlayerSpawnTime(CBasePlayer* pPlayer)
{
	return gpGlobals->time; //now!
}

//=========================================================
//=========================================================
bool CHalfLifeRules::FPlayerCanSuicide(CBasePlayer* pPlayer)
{
	return false;
}

//=========================================================
// GetPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
float CHalfLifeRules::GetPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled, bool assist)
{
	if (assist)
	{
		return 0.5F;
	}
	return 1;
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeRules::PlayerKilled(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType)
{
}

//=========================================================
// Deathnotice
//=========================================================
void CHalfLifeRules::DeathNotice(CBasePlayer* pVictim, CBaseEntity* killer, CBaseEntity* inflictor, CBaseEntity* accomplice, int bitsDamageType)
{
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeRules::PlayerGotWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon)
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeRules::FlWeaponRespawnTime(CBasePlayerWeapon* pWeapon)
{
	return -1;
}

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeRules::FlWeaponTryRespawn(CBasePlayerWeapon* pWeapon)
{
	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeRules::VecWeaponRespawnSpot(CBasePlayerWeapon* pWeapon)
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeRules::WeaponShouldRespawn(CBasePlayerWeapon* pWeapon)
{
	return GR_WEAPON_RESPAWN_NO;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::CanHaveItem(CBasePlayer* pPlayer, CItem* pItem)
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotItem(CBasePlayer* pPlayer, CItem* pItem)
{
}

//=========================================================
//=========================================================
int CHalfLifeRules::ItemShouldRespawn(CItem* pItem)
{
	return GR_ITEM_RESPAWN_NO;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeRules::FlItemRespawnTime(CItem* pItem)
{
	return -1;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeRules::VecItemRespawnSpot(CItem* pItem)
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::IsAllowedToSpawn(CBaseEntity* pEntity)
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotAmmo(CBasePlayer* pPlayer, char* szName, int iCount)
{
}

//=========================================================
//=========================================================
int CHalfLifeRules::AmmoShouldRespawn(CBasePlayerAmmo* pAmmo)
{
	return GR_AMMO_RESPAWN_NO;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo)
{
	return -1;
}

//=========================================================
//=========================================================
Vector CHalfLifeRules::VecAmmoRespawnSpot(CBasePlayerAmmo* pAmmo)
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlHealthChargerRechargeTime()
{
	return 0; // don't recharge
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerWeapons(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_GUN_NO;
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerAmmo(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_AMMO_NO;
}

//=========================================================
//=========================================================
int CHalfLifeRules::PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget)
{
	// why would a single player in half life need this?
	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
bool CHalfLifeRules::FAllowMonsters()
{
	return true;
}