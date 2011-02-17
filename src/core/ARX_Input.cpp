/*
===========================================================================
ARX FATALIS GPL Source Code
Copyright (C) 1999-2010 Arkane Studios SA, a ZeniMax Media company.

This file is part of the Arx Fatalis GPL Source Code ('Arx Fatalis Source Code'). 

Arx Fatalis Source Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public 
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Arx Fatalis Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Arx Fatalis Source Code.  If not, see 
<http://www.gnu.org/licenses/>.

In addition, the Arx Fatalis Source Code is also subject to certain additional terms. You should have received a copy of these 
additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Arx 
Fatalis Source Code. If not, please request a copy in writing from Arkane Studios at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing Arkane Studios, c/o 
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/
//////////////////////////////////////////////////////////////////////////////////////
//   @@        @@@        @@@                @@                           @@@@@     //
//   @@@       @@@@@@     @@@     @@        @@@@                         @@@  @@@   //
//   @@@       @@@@@@@    @@@    @@@@       @@@@      @@                @@@@        //
//   @@@       @@  @@@@   @@@  @@@@@       @@@@@@     @@@               @@@         //
//  @@@@@      @@  @@@@   @@@ @@@@@        @@@@@@@    @@@            @  @@@         //
//  @@@@@      @@  @@@@  @@@@@@@@         @@@@ @@@    @@@@@         @@ @@@@@@@      //
//  @@ @@@     @@  @@@@  @@@@@@@          @@@  @@@    @@@@@@        @@ @@@@         //
// @@@ @@@    @@@ @@@@   @@@@@            @@@@@@@@@   @@@@@@@      @@@ @@@@         //
// @@@ @@@@   @@@@@@@    @@@@@@           @@@  @@@@   @@@ @@@      @@@ @@@@         //
// @@@@@@@@   @@@@@      @@@@@@@@@@      @@@    @@@   @@@  @@@    @@@  @@@@@        //
// @@@  @@@@  @@@@       @@@  @@@@@@@    @@@    @@@   @@@@  @@@  @@@@  @@@@@        //
//@@@   @@@@  @@@@@      @@@      @@@@@@ @@     @@@   @@@@   @@@@@@@    @@@@@ @@@@@ //
//@@@   @@@@@ @@@@@     @@@@        @@@  @@      @@   @@@@   @@@@@@@    @@@@@@@@@   //
//@@@    @@@@ @@@@@@@   @@@@             @@      @@   @@@@    @@@@@      @@@@@      //
//@@@    @@@@ @@@@@@@   @@@@             @@      @@   @@@@    @@@@@       @@        //
//@@@    @@@  @@@ @@@@@                          @@            @@@                  //
//            @@@ @@@                           @@             @@        STUDIOS    //
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// ARX_Input
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		ARX Input interface (uses MERCURY input system)
//
// Updates: (date) (person) (update)
//
// Code: Cyril Meynier
//
// Copyright (c) 1999-2001 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "core/ARX_Input.h"
#include "core/ARX_Menu2.h" //controls

DXI_INIT	InputInit;
long ARX_SCID = 0;
long ARX_XBOXPAD = 0;

//-----------------------------------------------------------------------------
extern CDirectInput * pGetInfoDirectInput;
extern CMenuConfig * pMenuConfig;
extern long STOP_KEYBOARD_INPUT;

//-----------------------------------------------------------------------------
bool ARX_INPUT_Init(HINSTANCE hInst, HWND hWnd)
{
#ifdef NO_DIRECT_INPUT
	return true;
#endif
	memset((void *)&InputInit, 0, sizeof(DXI_INIT));
	DXI_Init(hInst, &InputInit);

	if (DXI_FAIL == DXI_GetKeyboardInputDevice(hWnd, DXI_KEYBOARD1, DXI_MODE_NONEXCLUSIF_OURMSG)) 
		return false;

	if (DXI_FAIL == DXI_GetMouseInputDevice(hWnd, DXI_MOUSE1, DXI_MODE_NONEXCLUSIF_ALLMSG, 2, 2))
		return false;

	if (DXI_FAIL == DXI_GetSCIDInputDevice(hWnd, DXI_SCID, DXI_MODE_EXCLUSIF_OURMSG, 2, 2))
		ARX_SCID = 0;
	else ARX_SCID = 1;

	if (DXI_FAIL == DXI_SetMouseRelative(DXI_MOUSE1))
		return false;

	//"ThrustMaster FireStorm(TM) Dual Power Gamepad"
	if (DXI_FAIL != DXI_GetJoyInputDevice(hWnd, DXI_JOY1, DXI_MODE_EXCLUSIF_ALLMSG, 13, 4))
	{
		ARX_XBOXPAD = 1;
		DXI_SetRangeJoy(DXI_JOY1, DXI_XAxis, 100);
		DXI_SetRangeJoy(DXI_JOY1, DXI_YAxis, 100);
		DXI_SetRangeJoy(DXI_JOY1, DXI_RzAxis, 100);
		DXI_SetRangeJoy(DXI_JOY1, DXI_Slider, 100);
	}
	else ARX_XBOXPAD = 0;

	return true;
}

void ARX_INPUT_Release()
{
#ifdef NO_DIRECT_INPUT
	return;
#endif
	DXI_Release();
}

//ARX_GAME_IMPULSES GameImpulses;
long GameImpulses[MAX_IMPULSES][MAX_IMPULSES_NB];


void ARX_INPUT_Init_Game_Impulses()
{
	for (long i = 0; i < MAX_IMPULSES; i++)
		for (long j = 0; j < MAX_IMPULSES_NB; j++)
			GameImpulses[i][j] = 0;

	GameImpulses[ARX_INPUT_IMPULSE_MAGIC_MODE][0] = 29;
	GameImpulses[ARX_INPUT_IMPULSE_MAGIC_MODE][1] = 157;
	GameImpulses[ARX_INPUT_IMPULSE_MAGIC_MODE][2] = 0; 
	GameImpulses[ARX_INPUT_IMPULSE_COMBAT_MODE][0] = 28;
	GameImpulses[ARX_INPUT_IMPULSE_COMBAT_MODE][1] = INTERNAL_JOYSTICK_7;
	GameImpulses[ARX_INPUT_IMPULSE_JUMP][0] = 82;
	GameImpulses[ARX_INPUT_IMPULSE_JUMP][1] = INTERNAL_MOUSE_3;
	GameImpulses[ARX_INPUT_IMPULSE_JUMP][2] = INTERNAL_JOYSTICK_1;
	GameImpulses[ARX_INPUT_IMPULSE_STEALTH][0] = 54;
	GameImpulses[ARX_INPUT_IMPULSE_STEALTH][1] = 42;

	GameImpulses[ARX_INPUT_IMPULSE_WALK_FORWARD][0] = 200;
	GameImpulses[ARX_INPUT_IMPULSE_WALK_FORWARD][1] = 0;
	GameImpulses[ARX_INPUT_IMPULSE_WALK_BACKWARD][0] = 208;
	GameImpulses[ARX_INPUT_IMPULSE_WALK_BACKWARD][1] = 0;
	GameImpulses[ARX_INPUT_IMPULSE_STRAFE_LEFT][0] = 203;
	GameImpulses[ARX_INPUT_IMPULSE_STRAFE_LEFT][1] = 0;
	GameImpulses[ARX_INPUT_IMPULSE_STRAFE_RIGHT][0] = 205;
	GameImpulses[ARX_INPUT_IMPULSE_STRAFE_RIGHT][1] = 0;

	GameImpulses[ARX_INPUT_IMPULSE_MOUSE_LOOK][0] = INTERNAL_MOUSE_2;
	GameImpulses[ARX_INPUT_IMPULSE_MOUSE_LOOK][1] = 0;

	GameImpulses[ARX_INPUT_IMPULSE_ACTION][0] = INTERNAL_MOUSE_1;
	GameImpulses[ARX_INPUT_IMPULSE_ACTION][1] = INTERNAL_JOYSTICK_13;

	GameImpulses[ARX_INPUT_IMPULSE_INVENTORY][0] = 23;
	GameImpulses[ARX_INPUT_IMPULSE_INVENTORY][1] = INTERNAL_JOYSTICK_8;

	GameImpulses[ARX_INPUT_IMPULSE_BOOK][0] = 15;
	GameImpulses[ARX_INPUT_IMPULSE_BOOK][1] = INTERNAL_JOYSTICK_6;

	GameImpulses[ARX_INPUT_IMPULSE_LEAN_RIGHT][0] = 49;
	GameImpulses[ARX_INPUT_IMPULSE_LEAN_RIGHT][1] = 0;

	GameImpulses[ARX_INPUT_IMPULSE_LEAN_LEFT][0] = 51;
	GameImpulses[ARX_INPUT_IMPULSE_LEAN_LEFT][1] = 0;

	GameImpulses[ARX_INPUT_IMPULSE_CROUCH][0] = 83; //50;
	GameImpulses[ARX_INPUT_IMPULSE_CROUCH][1] = INTERNAL_JOYSTICK_2;
}
 
bool ARX_INPUT_GetSCIDAxis(int * jx, int * jy, int * jz)
{
	if (ARX_SCID)
	{
		DXI_GetSCIDAxis(DXI_SCID, jx, jy, jz);
		return true;
	}
	else
	{
		*jx = 0;
		*jy = 0;
		*jz = 0;
		return false;
	}
}
 
//-----------------------------------------------------------------------------
bool ARX_IMPULSE_NowPressed(long ident)
{
	switch (ident)
	{
		case CONTROLS_CUST_MOUSELOOK:
		case CONTROLS_CUST_ACTION:
			break;
		default:
		{
			for (long j = 0; j < 2; j++)
			{
				if (pMenuConfig->sakActionKey[ident].iKey[j] != -1)
				{
					if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x80000000)
					{
						if (pGetInfoDirectInput->GetMouseButtonNowPressed(pMenuConfig->sakActionKey[ident].iKey[j]&~0x80000000))
							return true;
					}
					else
					{
						if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x40000000)
						{
							if (pMenuConfig->sakActionKey[ident].iKey[j] == 0x40000001)
							{
								if (pGetInfoDirectInput->iWheelSens < 0) return true;
							}
							else
							{
								if (pGetInfoDirectInput->iWheelSens > 0) return true;
							}
						}
						else
						{
							bool bCombine = true;

							if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x7FFF0000)
							{
								if (!pGetInfoDirectInput->IsVirtualKeyPressed((pMenuConfig->sakActionKey[ident].iKey[j] >> 16) & 0xFFFF))
									bCombine = false;
							}

							if (pGetInfoDirectInput->IsVirtualKeyPressedNowPressed(pMenuConfig->sakActionKey[ident].iKey[j] & 0xFFFF))
								return true & bCombine;
						}
					}
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
static unsigned int uiOneHandedMagicMode = 0;
static unsigned int uiOneHandedStealth = 0;

bool ARX_IMPULSE_Pressed(long ident)
{
	switch (ident)
	{
		case CONTROLS_CUST_MOUSELOOK:
		case CONTROLS_CUST_ACTION:
			break;
		default:
		{
			if (pMenuConfig->bOneHanded)
			{
				for (long j = 0; j < 2; j++)
				{
					if (pMenuConfig->sakActionKey[ident].iKey[j] != -1)
					{
						if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x80000000)
						{
							if (pGetInfoDirectInput->GetMouseButtonRepeat(pMenuConfig->sakActionKey[ident].iKey[j]&~0x80000000))
								return true;
						}
						else
						{
							if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x40000000)
							{
								if (pMenuConfig->sakActionKey[ident].iKey[j] == 0x40000001)
								{
									if (pGetInfoDirectInput->iWheelSens < 0) return true;
								}
								else
								{
									if (pGetInfoDirectInput->iWheelSens > 0) return true;
								}
							}
							else
							{
								bool bCombine = true;

								if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x7FFF0000)
								{
									if (!pGetInfoDirectInput->IsVirtualKeyPressed((pMenuConfig->sakActionKey[ident].iKey[j] >> 16) & 0xFFFF))
										bCombine = false;
								}

								if (pGetInfoDirectInput->IsVirtualKeyPressed(pMenuConfig->sakActionKey[ident].iKey[j] & 0xFFFF))
								{
									bool bQuit = false;

									switch (ident)
									{
										case CONTROLS_CUST_MAGICMODE:
										{
											if (bCombine)
											{
												if (!uiOneHandedMagicMode)
												{
													uiOneHandedMagicMode = 1;
												}
												else
												{
													if (uiOneHandedMagicMode == 2)
													{
														uiOneHandedMagicMode = 3;
													}
												}

												bQuit = true;
											}
										}
										break;
										case CONTROLS_CUST_STEALTHMODE:
										{
											if (bCombine)
											{
												if (!uiOneHandedStealth)
												{
													uiOneHandedStealth = 1;
												}
												else
												{
													if (uiOneHandedStealth == 2)
													{
														uiOneHandedStealth = 3;
													}
												}

												bQuit = true;
											}
										}
										break;
										default:
										{
											return true & bCombine;
										}
										break;
									}

									if (bQuit)
									{
										break;
									}
								}
								else
								{
									switch (ident)
									{
										case CONTROLS_CUST_MAGICMODE:
										{
											if ((!j) &&
											        (pGetInfoDirectInput->IsVirtualKeyPressed(pMenuConfig->sakActionKey[ident].iKey[j+1] & 0xFFFF)))
											{
												continue;
											}

											if (uiOneHandedMagicMode == 1)
											{
												uiOneHandedMagicMode = 2;
											}
											else
											{
												if (uiOneHandedMagicMode == 3)
												{
													uiOneHandedMagicMode = 0;
												}
											}
										}
										break;
										case CONTROLS_CUST_STEALTHMODE:
										{
											if ((!j) &&
											        (pGetInfoDirectInput->IsVirtualKeyPressed(pMenuConfig->sakActionKey[ident].iKey[j+1] & 0xFFFF)))
											{
												continue;
											}

											if (uiOneHandedStealth == 1)
											{
												uiOneHandedStealth = 2;
											}
											else
											{
												if (uiOneHandedStealth == 3)
												{
													uiOneHandedStealth = 0;
												}
											}
										}
										break;
									}
								}
							}
						}
					}
				}

				switch (ident)
				{
					case CONTROLS_CUST_MAGICMODE:

						if ((uiOneHandedMagicMode == 1) || (uiOneHandedMagicMode == 2))
						{
							return true;
						}

						break;
					case CONTROLS_CUST_STEALTHMODE:

						if ((uiOneHandedStealth == 1) || (uiOneHandedStealth == 2))
						{
							return true;
						}

						break;
				}
			}
			else
			{
				for (long j = 0; j < 2; j++)
				{
					if (pMenuConfig->sakActionKey[ident].iKey[j] != -1)
					{
						if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x80000000)
						{
							if (pGetInfoDirectInput->GetMouseButtonRepeat(pMenuConfig->sakActionKey[ident].iKey[j]&~0x80000000))
								return true;
						}
						else
						{
							if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x40000000)
							{
								if (pMenuConfig->sakActionKey[ident].iKey[j] == 0x40000001)
								{
									if (pGetInfoDirectInput->iWheelSens < 0) return true;
								}
								else
								{
									if (pGetInfoDirectInput->iWheelSens > 0) return true;
								}
							}
							else
							{
								bool bCombine = true;

								if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x7FFF0000)
								{
									if (!pGetInfoDirectInput->IsVirtualKeyPressed((pMenuConfig->sakActionKey[ident].iKey[j] >> 16) & 0xFFFF))
										bCombine = false;
								}

								if (pGetInfoDirectInput->IsVirtualKeyPressed(pMenuConfig->sakActionKey[ident].iKey[j] & 0xFFFF))
									return true & bCombine;
							}
						}
					}
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
bool ARX_IMPULSE_NowUnPressed(long ident)
{
	switch (ident)
	{
		case CONTROLS_CUST_MOUSELOOK:
		case CONTROLS_CUST_ACTION:
			break;
		default:
		{
			for (long j = 0; j < 2; j++)
			{
				if (pMenuConfig->sakActionKey[ident].iKey[j] != -1)
				{
					if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x80000000)
					{
						if (pGetInfoDirectInput->GetMouseButtonNowUnPressed(pMenuConfig->sakActionKey[ident].iKey[j]&~0x80000000))
							return true;
					}
					else
					{
						bool bCombine = true;

						if (pMenuConfig->sakActionKey[ident].iKey[j] & 0x7FFF0000)
						{
							if (!pGetInfoDirectInput->IsVirtualKeyPressed((pMenuConfig->sakActionKey[ident].iKey[j] >> 16) & 0xFFFF))
								bCombine = false;
						}

						if (pGetInfoDirectInput->IsVirtualKeyPressedNowUnPressed(pMenuConfig->sakActionKey[ident].iKey[j] & 0xFFFF))
							return true & bCombine;
					}
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
