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
//=========================================================
// sound.cpp
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "pm_defs.h"
#include "pm_materials.h"
#include "pm_shared.h"
#include "UserMessages.h"

#define AMBIENT_SOUND_EVERYWHERE 1
#define AMBIENT_SOUND_SMALLRADIUS 2
#define AMBIENT_SOUND_MEDIUMRADIUS 4
#define AMBIENT_SOUND_LARGERADIUS 8
#define AMBIENT_SOUND_START_SILENT 16
#define AMBIENT_SOUND_NOT_LOOPING 32

#define SPEAKER_START_SILENT 1 // wait for trigger 'on' to start announcements

#define LFO_SQUARE 1
#define LFO_TRIANGLE 2
#define LFO_RANDOM 3

static char* memfgets(byte* pMemFile, int fileSize, int& filePos, char* pBuffer, int bufferSize);

static void EmitAmbientSound(
	CBaseEntity* entity,
	const Vector& vecOrigin,
	const char* samp,
	float vol,
	float attenuation,
	int fFlags,
	int pitch)
{
	if (samp != nullptr && samp[0] == '!')
	{
		char name[32];
		if (SENTENCEG_Lookup(samp, name) >= 0)
		{
			engine::EmitAmbientSound(
				&entity->v,
				const_cast<Vector&>(vecOrigin),
				name,
				vol,
				attenuation,
				fFlags,
				pitch);
		}
	}
	else
	{
		engine::EmitAmbientSound(
			&entity->v,
			const_cast<Vector&>(vecOrigin),
			samp,
			vol,
			attenuation,
			fFlags,
			pitch);
	}
}


// ==================== GENERIC AMBIENT SOUND ======================================

// runtime pitch shift and volume fadein/out structure

// NOTE: IF YOU CHANGE THIS STRUCT YOU MUST CHANGE THE SAVE/RESTORE VERSION NUMBER
// SEE BELOW (in the typedescription for the class)
typedef struct dynpitchvol
{
	// NOTE: do not change the order of these parameters
	// NOTE: unless you also change order of rgdpvpreset array elements!
	int preset;

	int pitchrun;	// pitch shift % when sound is running 0 - 255
	int pitchstart; // pitch shift % when sound stops or starts 0 - 255
	int spinup;		// spinup time 0 - 100
	int spindown;	// spindown time 0 - 100

	int volrun;	  // volume change % when sound is running 0 - 10
	int volstart; // volume change % when sound stops or starts 0 - 10
	int fadein;	  // volume fade in time 0 - 100
	int fadeout;  // volume fade out time 0 - 100

	// Low Frequency Oscillator
	int lfotype; // 0) off 1) square 2) triangle 3) random
	int lforate; // 0 - 1000, how fast lfo osciallates

	int lfomodpitch; // 0-100 mod of current pitch. 0 is off.
	int lfomodvol;	 // 0-100 mod of current volume. 0 is off.

	int cspinup; // each trigger hit increments counter and spinup pitch


	int cspincount;

	int pitch;
	int spinupsav;
	int spindownsav;
	int pitchfrac;

	int vol;
	int fadeinsav;
	int fadeoutsav;
	int volfrac;

	int lfofrac;
	int lfomult;


} dynpitchvol_t;

#define CDPVPRESETMAX 27

// presets for runtime pitch and vol modulation of ambient sounds

dynpitchvol_t rgdpvpreset[CDPVPRESETMAX] =
	{
		// pitch	pstart	spinup	spindwn	volrun	volstrt	fadein	fadeout	lfotype	lforate	modptch modvol	cspnup
		{1, 255, 75, 95, 95, 10, 1, 50, 95, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{2, 255, 85, 70, 88, 10, 1, 20, 88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{3, 255, 100, 50, 75, 10, 1, 10, 75, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{4, 100, 100, 0, 0, 10, 1, 90, 90, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{5, 100, 100, 0, 0, 10, 1, 80, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{6, 100, 100, 0, 0, 10, 1, 50, 70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{7, 100, 100, 0, 0, 5, 1, 40, 50, 1, 50, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{8, 100, 100, 0, 0, 5, 1, 40, 50, 1, 150, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{9, 100, 100, 0, 0, 5, 1, 40, 50, 1, 750, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{10, 128, 100, 50, 75, 10, 1, 30, 40, 2, 8, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{11, 128, 100, 50, 75, 10, 1, 30, 40, 2, 25, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{12, 128, 100, 50, 75, 10, 1, 30, 40, 2, 70, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{13, 50, 50, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{14, 70, 70, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{15, 90, 90, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{16, 120, 120, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{17, 180, 180, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{18, 255, 255, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{19, 200, 75, 90, 90, 10, 1, 50, 90, 2, 100, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{20, 255, 75, 97, 90, 10, 1, 50, 90, 1, 40, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{21, 100, 100, 0, 0, 10, 1, 30, 50, 3, 15, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{22, 160, 160, 0, 0, 10, 1, 50, 50, 3, 500, 25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{23, 255, 75, 88, 0, 10, 1, 40, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{24, 200, 20, 95, 70, 10, 1, 70, 70, 3, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{25, 180, 100, 50, 60, 10, 1, 40, 60, 2, 90, 100, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{26, 60, 60, 0, 0, 10, 1, 40, 70, 3, 80, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{27, 128, 90, 10, 10, 10, 1, 20, 40, 1, 5, 10, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

class CAmbientGeneric : public CBaseEntity
{
public:
	CAmbientGeneric(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool KeyValue(KeyValueData* pkvd) override;
	bool Spawn() override;
	void Precache() override;
	void EXPORT ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT RampThink();
	void InitModulationParms();

	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

	float m_flAttenuation; // attenuation value
	dynpitchvol_t m_dpv;

	bool m_fActive;	 // only true when the entity is playing a looping sound
	bool m_fLooping; // true when the sound played will loop
};

LINK_ENTITY_TO_CLASS(ambient_generic, CAmbientGeneric);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CAmbientGeneric)
	DEFINE_FIELD(CAmbientGeneric, m_flAttenuation, FIELD_FLOAT),
	DEFINE_FIELD(CAmbientGeneric, m_fActive, FIELD_BOOLEAN),
	DEFINE_FIELD(CAmbientGeneric, m_fLooping, FIELD_BOOLEAN),
	DEFINE_ARRAY(CAmbientGeneric, m_dpv, FIELD_CHARACTER, sizeof(dynpitchvol_t)),
END_SAVERESTORE(CAmbientGeneric, CBaseEntity)
#endif

//
// ambient_generic - general-purpose user-defined static sound
//
bool CAmbientGeneric::Spawn()
{
	if (FBitSet(v.spawnflags, AMBIENT_SOUND_EVERYWHERE))
	{
		m_flAttenuation = ATTN_NONE;
	}
	else if (FBitSet(v.spawnflags, AMBIENT_SOUND_SMALLRADIUS))
	{
		m_flAttenuation = ATTN_IDLE;
	}
	else if (FBitSet(v.spawnflags, AMBIENT_SOUND_MEDIUMRADIUS))
	{
		m_flAttenuation = ATTN_STATIC;
	}
	else if (FBitSet(v.spawnflags, AMBIENT_SOUND_LARGERADIUS))
	{
		m_flAttenuation = ATTN_NORM;
	}
	else
	{ // if the designer didn't set a sound attenuation, default to one.
		m_flAttenuation = ATTN_STATIC;
	}

	char* szSoundFile = (char*)STRING(v.message);

	if (FStringNull(v.message) || strlen(szSoundFile) < 1)
	{
		engine::AlertMessage(at_error, "EMPTY AMBIENT AT: %f, %f, %f\n", v.origin.x, v.origin.y, v.origin.z);
		return false;
	}
	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NONE;

	// Set up think function for dynamic modification
	// of ambient sound's pitch or volume. Don't
	// start thinking yet.

	SetThink(&CAmbientGeneric::RampThink);
	v.nextthink = 0;

	// allow on/off switching via 'use' function.

	SetUse(&CAmbientGeneric::ToggleUse);

	m_fActive = false;

	if (FBitSet(v.spawnflags, AMBIENT_SOUND_NOT_LOOPING))
		m_fLooping = false;
	else
		m_fLooping = true;
	Precache();

	return true;
}


void CAmbientGeneric::Precache()
{
	char* szSoundFile = (char*)STRING(v.message);

	if (!FStringNull(v.message) && strlen(szSoundFile) > 1)
	{
		if (*szSoundFile != '!')
			engine::PrecacheSound(szSoundFile);
	}
	// init all dynamic modulation parms
	InitModulationParms();

	if (!FBitSet(v.spawnflags, AMBIENT_SOUND_START_SILENT))
	{
		// start the sound ASAP
		if (m_fLooping)
			m_fActive = true;
	}
	if (m_fActive)
	{
		EmitAmbientSound(this, v.origin, szSoundFile,
			(m_dpv.vol * 0.01), m_flAttenuation, SND_SPAWNING, m_dpv.pitch);

		v.nextthink = gpGlobals->time + 0.1;
	}
}

// RampThink - Think at 5hz if we are dynamically modifying
// pitch or volume of the playing sound.  This function will
// ramp pitch and/or volume up or down, modify pitch/volume
// with lfo if active.

void CAmbientGeneric::RampThink()
{
	char* szSoundFile = (char*)STRING(v.message);
	int pitch = m_dpv.pitch;
	int vol = m_dpv.vol;
	int flags = 0;
	bool fChanged = false; // false if pitch and vol remain unchanged this round
	int prev;

	if (0 == m_dpv.spinup && 0 == m_dpv.spindown && 0 == m_dpv.fadein && 0 == m_dpv.fadeout && 0 == m_dpv.lfotype)
		return; // no ramps or lfo, stop thinking

	// ==============
	// pitch envelope
	// ==============
	if (0 != m_dpv.spinup || 0 != m_dpv.spindown)
	{
		prev = m_dpv.pitchfrac >> 8;

		if (m_dpv.spinup > 0)
			m_dpv.pitchfrac += m_dpv.spinup;
		else if (m_dpv.spindown > 0)
			m_dpv.pitchfrac -= m_dpv.spindown;

		pitch = m_dpv.pitchfrac >> 8;

		if (pitch > m_dpv.pitchrun)
		{
			pitch = m_dpv.pitchrun;
			m_dpv.spinup = 0; // done with ramp up
		}

		if (pitch < m_dpv.pitchstart)
		{
			pitch = m_dpv.pitchstart;
			m_dpv.spindown = 0; // done with ramp down

			// shut sound off
			EmitAmbientSound(this, v.origin, szSoundFile,
				0, 0, SND_STOP, 0);

			// return without setting nextthink
			return;
		}

		if (pitch > 255)
			pitch = 255;
		if (pitch < 1)
			pitch = 1;

		m_dpv.pitch = pitch;

		fChanged |= (prev != pitch);
		flags |= SND_CHANGE_PITCH;
	}

	// ==================
	// amplitude envelope
	// ==================
	if (0 != m_dpv.fadein || 0 != m_dpv.fadeout)
	{
		prev = m_dpv.volfrac >> 8;

		if (m_dpv.fadein > 0)
			m_dpv.volfrac += m_dpv.fadein;
		else if (m_dpv.fadeout > 0)
			m_dpv.volfrac -= m_dpv.fadeout;

		vol = m_dpv.volfrac >> 8;

		if (vol > m_dpv.volrun)
		{
			vol = m_dpv.volrun;
			m_dpv.fadein = 0; // done with ramp up
		}

		if (vol < m_dpv.volstart)
		{
			vol = m_dpv.volstart;
			m_dpv.fadeout = 0; // done with ramp down

			// shut sound off
			EmitAmbientSound(this, v.origin, szSoundFile,
				0, 0, SND_STOP, 0);

			// return without setting nextthink
			return;
		}

		if (vol > 100)
			vol = 100;
		if (vol < 1)
			vol = 1;

		m_dpv.vol = vol;

		fChanged |= (prev != vol);
		flags |= SND_CHANGE_VOL;
	}

	// ===================
	// pitch/amplitude LFO
	// ===================
	if (0 != m_dpv.lfotype)
	{
		int pos;

		if (m_dpv.lfofrac > 0x6fffffff)
			m_dpv.lfofrac = 0;

		// update lfo, lfofrac/255 makes a triangle wave 0-255
		m_dpv.lfofrac += m_dpv.lforate;
		pos = m_dpv.lfofrac >> 8;

		if (m_dpv.lfofrac < 0)
		{
			m_dpv.lfofrac = 0;
			m_dpv.lforate = abs(m_dpv.lforate);
			pos = 0;
		}
		else if (pos > 255)
		{
			pos = 255;
			m_dpv.lfofrac = (255 << 8);
			m_dpv.lforate = -abs(m_dpv.lforate);
		}

		switch (m_dpv.lfotype)
		{
		case LFO_SQUARE:
			if (pos < 128)
				m_dpv.lfomult = 255;
			else
				m_dpv.lfomult = 0;

			break;
		case LFO_RANDOM:
			if (pos == 255)
				m_dpv.lfomult = engine::RandomLong(0, 255);
			break;
		case LFO_TRIANGLE:
		default:
			m_dpv.lfomult = pos;
			break;
		}

		if (0 != m_dpv.lfomodpitch)
		{
			prev = pitch;

			// pitch 0-255
			pitch += ((m_dpv.lfomult - 128) * m_dpv.lfomodpitch) / 100;

			if (pitch > 255)
				pitch = 255;
			if (pitch < 1)
				pitch = 1;


			fChanged |= (prev != pitch);
			flags |= SND_CHANGE_PITCH;
		}

		if (0 != m_dpv.lfomodvol)
		{
			// vol 0-100
			prev = vol;

			vol += ((m_dpv.lfomult - 128) * m_dpv.lfomodvol) / 100;

			if (vol > 100)
				vol = 100;
			if (vol < 0)
				vol = 0;

			fChanged |= (prev != vol);
			flags |= SND_CHANGE_VOL;
		}
	}

	// Send update to playing sound only if we actually changed
	// pitch or volume in this routine.

	if (0 != flags && fChanged)
	{
		if (pitch == PITCH_NORM)
			pitch = PITCH_NORM + 1; // don't send 'no pitch' !

		EmitAmbientSound(this, v.origin, szSoundFile,
			(vol * 0.01), m_flAttenuation, flags, pitch);
	}

	// update ramps at 5hz
	v.nextthink = gpGlobals->time + 0.2;
	return;
}

// Init all ramp params in preparation to
// play a new sound

void CAmbientGeneric::InitModulationParms()
{
	int pitchinc;

	m_dpv.volrun = v.health * 10; // 0 - 100
	if (m_dpv.volrun > 100)
		m_dpv.volrun = 100;
	if (m_dpv.volrun < 0)
		m_dpv.volrun = 0;

	// get presets
	if (m_dpv.preset != 0 && m_dpv.preset <= CDPVPRESETMAX)
	{
		// load preset values
		m_dpv = rgdpvpreset[m_dpv.preset - 1];

		// fixup preset values, just like
		// fixups in KeyValue routine.
		if (m_dpv.spindown > 0)
			m_dpv.spindown = (101 - m_dpv.spindown) * 64;
		if (m_dpv.spinup > 0)
			m_dpv.spinup = (101 - m_dpv.spinup) * 64;

		m_dpv.volstart *= 10;
		m_dpv.volrun *= 10;

		if (m_dpv.fadein > 0)
			m_dpv.fadein = (101 - m_dpv.fadein) * 64;
		if (m_dpv.fadeout > 0)
			m_dpv.fadeout = (101 - m_dpv.fadeout) * 64;

		m_dpv.lforate *= 256;

		m_dpv.fadeinsav = m_dpv.fadein;
		m_dpv.fadeoutsav = m_dpv.fadeout;
		m_dpv.spinupsav = m_dpv.spinup;
		m_dpv.spindownsav = m_dpv.spindown;
	}

	m_dpv.fadein = m_dpv.fadeinsav;
	m_dpv.fadeout = 0;

	if (0 != m_dpv.fadein)
		m_dpv.vol = m_dpv.volstart;
	else
		m_dpv.vol = m_dpv.volrun;

	m_dpv.spinup = m_dpv.spinupsav;
	m_dpv.spindown = 0;

	if (0 != m_dpv.spinup)
		m_dpv.pitch = m_dpv.pitchstart;
	else
		m_dpv.pitch = m_dpv.pitchrun;

	if (m_dpv.pitch == 0)
		m_dpv.pitch = PITCH_NORM;

	m_dpv.pitchfrac = m_dpv.pitch << 8;
	m_dpv.volfrac = m_dpv.vol << 8;

	m_dpv.lfofrac = 0;
	m_dpv.lforate = abs(m_dpv.lforate);

	m_dpv.cspincount = 1;

	if (0 != m_dpv.cspinup)
	{
		pitchinc = (255 - m_dpv.pitchstart) / m_dpv.cspinup;

		m_dpv.pitchrun = m_dpv.pitchstart + pitchinc;
		if (m_dpv.pitchrun > 255)
			m_dpv.pitchrun = 255;
	}

	if ((0 != m_dpv.spinupsav || 0 != m_dpv.spindownsav || (0 != m_dpv.lfotype && 0 != m_dpv.lfomodpitch)) && (m_dpv.pitch == PITCH_NORM))
		m_dpv.pitch = PITCH_NORM + 1; // must never send 'no pitch' as first pitch
									  // if we intend to pitch shift later!
}

//
// ToggleUse - turns an ambient sound on or off.  If the
// ambient is a looping sound, mark sound as active (m_fActive)
// if it's playing, innactive if not.  If the sound is not
// a looping sound, never mark it as active.
//
void CAmbientGeneric::ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	char* szSoundFile = (char*)STRING(v.message);
	float fraction;

	if (useType != USE_TOGGLE)
	{
		if ((m_fActive && useType == USE_ON) || (!m_fActive && useType == USE_OFF))
			return;
	}
	// Directly change pitch if arg passed. Only works if sound is already playing.

	if (useType == USE_SET && m_fActive) // Momentary buttons will pass down a float in here
	{

		fraction = value;

		if (fraction > 1.0)
			fraction = 1.0;
		if (fraction < 0.0)
			fraction = 0.01;

		m_dpv.pitch = fraction * 255;

		EmitAmbientSound(this, v.origin, szSoundFile,
			0, 0, SND_CHANGE_PITCH, m_dpv.pitch);

		return;
	}

	// Toggle

	// m_fActive is true only if a looping sound is playing.

	if (m_fActive)
	{ // turn sound off

		if (0 != m_dpv.cspinup)
		{
			// Don't actually shut off. Each toggle causes
			// incremental spinup to max pitch

			if (m_dpv.cspincount <= m_dpv.cspinup)
			{
				int pitchinc;

				// start a new spinup
				m_dpv.cspincount++;

				pitchinc = (255 - m_dpv.pitchstart) / m_dpv.cspinup;

				m_dpv.spinup = m_dpv.spinupsav;
				m_dpv.spindown = 0;

				m_dpv.pitchrun = m_dpv.pitchstart + pitchinc * m_dpv.cspincount;
				if (m_dpv.pitchrun > 255)
					m_dpv.pitchrun = 255;

				v.nextthink = gpGlobals->time + 0.1;
			}
		}
		else
		{
			m_fActive = false;

			// HACKHACK - this makes the code in Precache() work properly after a save/restore
			v.spawnflags |= AMBIENT_SOUND_START_SILENT;

			if (0 != m_dpv.spindownsav || 0 != m_dpv.fadeoutsav)
			{
				// spin it down (or fade it) before shutoff if spindown is set
				m_dpv.spindown = m_dpv.spindownsav;
				m_dpv.spinup = 0;

				m_dpv.fadeout = m_dpv.fadeoutsav;
				m_dpv.fadein = 0;
				v.nextthink = gpGlobals->time + 0.1;
			}
			else
				EmitAmbientSound(this, v.origin, szSoundFile,
					0, 0, SND_STOP, 0);
		}
	}
	else
	{ // turn sound on

		// only toggle if this is a looping sound.  If not looping, each
		// trigger will cause the sound to play.  If the sound is still
		// playing from a previous trigger press, it will be shut off
		// and then restarted.

		if (m_fLooping)
			m_fActive = true;
		else
			// shut sound off now - may be interrupting a long non-looping sound
			EmitAmbientSound(this, v.origin, szSoundFile,
				0, 0, SND_STOP, 0);

		// init all ramp params for startup

		InitModulationParms();

		EmitAmbientSound(this, v.origin, szSoundFile,
			(m_dpv.vol * 0.01), m_flAttenuation, 0, m_dpv.pitch);

		v.nextthink = gpGlobals->time + 0.1;
	}
}
// KeyValue - load keyvalue pairs into member data of the
// ambient generic. NOTE: called BEFORE spawn!

bool CAmbientGeneric::KeyValue(KeyValueData* pkvd)
{
	// NOTE: changing any of the modifiers in this code
	// NOTE: also requires changing InitModulationParms code.

	// preset
	if (streq(pkvd->szKeyName, "preset"))
	{
		m_dpv.preset = atoi(pkvd->szValue);
		return true;
	}

	// pitchrun
	else if (streq(pkvd->szKeyName, "pitch"))
	{
		m_dpv.pitchrun = atoi(pkvd->szValue);

		if (m_dpv.pitchrun > 255)
			m_dpv.pitchrun = 255;
		if (m_dpv.pitchrun < 0)
			m_dpv.pitchrun = 0;

		return true;
	}

	// pitchstart
	else if (streq(pkvd->szKeyName, "pitchstart"))
	{
		m_dpv.pitchstart = atoi(pkvd->szValue);

		if (m_dpv.pitchstart > 255)
			m_dpv.pitchstart = 255;
		if (m_dpv.pitchstart < 0)
			m_dpv.pitchstart = 0;

		return true;
	}

	// spinup
	else if (streq(pkvd->szKeyName, "spinup"))
	{
		m_dpv.spinup = atoi(pkvd->szValue);

		if (m_dpv.spinup > 100)
			m_dpv.spinup = 100;
		if (m_dpv.spinup < 0)
			m_dpv.spinup = 0;

		if (m_dpv.spinup > 0)
			m_dpv.spinup = (101 - m_dpv.spinup) * 64;
		m_dpv.spinupsav = m_dpv.spinup;
		return true;
	}

	// spindown
	else if (streq(pkvd->szKeyName, "spindown"))
	{
		m_dpv.spindown = atoi(pkvd->szValue);

		if (m_dpv.spindown > 100)
			m_dpv.spindown = 100;
		if (m_dpv.spindown < 0)
			m_dpv.spindown = 0;

		if (m_dpv.spindown > 0)
			m_dpv.spindown = (101 - m_dpv.spindown) * 64;
		m_dpv.spindownsav = m_dpv.spindown;
		return true;
	}

	// volstart
	else if (streq(pkvd->szKeyName, "volstart"))
	{
		m_dpv.volstart = atoi(pkvd->szValue);

		if (m_dpv.volstart > 10)
			m_dpv.volstart = 10;
		if (m_dpv.volstart < 0)
			m_dpv.volstart = 0;

		m_dpv.volstart *= 10; // 0 - 100

		return true;
	}

	// fadein
	else if (streq(pkvd->szKeyName, "fadein"))
	{
		m_dpv.fadein = atoi(pkvd->szValue);

		if (m_dpv.fadein > 100)
			m_dpv.fadein = 100;
		if (m_dpv.fadein < 0)
			m_dpv.fadein = 0;

		if (m_dpv.fadein > 0)
			m_dpv.fadein = (101 - m_dpv.fadein) * 64;
		m_dpv.fadeinsav = m_dpv.fadein;
		return true;
	}

	// fadeout
	else if (streq(pkvd->szKeyName, "fadeout"))
	{
		m_dpv.fadeout = atoi(pkvd->szValue);

		if (m_dpv.fadeout > 100)
			m_dpv.fadeout = 100;
		if (m_dpv.fadeout < 0)
			m_dpv.fadeout = 0;

		if (m_dpv.fadeout > 0)
			m_dpv.fadeout = (101 - m_dpv.fadeout) * 64;
		m_dpv.fadeoutsav = m_dpv.fadeout;
		return true;
	}

	// lfotype
	else if (streq(pkvd->szKeyName, "lfotype"))
	{
		m_dpv.lfotype = atoi(pkvd->szValue);
		if (m_dpv.lfotype > 4)
			m_dpv.lfotype = LFO_TRIANGLE;
		return true;
	}

	// lforate
	else if (streq(pkvd->szKeyName, "lforate"))
	{
		m_dpv.lforate = atoi(pkvd->szValue);

		if (m_dpv.lforate > 1000)
			m_dpv.lforate = 1000;
		if (m_dpv.lforate < 0)
			m_dpv.lforate = 0;

		m_dpv.lforate *= 256;

		return true;
	}
	// lfomodpitch
	else if (streq(pkvd->szKeyName, "lfomodpitch"))
	{
		m_dpv.lfomodpitch = atoi(pkvd->szValue);
		if (m_dpv.lfomodpitch > 100)
			m_dpv.lfomodpitch = 100;
		if (m_dpv.lfomodpitch < 0)
			m_dpv.lfomodpitch = 0;


		return true;
	}

	// lfomodvol
	else if (streq(pkvd->szKeyName, "lfomodvol"))
	{
		m_dpv.lfomodvol = atoi(pkvd->szValue);
		if (m_dpv.lfomodvol > 100)
			m_dpv.lfomodvol = 100;
		if (m_dpv.lfomodvol < 0)
			m_dpv.lfomodvol = 0;

		return true;
	}

	// cspinup
	else if (streq(pkvd->szKeyName, "cspinup"))
	{
		m_dpv.cspinup = atoi(pkvd->szValue);
		if (m_dpv.cspinup > 100)
			m_dpv.cspinup = 100;
		if (m_dpv.cspinup < 0)
			m_dpv.cspinup = 0;

		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


// =================== ROOM SOUND FX ==========================================

class CEnvSound : public CPointEntity
{
public:
	CEnvSound(Entity* containingEntity) : CPointEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool KeyValue(KeyValueData* pkvd) override;
	bool Spawn() override;

	void Think() override;

	float m_flRadius;
	int m_Roomtype;
};

LINK_ENTITY_TO_CLASS(env_sound, CEnvSound);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CEnvSound)
	DEFINE_FIELD(CEnvSound, m_flRadius, FIELD_FLOAT),
	DEFINE_FIELD(CEnvSound, m_Roomtype, FIELD_INTEGER),
END_SAVERESTORE(CEnvSound, CBaseEntity)
#endif


bool CEnvSound::KeyValue(KeyValueData* pkvd)
{

	if (streq(pkvd->szKeyName, "radius"))
	{
		m_flRadius = atof(pkvd->szValue);
		return true;
	}
	if (streq(pkvd->szKeyName, "roomtype"))
	{
		m_Roomtype = atoi(pkvd->szValue);
		return true;
	}

	return false;
}

// returns true if the given sound entity (pev) is in range
// and can see the given player entity (pevTarget)

static bool FEnvSoundInRange(CEnvSound* pSound, Entity* target, float& flRange)
{
	const Vector vecSpot1 = pSound->v.origin + pSound->v.view_ofs;
	const Vector vecSpot2 = target->origin + target->view_ofs;
	TraceResult tr;

	util::TraceLine(vecSpot1, vecSpot2, util::ignore_monsters, pSound, &tr);

	// check if line of sight crosses water boundary, or is blocked

	if ((0 != tr.fInOpen && 0 != tr.fInWater) || tr.flFraction != 1)
		return false;

	// calc range from sound entity to player

	const Vector vecRange = tr.vecEndPos - vecSpot1;
	flRange = vecRange.Length();

	return pSound->m_flRadius >= flRange;
}

//
// A client that is visible and in range of a sound entity will
// have its room_type set by that sound entity.  If two or more
// sound entities are contending for a client, then the nearest
// sound entity to the client will set the client's room_type.
// A client's room_type will remain set to its prior value until
// a new in-range, visible sound entity resets a new room_type.
//

// CONSIDER: if player in water state, autoset roomtype to 14,15 or 16.

void CEnvSound::Think()
{
	const bool shouldThinkFast = [this]()
	{
		// get pointer to client if visible; engine::FindClientInPVS will
		// cycle through visible clients on consecutive calls.
		Entity* pentPlayer = engine::FindClientInPVS(&v);

		if (pentPlayer == nullptr)
			return false; // no player in pvs of sound entity, slow it down

		// check to see if this is the sound entity that is currently affecting this player
		auto pPlayer = pentPlayer->Get<CBasePlayer>();
		float flRange;

		if (pPlayer == nullptr)
			return false;

		if (pPlayer->m_SndLast && pPlayer->m_SndLast == this)
		{
			// this is the entity currently affecting player, check for validity
			if (pPlayer->m_SndRoomtype != 0 && pPlayer->m_flSndRange != 0)
			{
				// we're looking at a valid sound entity affecting
				// player, make sure it's still valid, update range
				if (FEnvSoundInRange(this, pentPlayer, flRange))
				{
					pPlayer->m_flSndRange = flRange;
					return true;
				}
				else
				{
					// current sound entity affecting player is no longer valid,
					// flag this state by clearing sound handle and range.
					// NOTE: we do not actually change the player's room_type
					// NOTE: until we have a new valid room_type to change it to.

					pPlayer->m_SndLast = nullptr;
					pPlayer->m_flSndRange = 0;
				}
			}

			// entity is affecting player but is out of range,
			// wait passively for another entity to usurp it...
			return false;
		}

		// if we got this far, we're looking at an entity that is contending
		// for current player sound. the closest entity to player wins.
		if (FEnvSoundInRange(this, pentPlayer, flRange))
		{
			if (flRange < pPlayer->m_flSndRange || pPlayer->m_flSndRange == 0)
			{
				// new entity is closer to player, so it wins.
				pPlayer->m_SndLast = this;
				pPlayer->m_SndRoomtype = m_Roomtype;
				pPlayer->m_flSndRange = flRange;

				// New room type is sent to player in CBasePlayer::UpdateClientData.

				// crank up nextthink rate for new active sound entity
			}
			// player is not closer to the contending sound entity.
			// this effectively cranks up the think rate of env_sound entities near the player.
		}

		// player is in pvs of sound entity, but either not visible or not in range. do nothing.

		return true;
	}();

	v.nextthink = gpGlobals->time + (shouldThinkFast ? 0.25 : 0.75);
}

//
// env_sound - spawn a sound entity that will set player roomtype
// when player moves in range and sight.
//
//
bool CEnvSound::Spawn()
{
	// spread think times
	v.nextthink = gpGlobals->time + engine::RandomFloat(0.0, 0.5);
	return true;
}

// ==================== SENTENCE GROUPS, UTILITY FUNCTIONS  ======================================

#define CSENTENCE_LRU_MAX 32 // max number of elements per sentence group

// group of related sentences

typedef struct sentenceg
{
	char szgroupname[CBSENTENCENAME_MAX];
	int count;
	unsigned char rgblru[CSENTENCE_LRU_MAX];

} SENTENCEG;

#define CSENTENCEG_MAX 200 // max number of sentence groups
// globals

SENTENCEG rgsentenceg[CSENTENCEG_MAX];
bool fSentencesInit = false;

char gszallsentencenames[CVOXFILESENTENCEMAX][CBSENTENCENAME_MAX];
int gcallsentences = 0;

// randomize list of sentence name indices

void USENTENCEG_InitLRU(unsigned char* plru, int count)
{
	int i, j, k;
	unsigned char temp;

	if (!fSentencesInit)
		return;

	if (count > CSENTENCE_LRU_MAX)
		count = CSENTENCE_LRU_MAX;

	for (i = 0; i < count; i++)
		plru[i] = (unsigned char)i;

	// randomize array
	for (i = 0; i < (count * 4); i++)
	{
		j = engine::RandomLong(0, count - 1);
		k = engine::RandomLong(0, count - 1);
		temp = plru[j];
		plru[j] = plru[k];
		plru[k] = temp;
	}
}

// ignore lru. pick next sentence from sentence group. Go in order until we hit the last sentence,
// then repeat list if freset is true.  If freset is false, then repeat last sentence.
// ipick is passed in as the requested sentence ordinal.
// ipick 'next' is returned.
// return of -1 indicates an error.

int USENTENCEG_PickSequential(int isentenceg, char* szfound, int ipick, bool freset)
{
	char* szgroupname;
	unsigned char count;
	char sznum[8];

	if (!fSentencesInit)
		return -1;

	if (isentenceg < 0)
		return -1;

	szgroupname = rgsentenceg[isentenceg].szgroupname;
	count = rgsentenceg[isentenceg].count;

	if (count == 0)
		return -1;

	if (ipick >= count)
		ipick = count - 1;

	strcpy(szfound, "!");
	strcat(szfound, szgroupname);
	sprintf(sznum, "%d", ipick);
	strcat(szfound, sznum);

	if (ipick >= count)
	{
		if (freset)
			// reset at end of list
			return 0;
		else
			return count;
	}

	return ipick + 1;
}



// pick a random sentence from rootname0 to rootnameX.
// picks from the rgsentenceg[isentenceg] least
// recently used, modifies lru array. returns the sentencename.
// note, lru must be seeded with 0-n randomized sentence numbers, with the
// rest of the lru filled with -1. The first integer in the lru is
// actually the size of the list.  Returns ipick, the ordinal
// of the picked sentence within the group.

int USENTENCEG_Pick(int isentenceg, char* szfound)
{
	char* szgroupname;
	unsigned char* plru;
	unsigned char i;
	unsigned char count;
	char sznum[8];
	unsigned char ipick;
	bool ffound = false;

	if (!fSentencesInit)
		return -1;

	if (isentenceg < 0)
		return -1;

	szgroupname = rgsentenceg[isentenceg].szgroupname;
	count = rgsentenceg[isentenceg].count;
	plru = rgsentenceg[isentenceg].rgblru;

	while (!ffound)
	{
		for (i = 0; i < count; i++)
			if (plru[i] != 0xFF)
			{
				ipick = plru[i];
				plru[i] = 0xFF;
				ffound = true;
				break;
			}

		if (!ffound)
			USENTENCEG_InitLRU(plru, count);
		else
		{
			strcpy(szfound, "!");
			strcat(szfound, szgroupname);
			sprintf(sznum, "%d", ipick);
			strcat(szfound, sznum);
			return ipick;
		}
	}
	return -1;
}

// ===================== SENTENCE GROUPS, MAIN ROUTINES ========================

// Given sentence group rootname (name without number suffix),
// get sentence group index (isentenceg). Returns -1 if no such name.

int SENTENCEG_GetIndex(const char* szgroupname)
{
	int i;

	if (!fSentencesInit || !szgroupname)
		return -1;

	// search rgsentenceg for match on szgroupname

	i = 0;
	while (0 != rgsentenceg[i].count)
	{
		if (0 == strcmp(szgroupname, rgsentenceg[i].szgroupname))
			return i;
		i++;
	}

	return -1;
}

// given sentence group index, play random sentence for given entity.
// returns ipick - which sentence was picked to
// play from the group. Ipick is only needed if you plan on stopping
// the sound before playback is done (see SENTENCEG_Stop).

int SENTENCEG_PlayRndI(CBaseEntity* entity, int isentenceg,
	float volume, float attenuation, int flags, int pitch)
{
	char name[64];
	int ipick;

	if (!fSentencesInit)
		return -1;

	name[0] = 0;

	ipick = USENTENCEG_Pick(isentenceg, name);
	if (ipick > 0 && name)
		entity->EmitSound(name, CHAN_VOICE, volume, attenuation, pitch, flags);
	return ipick;
}

// same as above, but takes sentence group name instead of index

int SENTENCEG_PlayRndSz(CBaseEntity* entity, const char* szgroupname,
	float volume, float attenuation, int flags, int pitch)
{
	char name[64];
	int ipick;
	int isentenceg;

	if (!fSentencesInit)
		return -1;

	name[0] = 0;

	isentenceg = SENTENCEG_GetIndex(szgroupname);
	if (isentenceg < 0)
	{
		engine::AlertMessage(at_console, "No such sentence group %s\n", szgroupname);
		return -1;
	}

	ipick = USENTENCEG_Pick(isentenceg, name);
	if (ipick >= 0 && '\0' != name[0])
		entity->EmitSound(name, CHAN_VOICE, volume, attenuation, pitch, flags);

	return ipick;
}

// play sentences in sequential order from sentence group.  Reset after last sentence.

int SENTENCEG_PlaySequentialSz(CBaseEntity* entity, const char* szgroupname,
	float volume, float attenuation, int flags, int pitch, int ipick, bool freset)
{
	char name[64];
	int ipicknext;
	int isentenceg;

	if (!fSentencesInit)
		return -1;

	name[0] = 0;

	isentenceg = SENTENCEG_GetIndex(szgroupname);
	if (isentenceg < 0)
		return -1;

	ipicknext = USENTENCEG_PickSequential(isentenceg, name, ipick, freset);
	if (ipicknext >= 0 && '\0' != name[0])
		entity->EmitSound(name, CHAN_VOICE, volume, attenuation, pitch, flags);
	return ipicknext;
}


// for this entity, for the given sentence within the sentence group, stop
// the sentence.

void SENTENCEG_Stop(CBaseEntity* entity, int isentenceg, int ipick)
{
	char buffer[64];
	char sznum[8];

	if (!fSentencesInit)
		return;

	if (isentenceg < 0 || ipick < 0)
		return;

	strcpy(buffer, "!");
	strcat(buffer, rgsentenceg[isentenceg].szgroupname);
	sprintf(sznum, "%d", ipick);
	strcat(buffer, sznum);

	entity->StopSound(buffer, CHAN_VOICE);
}

// open sentences.txt, scan for groups, build rgsentenceg
// Should be called from world spawn, only works on the
// first call and is ignored subsequently.

void SENTENCEG_Init()
{
	char buffer[512]{};
	char szgroup[64]{};
	int i, j;
	int isentencegs;

	if (fSentencesInit)
		return;

	memset(gszallsentencenames, 0, CVOXFILESENTENCEMAX * CBSENTENCENAME_MAX);
	gcallsentences = 0;

	memset(rgsentenceg, 0, CSENTENCEG_MAX * sizeof(SENTENCEG));
	isentencegs = -1;


	int filePos = 0, fileSize;
	byte* pMemFile = engine::LoadFileForMe("sound/sentences.txt", &fileSize);
	if (!pMemFile)
		return;

	// for each line in the file...
	while (memfgets(pMemFile, fileSize, filePos, buffer, 511) != nullptr)
	{
		// skip whitespace
		i = 0;
		while ('\0' != buffer[i] && buffer[i] == ' ')
			i++;

		if ('\0' == buffer[i])
			continue;

		if (buffer[i] == '/' || 0 == isalpha(buffer[i]))
			continue;

		// get sentence name
		j = i;
		while ('\0' != buffer[j] && buffer[j] != ' ')
			j++;

		if ('\0' == buffer[j])
			continue;

		if (gcallsentences >= CVOXFILESENTENCEMAX)
		{
			engine::AlertMessage(at_error, "Too many sentences in sentences.txt!\n");
			break;
		}

		// null-terminate name and save in sentences array
		buffer[j] = 0;
		const char* pString = buffer + i;

		if (strlen(pString) >= CBSENTENCENAME_MAX)
			engine::AlertMessage(at_warning, "Sentence %s longer than %d letters\n", pString, CBSENTENCENAME_MAX - 1);

		strcpy(gszallsentencenames[gcallsentences++], pString);

		j--;
		if (j <= i)
			continue;
		if (0 == isdigit(buffer[j]))
			continue;

		// cut out suffix numbers
		while (j > i && 0 != isdigit(buffer[j]))
			j--;

		if (j <= i)
			continue;

		buffer[j + 1] = 0;

		// if new name doesn't match previous group name,
		// make a new group.

		if (0 != strcmp(szgroup, &(buffer[i])))
		{
			// name doesn't match with prev name,
			// copy name into group, init count to 1
			isentencegs++;
			if (isentencegs >= CSENTENCEG_MAX)
			{
				engine::AlertMessage(at_error, "Too many sentence groups in sentences.txt!\n");
				break;
			}

			strcpy(rgsentenceg[isentencegs].szgroupname, &(buffer[i]));
			rgsentenceg[isentencegs].count = 1;

			strcpy(szgroup, &(buffer[i]));

			continue;
		}
		else
		{
			//name matches with previous, increment group count
			if (isentencegs >= 0)
				rgsentenceg[isentencegs].count++;
		}
	}

	engine::FreeFile(pMemFile);

	fSentencesInit = true;

	// init lru lists

	i = 0;

	while (0 != rgsentenceg[i].count && i < CSENTENCEG_MAX)
	{
		USENTENCEG_InitLRU(&(rgsentenceg[i].rgblru[0]), rgsentenceg[i].count);
		i++;
	}
}

// convert sentence (sample) name to !sentencenum, return !sentencenum

int SENTENCEG_Lookup(const char* sample, char* sentencenum)
{
	char sznum[32];

	int i;
	// this is a sentence name; lookup sentence number
	// and give to engine as string.
	for (i = 0; i < gcallsentences; i++)
		if (!stricmp(gszallsentencenames[i], sample + 1))
		{
			if (sentencenum)
			{
				strcpy(sentencenum, "!");
				sprintf(sznum, "%d", i);
				strcat(sentencenum, sznum);
			}
			return i;
		}
	// sentence name not found!
	return -1;
}

void CBaseEntity::EmitSound(
	const char* sample,
	int channel,
	float volume,
	float attenuation,
	int pitch,
	int flags)
{
	if (sample[0] == '!')
	{
		char name[32];
		if (SENTENCEG_Lookup(sample, name) >= 0)
		{
			engine::EmitSound(
				&v,
				channel,
				name,
				volume,
				attenuation,
				flags,
				pitch);
		}
		else
		{
			engine::AlertMessage(at_aiconsole, "Unable to find %s in sentences.txt\n", sample);
		}
	}
	else
	{
		engine::EmitSound(
			&v,
			channel,
			sample,
			volume,
			attenuation,
			flags,
			pitch);
	}
}

void CBaseEntity::EmitSoundPredicted(
	const char* sample,
	int channel,
	float volume,
	float attenuation,
	int pitch,
	int flags)
{
	const auto pas = engine::SetFatPAS(v.origin);
	const auto canSkipPlayer = engine::CanSkipPlayer(&v) != 0;

	volume *= 255;
	attenuation *= 127;

	for (auto i = 1; i <= gpGlobals->maxClients; i++)
	{
		auto player = util::PlayerByIndex(i);

		if (player == nullptr || (player == this && canSkipPlayer) || !player->IsNetClient())
		{
			continue;
		}

		if (!engine::CheckVisibility(&player->v, pas))
		{
			continue;
		}

		MessageBegin(MSG_ONE_UNRELIABLE, gmsgPredictedSound, player);
		WriteByte(v.GetIndex());
		WriteByte(channel);
		WriteByte(volume);
		WriteByte(attenuation);
		WriteByte(pitch);
		WriteString(sample);
		MessageEnd();
	}
}

void CBaseEntity::StopSound(const char* sample, int channel)
{
	engine::EmitSound(
		&v,
		channel,
		sample,
		VOL_NORM,
		ATTN_NORM,
		SND_STOP,
		PITCH_NORM);
}

// ===================== MATERIAL TYPE DETECTION, MAIN ROUTINES ========================
//
// Used to detect the texture the player is standing on, map the
// texture name to a material type.  Play footstep sound based
// on material type.

bool fTextureTypeInit = false;

int gcTextures = 0;
char grgszTextureName[CTEXTURESMAX][CBTEXTURENAMEMAX]; // texture names
char grgchTextureType[CTEXTURESMAX];				   // parallel array of texture types

// open materials.txt,  get size, alloc space,
// save in array.  Only works first time called,
// ignored on subsequent calls.

static char* memfgets(byte* pMemFile, int fileSize, int& filePos, char* pBuffer, int bufferSize)
{
	// Bullet-proofing
	if (!pMemFile || !pBuffer)
		return nullptr;

	if (filePos >= fileSize)
		return nullptr;

	int i = filePos;
	int last = fileSize;

	// fgets always nullptr terminates, so only read bufferSize-1 characters
	if (last - filePos > (bufferSize - 1))
		last = filePos + (bufferSize - 1);

	bool stop = false;

	// Stop at the next newline (inclusive) or end of buffer
	while (i < last && !stop)
	{
		if (pMemFile[i] == '\n')
			stop = true;
		i++;
	}


	// If we actually advanced the pointer, copy it over
	if (i != filePos)
	{
		// We read in size bytes
		int size = i - filePos;
		// copy it out
		memcpy(pBuffer, pMemFile + filePos, sizeof(byte) * size);

		// If the buffer isn't full, terminate (this is always true)
		if (size < bufferSize)
			pBuffer[size] = 0;

		// Update file pointer
		filePos = i;
		return pBuffer;
	}

	// No data read, bail
	return nullptr;
}

// ===================================================================================
//
// Speaker class. Used for announcements per level, for door lock/unlock spoken voice.
//

class CSpeaker : public CBaseEntity
{
public:
	CSpeaker(Entity* containingEntity) : CBaseEntity(containingEntity) {}

	DECLARE_SAVERESTORE()

	bool KeyValue(KeyValueData* pkvd) override;
	bool Spawn() override;
	void Precache() override;
	void EXPORT ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT SpeakerThink();

	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

	int m_preset; // preset number
};

LINK_ENTITY_TO_CLASS(speaker, CSpeaker);

#ifdef HALFLIFE_SAVERESTORE
IMPLEMENT_SAVERESTORE(CSpeaker)
	DEFINE_FIELD(CSpeaker, m_preset, FIELD_INTEGER),
END_SAVERESTORE(CSpeaker, CBaseEntity)
#endif

//
// ambient_generic - general-purpose user-defined static sound
//
bool CSpeaker::Spawn()
{
	char* szSoundFile = (char*)STRING(v.message);

	if (0 == m_preset && (FStringNull(v.message) || strlen(szSoundFile) < 1))
	{
		engine::AlertMessage(at_error, "SPEAKER with no Level/Sentence! at: %f, %f, %f\n", v.origin.x, v.origin.y, v.origin.z);
		return false;
	}
	v.solid = SOLID_NOT;
	v.movetype = MOVETYPE_NONE;


	SetThink(&CSpeaker::SpeakerThink);
	v.nextthink = 0.0;

	// allow on/off switching via 'use' function.

	SetUse(&CSpeaker::ToggleUse);

	Precache();

	return true;
}

#define ANNOUNCE_MINUTES_MIN 0.25
#define ANNOUNCE_MINUTES_MAX 2.25

void CSpeaker::Precache()
{
	if (!FBitSet(v.spawnflags, SPEAKER_START_SILENT))
		// set first announcement time for random n second
		v.nextthink = gpGlobals->time + engine::RandomFloat(5.0, 15.0);
}
void CSpeaker::SpeakerThink()
{
	float flvolume = v.health * 0.1;
	float flattenuation = 0.3;
	int flags = 0;
	int pitch = 100;

	const char* const szSoundFile = [this]()
	{
		// go lookup preset text, assign szSoundFile
		switch (m_preset)
		{
		case 0: return STRING(v.message);
		case 1: return "C1A0_";
		case 2: return "C1A1_";
		case 3: return "C1A2_";
		case 4: return "C1A3_";
		case 5: return "C1A4_";
		case 6: return "C2A1_";
		case 7: return "C2A2_";
		case 8: return "C2A3_";
		case 9: return "C2A4_";
		case 10: return "C2A5_";
		case 11: return "C3A1_";
		case 12: return "C3A2_";
		default: return "";
		}
	}();

	if (szSoundFile[0] == '!')
	{
		// play single sentence, one shot
		EmitAmbientSound(this, v.origin, szSoundFile,
			flvolume, flattenuation, flags, pitch);

		// shut off and reset
		v.nextthink = 0.0;
	}
	else
	{
		// make random announcement from sentence group

		if (SENTENCEG_PlayRndSz(this, szSoundFile, flvolume, flattenuation, flags, pitch) < 0)
			engine::AlertMessage(at_console, "Level Design Error!\nSPEAKER has bad sentence group name: %s\n", szSoundFile);

		// set next announcement time for random 5 to 10 minute delay
		v.nextthink = gpGlobals->time +
						 engine::RandomFloat(ANNOUNCE_MINUTES_MIN * 60.0, ANNOUNCE_MINUTES_MAX * 60.0);
	}

	return;
}


//
// ToggleUse - if an announcement is pending, cancel it.  If no announcement is pending, start one.
//
void CSpeaker::ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	bool fActive = (v.nextthink > 0.0);

	// fActive is true only if an announcement is pending

	if (useType != USE_TOGGLE)
	{
		// ignore if we're just turning something on that's already on, or
		// turning something off that's already off.
		if ((fActive && useType == USE_ON) || (!fActive && useType == USE_OFF))
			return;
	}

	if (useType == USE_ON)
	{
		// turn on announcements
		v.nextthink = gpGlobals->time + 0.1;
		return;
	}

	if (useType == USE_OFF)
	{
		// turn off announcements
		v.nextthink = 0.0;
		return;
	}

	// Toggle announcements


	if (fActive)
	{
		// turn off announcements
		v.nextthink = 0.0;
	}
	else
	{
		// turn on announcements
		v.nextthink = gpGlobals->time + 0.1;
	}
}

// KeyValue - load keyvalue pairs into member data
// NOTE: called BEFORE spawn!

bool CSpeaker::KeyValue(KeyValueData* pkvd)
{

	// preset
	if (streq(pkvd->szKeyName, "preset"))
	{
		m_preset = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}
