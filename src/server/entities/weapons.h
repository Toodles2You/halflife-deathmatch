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

#pragma once

#include "weaponinfo.h"
#ifdef CLIENT_DLL
#include "event_args.h"
#endif

class CBasePlayer;
class CBasePlayerWeapon;

// Hit Group standards
enum {
	HITGROUP_GENERIC,
	HITGROUP_HEAD,
	HITGROUP_CHEST,
	HITGROUP_STOMACH,
	HITGROUP_LEFTARM,
	HITGROUP_RIGHTARM,
	HITGROUP_LEFTLEG,
	HITGROUP_RIGHTLEG,
	HITGROUP_COUNT,
};

enum WeaponAnim
{
	kWeaponAnimIdle = 0,
	kWeaponAnimDeploy,
	kWeaponAnimHolster,

	kWeaponAnimLast
};

typedef struct
{
	int iSlot;
	int iPosition;
	int iAmmo1;
	int iAmmo2;
	const char* pszName;
	int iMaxClip;
	int iFlags;
	int iWeight;
	const char* pszWorld;
	const char* pszView;
	const char* pszPlayer;
	const char* pszAnimExt;
	int iAnims[kWeaponAnimLast];
} WeaponInfo;

// inventory items that commit war crimes
class CBasePlayerWeapon : public CBaseAnimating
{
public:
	CBasePlayerWeapon(Entity* containingEntity) : CBaseAnimating(containingEntity) {}

	DECLARE_SAVERESTORE()

	void SetObjectCollisionBox() override;

	virtual bool Spawn() override;
	virtual void Precache() override;

	// generic weapon versions of CBasePlayerWeapon calls
	virtual bool AddToPlayer(CBasePlayer* pPlayer);
	virtual void RemoveFromPlayer(bool forceSendAnimations = true);
	virtual bool AddDuplicate(CBasePlayerWeapon* pOriginal) { return false; }
#ifndef CLIENT_DLL
	virtual void UpdateOnRemove() override;
#endif
	void EXPORT DefaultTouch(CBaseEntity* pOther);
	void EXPORT Materialize();
	void EXPORT AttemptToMaterialize();
	CBaseEntity* Respawn() override;
	void CheckRespawn();

	virtual void SendWeaponAnim(int iAnim);

	virtual bool CanDeploy() { return true; }
	virtual void Deploy();

	virtual void WeaponPostFrame() = 0;

	virtual bool CanHolster() { return true; }
	virtual void Holster();

	virtual void GetWeaponData(weapon_data_t& data);
	virtual void SetWeaponData(const weapon_data_t& data);
	virtual void DecrementTimers(const int msec);

	static inline WeaponInfo WeaponInfoArray[WEAPON_TYPES];

	CBasePlayer* m_pPlayer;

	virtual int GetID() const = 0;
	virtual void GetWeaponInfo(WeaponInfo& i) = 0;

	WeaponInfo& GetInfo() { return WeaponInfoArray[GetID()]; }
	int iAmmo1() { return GetInfo().iAmmo1; }
	int iAmmo2() { return GetInfo().iAmmo2; }
	const char* pszName() { return GetInfo().pszName; }
	int iMaxClip() { return GetInfo().iMaxClip; }
	int iWeight() { return GetInfo().iWeight; }
	int iFlags() { return GetInfo().iFlags; }

	//Hack so deploy animations work when weapon prediction is enabled.
	bool m_ForceSendAnimations = false;

	int m_iNextPrimaryAttack;
	int m_iClip;
	bool m_fInReload;
};

inline short g_sModelIndexPlayer;
inline short g_sModelIndexGibs;
inline short g_sModelIndexShell;
inline short g_sModelIndexLaser;
inline short g_sModelIndexLaserDot;
inline short g_sModelIndexFireball;
inline short g_sModelIndexSmoke;
inline short g_sModelIndexWExplosion;
inline short g_sModelIndexBubbles;
inline short g_sModelIndexBloodDrop;
inline short g_sModelIndexBloodSpray;
inline short g_sModelIndexWallPuff;
inline short g_sModelIndexDebris;

class CCrowbar : public CBasePlayerWeapon
{
public:
	CCrowbar(Entity* containingEntity) : CBasePlayerWeapon(containingEntity) {}

	enum
	{
		kAnimIdle = 0,
		kAnimDeploy,
		kAnimHolster,
		kAnimAttack1Hit,
		kAnimAttack1Miss,
		kAnimAttack2Miss,
		kAnimAttack2Hit,
		kAnimAttack3Miss,
		kAnimAttack3Hit
	};

	enum
	{
		kCrowbarMiss = 0,
		kCrowbarHitWorld,
		kCrowbarHitPlayer,
	};

	int GetID() const override { return WEAPON_CROWBAR; }
	void GetWeaponInfo(WeaponInfo& i) override;

	void Precache() override;

	void PrimaryAttack();
	void WeaponPostFrame() override;

#ifdef CLIENT_DLL
	static void EV_PrimaryAttack(event_args_t* args);
#endif

private:
	unsigned short m_usPrimaryAttack;
};

class CMP5 : public CBasePlayerWeapon
{
public:
	CMP5(Entity* containingEntity) : CBasePlayerWeapon(containingEntity) {}

	enum
	{
		kAnimLongidle = 0,
		kAnimIdle1,
		kAnimLaunch,
		kAnimReload,
		kAnimDeploy,
		kAnimFire1,
		kAnimFire2,
		kAnimFire3,
		kAnimHolster,
	};

	int GetID() const override { return WEAPON_MP5; }
	void GetWeaponInfo(WeaponInfo& i) override;

	void Precache() override;

	void PrimaryAttack();
	void SecondaryAttack();
	void WeaponPostFrame() override;

#ifdef CLIENT_DLL
	static void EV_PrimaryAttack(event_args_t* args);
	static void EV_SecondaryAttack(event_args_t* args);
#endif

private:
	unsigned short m_usPrimaryAttack;
	unsigned short m_usSecondaryAttack;
};

#ifdef GAME_DLL

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseAnimating
{
public:
	CGrenade(Entity* containingEntity) : CBaseAnimating(containingEntity) {}

	bool Spawn() override;

	static CGrenade* ShootContact(CBaseEntity* owner, Vector vecStart, Vector vecVelocity);

	void Explode(Vector vecSrc, Vector vecAim);
	void Explode(TraceResult* pTrace, int bitsDamageType);

	void EXPORT BounceTouch(CBaseEntity* pOther);
	void EXPORT ExplodeTouch(CBaseEntity* pOther);
	void EXPORT Detonate();
	void EXPORT TumbleThink();

	virtual void BounceSound();
};

#ifdef HALFLIFE_GRENADES

/* Team Fortress style timed grenade */
class CPrimeGrenade : public CGrenade
{
public:
	CPrimeGrenade(Entity* containingEntity) : CGrenade(containingEntity) {}

	bool Spawn() override;
	void EXPORT PrimedThink();

	static CPrimeGrenade* PrimeGrenade(CBaseEntity* owner);

protected:
	typedef enum
	{
		kThrow,
		kDrop,
		kOvercook,
	} throw_e;

	void Throw(throw_e mode);
};

#endif

void RadiusDamage(
	const Vector& origin,
	CBaseEntity* inflictor,
	CBaseEntity* attacker,
	const float damage,
	const float radius,
	const int damageType);

#endif
