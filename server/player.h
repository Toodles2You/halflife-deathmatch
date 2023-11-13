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

#include "pm_materials.h"

#include <forward_list>


#define PLAYER_FATAL_FALL_SPEED 1024															  // approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED 580															  // approx 20 feet
#define DAMAGE_FOR_FALL_SPEED (float)100 / (PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED) // damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED 200
#define PLAYER_FALL_PUNCH_THRESHHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.

//
// Player PHYSICS FLAGS bits
//
#define PFLAG_ONLADDER (1 << 0)
#define PFLAG_ONSWING (1 << 0)
#define PFLAG_ONTRAIN (1 << 1)
#define PFLAG_ONBARNACLE (1 << 2)
#define PFLAG_DUCKING (1 << 3)	// In the process of ducking, but totally squatted yet
#define PFLAG_USING (1 << 4)	// Using a continuous entity
#define PFLAG_OBSERVER (1 << 5) // player is locked in stationary cam mode. Spectators can move, observers can't.

//
// generic player
//
//-----------------------------------------------------
//This is Half-Life player entity
//-----------------------------------------------------
#define CSUITPLAYLIST 4 // max of 4 suit sentences queued up at any time

#define SUIT_REPEAT_OK 0
#define SUIT_NEXT_IN_30SEC 30
#define SUIT_NEXT_IN_1MIN 60
#define SUIT_NEXT_IN_5MIN 300
#define SUIT_NEXT_IN_10MIN 600
#define SUIT_NEXT_IN_30MIN 1800
#define SUIT_NEXT_IN_1HOUR 3600

#define CSUITNOREPEAT 32

#define SOUND_FLASHLIGHT_ON "items/flashlight1.wav"
#define SOUND_FLASHLIGHT_OFF "items/flashlight1.wav"

#define TEAM_NAME_LENGTH 16

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
} PLAYER_ANIM;

#define MAX_ID_RANGE 2048
#define SBAR_STRING_SIZE 128

enum sbar_data
{
	SBAR_ID_TARGETNAME = 1,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_END,
};

#define CHAT_INTERVAL 1.0f

class CBasePlayer : public CBaseAnimating
{
public:
	// Spectator camera
	void Observer_FindNextPlayer(bool bReverse);
	void Observer_HandleButtons();
	void Observer_SetMode(int iMode);
	void Observer_CheckTarget();
	void Observer_CheckProperties();
	EHANDLE m_hObserverTarget;
	float m_flNextObserverInput;
	int m_iObserverWeapon;	 // weapon of current tracked target
	int m_iObserverLastMode; // last used observer mode
	bool IsObserver() { return 0 != pev->iuser1; }

	int random_seed; // See that is shared between client & server for shared weapons code

	int m_afButtonLast;
	int m_afButtonPressed;
	int m_afButtonReleased;

	EHANDLE m_SndLast;	   // last sound entity to modify player room type
	int m_SndRoomtype = 0; // last roomtype set by sound entity. Defaults to 0 on new maps to disable it by default.
	int m_ClientSndRoomtype;
	float m_flSndRange; // dist from player to sound entity

	float m_flFallVelocity;

	int m_rgItems[MAX_ITEMS];
	bool m_fKnownItem; // True when a new item needs to be added
	int m_fNewAmmo;	   // True when a new item has been added

	unsigned int m_afPhysicsFlags; // physics flags - set when 'normal' physics should be revisited or overriden
	float m_fNextSuicideTime;	   // the time after which the player can next use the suicide command


	// these are time-sensitive things that we keep track of
	float m_flTimeWeaponIdle; // when to play another weapon idle animation.
	float m_flSwimTime;		  // how long player has been underwater
	float m_flDuckTime;		  // how long we've been ducking
	float m_flWallJumpTime;	  // how long until next walljump

	float m_flSuitUpdate;						 // when to play next suit update
	int m_rgSuitPlayList[CSUITPLAYLIST];		 // next sentencenum to play for suit update
	int m_iSuitPlayNext;						 // next sentence slot for queue storage;
	int m_rgiSuitNoRepeat[CSUITNOREPEAT];		 // suit sentence no repeat list
	float m_rgflSuitNoRepeatTime[CSUITNOREPEAT]; // how long to wait before allowing repeat
	int m_lastDamageAmount;						 // Last damage taken
	float m_tbdPrev;							 // Time-based damage timer

	float m_flgeigerRange; // range to nearest radiation source
	float m_flgeigerDelay; // delay per update of range msg to client
	int m_igeigerRangePrev;
	char m_szTextureName[CBTEXTURENAMEMAX]; // current texture name we're standing on
	char m_chTextureType;					// current texture type

	int m_idrowndmg;	  // track drowning damage taken
	int m_idrownrestored; // track drowning damage restored

	int m_bitsHUDDamage; // Damage bits for the current fame. These get sent to
						 // the hude via the DAMAGE message
	bool m_fInitHUD;	 // True when deferred HUD restart msg needs to be sent
	bool m_fGameHUDInitialized;
	int m_iTrain;	// Train control position

	EHANDLE m_pTank;		 // the tank which the player is currently controlling,  NULL if no tank
	EHANDLE m_hViewEntity;	 // The view entity being used, or null if the player is using itself as the view entity
	bool m_bResetViewEntity; //True if the player's view needs to be set back to the view entity
	float m_fDeadTime;		 // the time at which the player died  (used in PlayerDeathFrame())

	bool m_fLongJump;	   // does this player have the longjump module?

	int m_iHideHUD;		  // the players hud weapon info is to be hidden
	int m_iClientHideHUD;
	int m_iFOV;		  // field of view
	// usable player items
	CBasePlayerItem* m_rgpPlayerItems[MAX_WEAPONS];
	std::forward_list<CBasePlayerItem *> m_lpPlayerItems;
	CBasePlayerItem* m_pActiveItem;
	CBasePlayerItem* m_pClientActiveItem; // client version of the active item

	std::uint64_t m_WeaponBits;

	//Not saved, used to update client.
	std::uint64_t m_ClientWeaponBits;

	// shared ammo slots
	int m_rgAmmo[MAX_AMMO_SLOTS];
	int m_rgAmmoLast[MAX_AMMO_SLOTS];

	Vector m_vecAutoAim;
	bool m_fOnTarget;
	int m_iDeaths;

	int m_lastx, m_lasty; // These are the previous update's crosshair angles, DON"T SAVE/RESTORE

	int m_nCustomSprayFrames = -1; // Custom clan logo frames for this player
	float m_flNextDecalTime;  // next time this player can spray a decal

	char m_szTeamName[TEAM_NAME_LENGTH];

	void Spawn() override;
	void Pain();

	//	void Think() override;
	virtual void Jump();
	virtual void Duck();
	virtual void PreThink();
	virtual void PostThink();
	inline Vector GetGunPosition() { return pev->origin + pev->view_ofs; }
	bool TakeHealth(float flHealth, int bitsDamageType) override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	void Killed(entvars_t* pevInflictor, entvars_t* pevAttacker, int iGib) override;
	Vector BodyTarget(const Vector& posSrc) override { return Center() + pev->view_ofs * 0.8; } // position to shoot at
	bool IsAlive() override { return (pev->deadflag == DEAD_NO) && pev->health > 0; }
	bool IsPlayer() override { return true; } // Spectators should return false for this, they aren't "players" as far as game logic is concerned

	bool IsNetClient() override { return true; } // Bots should return false for this, they can't receive NET messages
												 // Spectators should return true for this
	const char* TeamID() override;
	int TeamNumber() override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	void PackDeadPlayerItems();
	void RemoveAllItems(bool removeSuit);
	bool SwitchWeapon(CBasePlayerItem* pWeapon);

	/**
	*	@brief Equips an appropriate weapon for the player if they don't have one equipped already.
	*/
	void EquipWeapon();

	void SetWeaponBit(int id);
	void ClearWeaponBit(int id);

	bool HasSuit() const;
	void SetHasSuit(bool hasSuit);

	// JOHN:  sends custom messages if player HUD data has changed  (eg health, ammo)
	virtual void UpdateClientData();

	static TYPEDESCRIPTION m_playerSaveData[];

	// Player is moved across the transition by other means
	int ObjectCaps() override { return CBaseAnimating::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void Precache() override;
	bool IsOnLadder();

	void DeathSound();

	int Classify() override;
	void SetAnimation(PLAYER_ANIM playerAnim);
	Activity CBasePlayer::GetDeathActivity();
	Activity CBasePlayer::GetSmallFlinchActivity();
	void SetWeaponAnimType(const char* szExtention);
	char m_szAnimExtention[32];

	// custom player functions
	virtual void ImpulseCommands();
	void CheatImpulseCommands(int iImpulse);

	void StartDeathCam();
	void StartObserver(Vector vecPosition, Vector vecViewAngle);

	void AddPoints(int score, bool bAllowNegativeScore) override;
	void AddPointsToTeam(int score, bool bAllowNegativeScore) override;
	bool AddPlayerItem(CBasePlayerItem* pItem) override;
	bool RemovePlayerItem(CBasePlayerItem* pItem) override;
	void DropPlayerItem(char* pszItemName);
	bool HasPlayerItem(CBasePlayerItem* pCheckItem);
	bool HasNamedPlayerItem(const char* pszItemName);
	bool HasWeapons(); // do I have ANY weapons?
	void SelectItem(const char* pstr);
	void SelectItem(int iId);
	void ItemPreFrame();
	void ItemPostFrame();
	void GiveNamedItem(const char* szName);
	void GiveNamedItem(const char* szName, int defaultAmmo);
	void EnableControl(bool fControl);

	int GiveAmmo(int iAmount, int iType, int iMax) override;

	void PlaybackEvent(
		unsigned short event,
		float fParam1 = 0.0F,
		float fParam2 = 0.0F,
		int iParam1 = 0,
		int iParam2 = 0,
		qboolean bParam1 = false,
		qboolean bParam2 = false,
		unsigned int flags = FEV_NOTHOST);

public:
	void WaterMove();
	void PlayerDeathFrame();
	void PlayerUse();

	void CheckSuitUpdate();
	void SetSuitUpdate(const char* name, bool fgroup, int iNoRepeat);
	void UpdateGeigerCounter();
	void CheckTimeBasedDamage();
	void CheckAmmoLevel(CBasePlayerItem* pItem, bool bPrimary = true);

	int AmmoInventory(int iAmmoIndex);

	Vector GetAimVector();

	void ForceClientDllUpdate(); // Forces all client .dll specific data to be resent to client.

	void SetCustomDecalFrames(int nFrames);
	int GetCustomDecalFrames();

	float m_flStartCharge;
	float m_flAmmoStartCharge;
	float m_flNextAmmoBurn; // while charging, when to absorb another unit of player's ammo?

	//Player ID
	void InitStatusBar();
	void UpdateStatusBar();
	int m_izSBarState[SBAR_END];
	float m_flNextSBarUpdateTime;
	float m_flStatusBarDisappearDelay;
	char m_SbarString0[SBAR_STRING_SIZE];
	char m_SbarString1[SBAR_STRING_SIZE];

	float m_flNextChatTime;

	void SetPrefsFromUserinfo(char* infobuffer);

	int m_iAutoWepSwitch;

	bool m_bRestored;

	//True if the player is currently spawning.
	bool m_bIsSpawning = false;

	int m_bitsDamageType; // what types of damage has monster (player) taken
	byte m_rgbTimeBasedDamage[CDMG_TIMEBASED];

	Activity m_Activity;	  // what the monster is doing (animation)
	Activity m_IdealActivity; // monster should switch to this activity

	int m_LastHitGroup; // the last body region that took damage
};

inline void CBasePlayer::SetWeaponBit(int id)
{
	m_WeaponBits |= 1ULL << id;
}

inline void CBasePlayer::ClearWeaponBit(int id)
{
	m_WeaponBits &= ~(1ULL << id);
}

inline bool CBasePlayer::HasSuit() const
{
	return (m_WeaponBits & (1ULL << WEAPON_SUIT)) != 0;
}

inline void CBasePlayer::SetHasSuit(bool hasSuit)
{
	if (hasSuit)
	{
		SetWeaponBit(WEAPON_SUIT);
	}
	else
	{
		ClearWeaponBit(WEAPON_SUIT);
	}
}

inline bool gInitHUD = true;
inline bool gEvilImpulse101 = false;

inline DLL_GLOBAL CBaseEntity* g_pLastSpawn = nullptr;