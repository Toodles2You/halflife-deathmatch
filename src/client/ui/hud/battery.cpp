/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Battery, Battery)

bool CHudBattery::Init()
{
	m_iBat = 0;

	HOOK_MESSAGE(Battery);

	return CHudBase::Init();
}


void CHudBattery::VidInit()
{
	int HUD_suit_empty = gHUD.GetSpriteIndex("suit_empty");
	int HUD_suit_full = gHUD.GetSpriteIndex("suit_full");

	m_hSprite1 = m_hSprite2 = 0; // delaying get sprite handles until we know the sprites are loaded
	m_prc1 = &gHUD.GetSpriteRect(HUD_suit_empty);
	m_prc2 = &gHUD.GetSpriteRect(HUD_suit_full);
	m_iHeight = m_prc2->bottom - m_prc1->top;
}

void CHudBattery::Update_Battery(int iBat)
{
	SetActive(true);

	if (iBat != m_iBat)
	{
		m_iBat = iBat;
		Flash();
	}
}

bool CHudBattery::MsgFunc_Battery(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int x = READ_SHORT();

	Update_Battery(x);

	return true;
}


void CHudBattery::Draw(const float time)
{
	int x, y, a;
	Rect rc;

	rc = *m_prc2;

	rc.top += m_iHeight * ((float)(100 - (std::min(100, m_iBat))) * 0.01); // battery can go from 0 to 100 so * 0.01 goes from 0 to 1

	a = GetAlpha();

	int iOffset = (m_prc1->bottom - m_prc1->top) / 6;

	y = m_iAnchorY - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	x = m_iAnchorX;

	// make sure we have the right sprite handles
	if (0 == m_hSprite1)
		m_hSprite1 = gHUD.GetSprite(gHUD.GetSpriteIndex("suit_empty"));
	if (0 == m_hSprite2)
		m_hSprite2 = gHUD.GetSprite(gHUD.GetSpriteIndex("suit_full"));

	gHUD.DrawHudSprite(m_hSprite1, 0, m_prc1, x, y - iOffset, CHud::COLOR_PRIMARY, a);

	if (rc.bottom > rc.top)
	{
		gHUD.DrawHudSprite(m_hSprite2, 0, &rc, x, y - iOffset + (rc.top - m_prc2->top), CHud::COLOR_PRIMARY, a);
	}

	x += (m_prc1->right - m_prc1->left);
	x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, CHud::COLOR_PRIMARY, a);
}
