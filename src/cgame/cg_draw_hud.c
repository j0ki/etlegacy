/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file cg_draw_hud.c
 * @brief Draws the player's hud
 *
 */

#include "cg_local.h"

typedef enum
{
	STYLE_NORMAL,
	STYLE_SIMPLE
} componentStyle;

#define SKILL_ICON_SIZE     14

#define SKILLS_X 112
#define SKILLS_Y 20

#define SKILL_BAR_OFFSET    (2 * SKILL_BAR_X_INDENT)
#define SKILL_BAR_X_INDENT  0
#define SKILL_BAR_Y_INDENT  6

#define SKILL_BAR_WIDTH     (SKILL_ICON_SIZE - SKILL_BAR_OFFSET)
#define SKILL_BAR_X         (SKILL_BAR_OFFSET + SKILL_BAR_X_INDENT + SKILLS_X)
#define SKILL_BAR_X_SCALE   (SKILL_ICON_SIZE + 2)
#define SKILL_ICON_X        (SKILL_BAR_OFFSET + SKILLS_X)
#define SKILL_ICON_X_SCALE  (SKILL_ICON_SIZE + 2)
#define SKILL_BAR_Y         (SKILL_BAR_Y_INDENT - SKILL_BAR_OFFSET - SKILLS_Y)
#define SKILL_BAR_Y_SCALE   (SKILL_ICON_SIZE + 2)
#define SKILL_ICON_Y        (-(SKILL_ICON_SIZE + 2) - SKILL_BAR_OFFSET - SKILLS_Y)

#define MAXHUDS 32

int           hudCount = 0;
hudStucture_t hudlist[MAXHUDS];

hudStucture_t *activehud;

lagometer_t lagometer;

static void CG_DrawPlayerStatusHead(hudComponent_t *comp);
static void CG_DrawGunIcon(hudComponent_t *comp);
static void CG_DrawAmmoCount(hudComponent_t *comp);
static void CG_DrawPowerUps(hudComponent_t *comp);
static void CG_DrawObjectiveStatus(hudComponent_t *comp);
static void CG_DrawPlayerHealthBar(hudComponent_t *comp);
static void CG_DrawStaminaBar(hudComponent_t *comp);
static void CG_DrawBreathBar(hudComponent_t *comp);
static void CG_DrawWeapRecharge(hudComponent_t *comp);
static void CG_DrawPlayerHealth(hudComponent_t *comp);
static void CG_DrawPlayerSprint(hudComponent_t *comp);
static void CG_DrawPlayerBreath(hudComponent_t *comp);
static void CG_DrawWeaponCharge(hudComponent_t *comp);
static void CG_DrawSkills(hudComponent_t *comp);
static void CG_DrawXP(hudComponent_t *comp);
static void CG_DrawRank(hudComponent_t *comp);
static void CG_DrawLivesLeft(hudComponent_t *comp);
static void CG_DrawCursorhint_f(hudComponent_t *comp);
static void CG_DrawWeapStability_f(hudComponent_t *comp);
static void CG_DrawRespawnTimer(hudComponent_t *comp);
static void CG_DrawSpawnTimer(hudComponent_t *comp);
static void CG_DrawLocalTime(hudComponent_t *comp);
static void CG_DrawRoundTimer(hudComponent_t *comp);
static void CG_DrawDemoMessage(hudComponent_t *comp);
static void CG_DrawFPS(hudComponent_t *comp);
static void CG_DrawSnapshot(hudComponent_t *comp);
static void CG_DrawPing(hudComponent_t *comp);
static void CG_DrawSpeed(hudComponent_t *comp);
static void CG_DrawLagometer(hudComponent_t *comp);
static void CG_DrawDisconnect(hudComponent_t *comp);

/**
 * @brief Using the stringizing operator to save typing...
 */
#define HUDF(x) # x, offsetof(hudStucture_t, x), qfalse

typedef struct
{
	const char *name;
	size_t offset;
	qboolean isAlias;
	void (*draw)(hudComponent_t *comp);

} hudComponentFields_t;

/**
* @var hudComponentFields
* @brief for accessing hudStucture_t's fields in a loop
*/
static const hudComponentFields_t hudComponentFields[] =
{
	{ HUDF(compass),          CG_DrawNewCompass       },
	{ "compas",               offsetof(hudStucture_t, compass), qtrue, CG_DrawNewCompass}, // v2.78 backward compatibility
	{ HUDF(staminabar),       CG_DrawStaminaBar       },
	{ HUDF(breathbar),        CG_DrawBreathBar        },
	{ HUDF(healthbar),        CG_DrawPlayerHealthBar  },
	{ HUDF(weaponchargebar),  CG_DrawWeapRecharge     },
	{ "weaponchangebar",      offsetof(hudStucture_t, weaponchargebar), qtrue, CG_DrawWeapRecharge}, // v2.78 backward compatibility
	{ HUDF(healthtext),       CG_DrawPlayerHealth     },
	{ HUDF(xptext),           CG_DrawXP               },
	{ HUDF(ranktext),         CG_DrawRank             },
	{ HUDF(statsdisplay),     CG_DrawSkills           },
	{ HUDF(weaponicon),       CG_DrawGunIcon          },
	{ HUDF(weaponammo),       CG_DrawAmmoCount        },
	{ HUDF(fireteam),         CG_DrawFireTeamOverlay  },    // FIXME: outside cg_draw_hud
	{ HUDF(popupmessages),    CG_DrawPMItems          },    // FIXME: outside cg_draw_hud
	{ HUDF(powerups),         CG_DrawPowerUps         },
	{ HUDF(objectives),       CG_DrawObjectiveStatus  },
	{ HUDF(hudhead),          CG_DrawPlayerStatusHead },
	{ HUDF(cursorhints),      CG_DrawCursorhint_f     },
	{ HUDF(weaponstability),  CG_DrawWeapStability_f  },
	{ HUDF(livesleft),        CG_DrawLivesLeft        },
	{ HUDF(roundtimer),       CG_DrawRoundTimer       },
	{ HUDF(reinforcement),    CG_DrawRespawnTimer     },
	{ HUDF(spawntimer),       CG_DrawSpawnTimer       },
	{ HUDF(localtime),        CG_DrawLocalTime        },
	{ HUDF(votetext),         CG_DrawVote             },    // FIXME: outside cg_draw_hud
	{ HUDF(spectatortext),    CG_DrawSpectatorMessage },    // FIXME: outside cg_draw_hud
	{ HUDF(limbotext),        CG_DrawLimboMessage     },    // FIXME: outside cg_draw_hud
	{ HUDF(followtext),       CG_DrawFollow           },    // FIXME: outside cg_draw_hud
	{ HUDF(demotext),         CG_DrawDemoMessage      },
	{ HUDF(missilecamera),    CG_DrawMissileCamera    },    // FIXME: outside cg_draw_hud
	{ HUDF(sprinttext),       CG_DrawPlayerSprint     },
	{ HUDF(breathtext),       CG_DrawPlayerBreath     },
	{ HUDF(weaponchargetext), CG_DrawWeaponCharge     },
	{ HUDF(fps),              CG_DrawFPS              },
	{ HUDF(snapshot),         CG_DrawSnapshot         },
	{ HUDF(ping),             CG_DrawPing             },
	{ HUDF(speed),            CG_DrawSpeed            },
	{ HUDF(lagometer),        CG_DrawLagometer        },
	{ HUDF(disconnect),       CG_DrawDisconnect       },
	{ HUDF(chat),             CG_DrawTeamInfo         },    // FIXME: outside cg_draw_hud
	{ NULL,                   0, qfalse, NULL         },
};

/**
 * @brief CG_getActiveHUD Returns reference to an active hud structure.
 * @return
 */
hudStucture_t *CG_GetActiveHUD()
{
	return activehud;
}

/**
 * @brief CG_getComponent
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] visible
 * @param[in] style
 * @return
 */
static ID_INLINE hudComponent_t CG_getComponent(float x, float y, float w, float h, qboolean visible, componentStyle style, float scale, const vec4_t color, int offset, void (*draw)(hudComponent_t *comp))
{
	return (hudComponent_t) { { x, y, w, h }, visible, style, scale, { color[0], color[1], color[2], color[3] }, offset, draw };
}

vec4_t HUD_Background = { 0.16f, 0.2f, 0.17f, 0.5f };
vec4_t HUD_Border     = { 0.5f, 0.5f, 0.5f, 0.5f };
vec4_t HUD_Text       = { 0.6f, 0.6f, 0.6f, 1.0f };

/**
 * @brief CG_setDefaultHudValues
 * @param[out] hud
 */
void CG_setDefaultHudValues(hudStucture_t *hud)
{
	// the Default hud
	hud->hudnumber        = 0;
	hud->compass          = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 100 - 20 - 16, 0, 100 + 32, 100 + 32, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 0, CG_DrawNewCompass);
	hud->staminabar       = CG_getComponent(4, SCREEN_HEIGHT - 92, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 1, CG_DrawStaminaBar);
	hud->breathbar        = CG_getComponent(4, SCREEN_HEIGHT - 92, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 2, CG_DrawBreathBar);
	hud->healthbar        = CG_getComponent(24, SCREEN_HEIGHT - 92, 12, 72, qtrue, STYLE_SIMPLE, 0.19f, colorWhite, 3, CG_DrawPlayerHealthBar);
	hud->weaponchargebar  = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 16, SCREEN_HEIGHT - 92, 12, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 4, CG_DrawWeapRecharge);
	hud->healthtext       = CG_getComponent(47, 465, 57, 14, qtrue, STYLE_SIMPLE, 0.25f, colorWhite, 5, CG_DrawPlayerHealth);
	hud->xptext           = CG_getComponent(108, 465, 57, 14, qtrue, STYLE_SIMPLE, 0.25f, colorWhite, 6, CG_DrawXP);
	hud->ranktext         = CG_getComponent(0, SCREEN_HEIGHT, 57, 14, qfalse, STYLE_SIMPLE, 0.2f, colorWhite, 7, CG_DrawRank);   // disable
	hud->statsdisplay     = CG_getComponent(116, 394, 14, 70, qtrue, STYLE_NORMAL, 0.25f, colorWhite, 8, CG_DrawSkills);
	hud->weaponicon       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 82, SCREEN_HEIGHT - 56, 60, 32, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 9, CG_DrawGunIcon);
	hud->weaponammo       = CG_getComponent(694, 458, 57, 14, qtrue, STYLE_SIMPLE, 0.25f, colorWhite, 10, CG_DrawAmmoCount);
	hud->fireteam         = CG_getComponent(10, 10, 260, 14, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 11, CG_DrawFireTeamOverlay);
	hud->popupmessages    = CG_getComponent(4, 320, 72, 72, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 12, CG_DrawPMItems);
	hud->powerups         = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 40, SCREEN_HEIGHT - 136, 36, 36, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 13, CG_DrawPowerUps);
	hud->objectives       = CG_getComponent(8, SCREEN_HEIGHT - 136, 36, 36, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 14, CG_DrawObjectiveStatus);
	hud->hudhead          = CG_getComponent(44, SCREEN_HEIGHT - 92, 62, 80, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 15, CG_DrawPlayerStatusHead);
	hud->cursorhints      = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) * .5f - 24, 260, 48, 48, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 16, CG_DrawCursorhint_f);
	hud->weaponstability  = CG_getComponent(50, 208, 10, 64, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 17, CG_DrawWeapStability_f);
	hud->livesleft        = CG_getComponent(4, 360, 48, 24, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 18, CG_DrawLivesLeft);
	hud->roundtimer       = CG_getComponent(706, 152, 57, 14, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 19, CG_DrawRoundTimer);
	hud->reinforcement    = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 60, SCREEN_HEIGHT - 70, 57, 14, qfalse, STYLE_SIMPLE, 0.19f, colorLtBlue, 20, CG_DrawRespawnTimer);
	hud->spawntimer       = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 60, SCREEN_HEIGHT - 60, 57, 14, qfalse, STYLE_SIMPLE, 0.19f, colorRed, 21, CG_DrawSpawnTimer);
	hud->localtime        = CG_getComponent(706, 168, 57, 14, qtrue, STYLE_NORMAL, 0.19f, HUD_Text, 22, CG_DrawLocalTime);
	hud->votetext         = CG_getComponent(8, 224, 57, 14, qtrue, STYLE_NORMAL, 0.22f, colorWhite, 23, CG_DrawVote);
	hud->spectatortext    = CG_getComponent(8, 188, 57, 14, qtrue, STYLE_NORMAL, 0.22f, colorWhite, 24, CG_DrawSpectatorMessage);
	hud->limbotext        = CG_getComponent(8, 164, 57, 14, qtrue, STYLE_NORMAL, 0.22f, colorWhite, 25, CG_DrawLimboMessage);
	hud->followtext       = CG_getComponent(8, 164, 57, 14, qtrue, STYLE_NORMAL, 0.22f, colorWhite, 26, CG_DrawFollow);
	hud->demotext         = CG_getComponent(10, 9, 57, 14, qtrue, STYLE_SIMPLE, 0.22f, colorRed, 27, CG_DrawDemoMessage);
	hud->missilecamera    = CG_getComponent(4, 120, 160, 120, qtrue, STYLE_NORMAL, 1, colorWhite, 28, CG_DrawMissileCamera);
	hud->sprinttext       = CG_getComponent(20, SCREEN_HEIGHT - 96, 57, 14, qfalse, STYLE_SIMPLE, 0.25f, colorWhite, 29, CG_DrawPlayerSprint);
	hud->breathtext       = CG_getComponent(20, SCREEN_HEIGHT - 96, 57, 14, qfalse, STYLE_SIMPLE, 0.25, colorWhite, 30, CG_DrawPlayerBreath);
	hud->weaponchargetext = CG_getComponent(Ccg_WideX(SCREEN_WIDTH) - 16, SCREEN_HEIGHT - 96, 57, 14, qfalse, STYLE_NORMAL, 0.25f, colorWhite, 31, CG_DrawWeaponCharge);
	hud->fps              = CG_getComponent(706, 184, 57, 14, qtrue, STYLE_NORMAL, 0.19f, HUD_Text, 32, CG_DrawFPS);
	hud->snapshot         = CG_getComponent(706, 305, 57, 38, qfalse, STYLE_NORMAL, 0.19f, HUD_Text, 33, CG_DrawSnapshot);
	hud->ping             = CG_getComponent(706, 200, 57, 14, qtrue, STYLE_NORMAL, 0.19f, HUD_Text, 34, CG_DrawPing);
	hud->speed            = CG_getComponent(706, 275, 57, 14, qtrue, STYLE_NORMAL, 0.19f, HUD_Text, 35, CG_DrawSpeed);
	hud->lagometer        = CG_getComponent(706, 216, 57, 57, qtrue, STYLE_NORMAL, 0.19f, HUD_Text, 36, CG_DrawLagometer);
	hud->disconnect       = CG_getComponent(706, 216, 57, 57, qtrue, STYLE_NORMAL, 0.19f, colorWhite, 37, CG_DrawDisconnect);
	hud->chat             = CG_getComponent(Ccg_WideX(160), 469, 434, 9, qtrue, STYLE_NORMAL, 1.0f, colorWhite, 38, CG_DrawTeamInfo);
}

/**
 * @brief CG_getHudByNumber
 * @param[in] number
 * @return
 */
static hudStucture_t *CG_getHudByNumber(int number)
{
	int           i;
	hudStucture_t *hud;

	for (i = 0; i < hudCount; i++)
	{
		hud = &hudlist[i];

		if (hud->hudnumber == number)
		{
			return hud;
		}
	}

	return NULL;
}

static int QDECL CG_HudComponentSort(const void *a, const void *b)
{
	return ((*(hudComponent_t **) a)->offset - (*(hudComponent_t **) b)->offset);
}

static void CG_HudComponentsFill(hudStucture_t *hud)
{
	int i, componentOffset;

	// setup component pointers to the components list
	for (i = 0, componentOffset = 0; hudComponentFields[i].name; i++)
	{
		if (hudComponentFields[i].isAlias)
		{
			continue;
		}
		hud->components[componentOffset++] = (hudComponent_t *)((char * )hud + hudComponentFields[i].offset);
	}
	// sort the components by their offset
	qsort(hud->components, sizeof(hud->components) / sizeof(hudComponent_t *), sizeof(hudComponent_t *), CG_HudComponentSort);
}

/**
 * @brief CG_isHudNumberAvailable checks if the hud by said number is available for use, 0 to 2 are forbidden.
 * @param[in] number
 * @return
 */
static qboolean CG_isHudNumberAvailable(int number)
{
	// values 0 is used by the default hud's
	if (number <= 0 || number >= MAXHUDS)
	{
		Com_Printf(S_COLOR_RED "CG_isHudNumberAvailable: invalid HUD number %i, allowed values: 1 - %i\n", number, MAXHUDS);
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief CG_addHudToList
 * @param[in] hud
 */
static hudStucture_t *CG_addHudToList(hudStucture_t *hud)
{
	hudStucture_t *out = NULL;

	hudlist[hudCount] = *hud;
	out               = &hudlist[hudCount];
	hudCount++;

	CG_HudComponentsFill(out);

	return out;
}

//  HUD SCRIPT FUNCTIONS BELLOW

/**
 * @brief CG_HUD_ParseError
 * @param[in] handle
 * @param[in] format
 * @return
 */
static qboolean CG_HUD_ParseError(int handle, const char *format, ...)
{
	int         line;
	char        filename[MAX_QPATH];
	va_list     argptr;
	static char string[4096];

	va_start(argptr, format);
	Q_vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	filename[0] = '\0';
	line        = 0;
	trap_PC_SourceFileAndLine(handle, filename, &line);

	Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string);

	trap_PC_FreeSource(handle);

	return qfalse;
}

/**
 * @brief CG_RectParse
 * @param[in] handle
 * @param[in,out] r
 * @return
 */
static qboolean CG_RectParse(int handle, rectDef_t *r)
{
	float      x = 0;
	pc_token_t peakedToken;

	if (!PC_PeakToken(handle, &peakedToken))
	{
		return qfalse;
	}

	if (peakedToken.string[0] == '(')
	{
		if (!trap_PC_ReadToken(handle, &peakedToken))
		{
			return qfalse;
		}
	}

	if (PC_Float_Parse(handle, &x))
	{
		r->x = Ccg_WideX(x);
		if (PC_Float_Parse(handle, &r->y))
		{
			if (PC_Float_Parse(handle, &r->w))
			{
				if (PC_Float_Parse(handle, &r->h))
				{
					return qtrue;
				}
			}
		}
	}

	if (!PC_PeakToken(handle, &peakedToken))
	{
		return qfalse;
	}

	if (peakedToken.string[0] == ')')
	{
		if (!trap_PC_ReadToken(handle, &peakedToken))
		{
			return qfalse;
		}
	}

	return qfalse;
}

static qboolean CG_Vec4Parse(int handle, vec4_t v)
{
	float      r, g, b, a = 0;
	pc_token_t peakedToken;

	if (!PC_PeakToken(handle, &peakedToken))
	{
		return qfalse;
	}

	if (peakedToken.string[0] == '(')
	{
		if (!trap_PC_ReadToken(handle, &peakedToken))
		{
			return qfalse;
		}
	}

	if (PC_Float_Parse(handle, &r))
	{
		if (PC_Float_Parse(handle, &g))
		{
			if (PC_Float_Parse(handle, &b))
			{
				if (PC_Float_Parse(handle, &a))
				{
					v[0] = r;
					v[1] = g;
					v[2] = b;
					v[3] = a;
					return qtrue;
				}
			}
		}
	}

	if (!PC_PeakToken(handle, &peakedToken))
	{
		return qfalse;
	}

	if (peakedToken.string[0] == ')')
	{
		if (!trap_PC_ReadToken(handle, &peakedToken))
		{
			return qfalse;
		}
	}

	return qfalse;
}

/**
 * @brief CG_ParseHudComponent
 * @param[in] handle
 * @param[in] comp
 * @return
 */
static qboolean CG_ParseHudComponent(int handle, hudComponent_t *comp)
{
	//PC_Rect_Parse
	if (!CG_RectParse(handle, &comp->location))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &comp->style))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &comp->visible))
	{
		return qfalse;
	}

	// Optional scale and color
	pc_token_t token;
	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}
	if (token.type == TT_NUMBER)
	{
		trap_PC_UnReadToken(handle);
		if (!PC_Float_Parse(handle, &comp->scale))
		{
			return qfalse;
		}

		if (!CG_Vec4Parse(handle, comp->color))
		{
			return qfalse;
		}
	}
	else
	{
		trap_PC_UnReadToken(handle);
	}

	return qtrue;
}

/**
 * @brief CG_ParseHUD
 * @param[in] handle
 * @return
 */
static qboolean CG_ParseHUD(int handle)
{
	int           i, componentOffset = 0;
	pc_token_t    token;
	hudStucture_t temphud;
	hudStucture_t *hud;
	qboolean      loadDefaults = qtrue;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_HUD_ParseError(handle, "expected '{'");
	}

	if (!trap_PC_ReadToken(handle, &token))
	{
		return CG_HUD_ParseError(handle, "Error while parsing hud");
	}

	// if the first parameter in the hud definition is a "no-defaults" line then no default values are set
	// and the hud is plain (everything is hidden and no positions are set)
	if (!Q_stricmp(token.string, "no-defaults"))
	{
		loadDefaults = qfalse;
	}
	else
	{
		trap_PC_UnReadToken(handle);
	}

	// reset all the components, and set the offset value to 999 for sorting
	Com_Memset(&temphud, 0, sizeof(hudStucture_t));

	if (loadDefaults)
	{
		CG_setDefaultHudValues(&temphud);
	}
	else
	{
		for (i = 0; hudComponentFields[i].name; i++)
		{
			hudComponent_t *component = (hudComponent_t *)((char * )&temphud + hudComponentFields[i].offset);
			component->offset = 999;
		}
	}

	componentOffset = 0;
	while (qtrue)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "hudnumber"))
		{
			if (!PC_Int_Parse(handle, &temphud.hudnumber))
			{
				return CG_HUD_ParseError(handle, "expected integer value for hudnumber");
			}

			continue;
		}

		for (i = 0; hudComponentFields[i].name; i++)
		{
			if (!Q_stricmp(token.string, hudComponentFields[i].name))
			{
				hudComponent_t *component = (hudComponent_t *)((char * )&temphud + hudComponentFields[i].offset);
				component->offset = componentOffset++;
				component->draw   = hudComponentFields[i].draw;
				if (!CG_ParseHudComponent(handle, component))
				{
					return CG_HUD_ParseError(handle, "expected %s", hudComponentFields[i].name);
				}
				break;
			}
		}

		if (!hudComponentFields[i].name)
		{
			return CG_HUD_ParseError(handle, "unexpected token: %s", token.string);
		}
	}

	// check that the hudnumber value was set
	if (!CG_isHudNumberAvailable(temphud.hudnumber))
	{
		return CG_HUD_ParseError(handle, "Invalid hudnumber value: %i", temphud.hudnumber);
	}

	hud = CG_getHudByNumber(temphud.hudnumber);

	if (!hud)
	{
		CG_addHudToList(&temphud);
		Com_Printf("...properties for hud %i have been read.\n", temphud.hudnumber);
	}
	else
	{
		Com_Memcpy(hud, &temphud, sizeof(temphud));
		CG_HudComponentsFill(hud);
		Com_Printf("...properties for hud %i have been updated.\n", temphud.hudnumber);
	}

	return qtrue;
}

static qboolean CG_ReadHudFile(const char *filename)
{
	pc_token_t token;
	int        handle;

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		return qfalse;
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "hudDef"))
	{
		return CG_HUD_ParseError(handle, "expected 'hudDef'");
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_HUD_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "hud"))
		{
			if (!CG_ParseHUD(handle))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_HUD_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	trap_PC_FreeSource(handle);

	return qtrue;
}

/**
 * @brief CG_ReadHudScripts
 */
void CG_ReadHudScripts(void)
{
	if (!CG_ReadHudFile("ui/huds.hud"))
	{
		Com_Printf("^1ERROR while reading hud file\n");
	}

	// This needs to be a .dat file to go around the file extension restrictions of the engine.
	CG_ReadHudFile("hud.dat");

	Com_Printf("...hud count: %i\n", hudCount);
}

// HUD DRAWING FUNCTIONS BELLOW

/**
 * @brief CG_DrawPicShadowed
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] icon
 */
static void CG_DrawPicShadowed(float x, float y, float w, float h, qhandle_t icon)
{
	trap_R_SetColor(colorBlack);
	CG_DrawPic(x + 2, y + 2, w, h, icon);
	trap_R_SetColor(NULL);
	CG_DrawPic(x, y, w, h, icon);
}

/**
 * @brief CG_DrawCompText
 * @param[in] comp
 * @param[in] str
 * @param[in] color
 */
static void CG_DrawCompText(hudComponent_t *comp, const char *str, vec4_t color, int fontStyle)
{
	float w, w2, h, h2;

	w  = CG_Text_Width_Ext(str, comp->scale, 0, &cgs.media.limboFont1);
	h  = CG_Text_Height_Ext(str, comp->scale, 0, &cgs.media.limboFont1);
	w2 = MAX(comp->location.w, w);
	h2 = MAX(comp->location.h, h);

	if (comp->style == STYLE_NORMAL)
	{
		CG_FillRect(comp->location.x, comp->location.y, w2, comp->location.h, HUD_Background);
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, w2, comp->location.h, 1, HUD_Border);
	}

	CG_Text_Paint_Ext(comp->location.x + ((w2 - w) / 2), comp->location.y + ((h2 + h) / 2), comp->scale, comp->scale, color, str, 0, 0, fontStyle, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawPlayerStatusHead
 * @param[in] comp
 */
static void CG_DrawPlayerStatusHead(hudComponent_t *comp)
{
	hudHeadAnimNumber_t anim           = cg.idleAnim;
	bg_character_t      *character     = CG_CharacterForPlayerstate(&cg.snap->ps);
	bg_character_t      *headcharacter = BG_GetCharacter(cgs.clientinfo[cg.snap->ps.clientNum].team, cgs.clientinfo[cg.snap->ps.clientNum].cls);
	qhandle_t           painshader     = 0;
	rectDef_t           *headRect      = &comp->location;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (cg.weaponFireTime > 500)
	{
		anim = HD_ATTACK;
	}
	else if (cg.time - cg.lastFiredWeaponTime < 500)
	{
		anim = HD_ATTACK_END;
	}
	else if (cg.time - cg.painTime < (character->hudheadanimations[HD_PAIN].numFrames * character->hudheadanimations[HD_PAIN].frameLerp))
	{
		anim = HD_PAIN;
	}
	else if (cg.time > cg.nextIdleTime)
	{
		cg.nextIdleTime = cg.time + 7000 + rand() % 1000;
		if (cg.snap->ps.stats[STAT_HEALTH] < 40)
		{
			cg.idleAnim = (hudHeadAnimNumber_t)((rand() % (HD_DAMAGED_IDLE3 - HD_DAMAGED_IDLE2 + 1)) + HD_DAMAGED_IDLE2);
		}
		else
		{
			cg.idleAnim = (hudHeadAnimNumber_t)((rand() % (HD_IDLE8 - HD_IDLE2 + 1)) + HD_IDLE2);
		}

		cg.lastIdleTimeEnd = cg.time + character->hudheadanimations[cg.idleAnim].numFrames * character->hudheadanimations[cg.idleAnim].frameLerp;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] < 5)
	{
		painshader = cgs.media.hudDamagedStates[3];
	}
	else if (cg.snap->ps.stats[STAT_HEALTH] < 20)
	{
		painshader = cgs.media.hudDamagedStates[2];
	}
	else if (cg.snap->ps.stats[STAT_HEALTH] < 40)
	{
		painshader = cgs.media.hudDamagedStates[1];
	}
	else if (cg.snap->ps.stats[STAT_HEALTH] < 60)
	{
		painshader = cgs.media.hudDamagedStates[0];
	}

	if (cg.time > cg.lastIdleTimeEnd)
	{
		if (cg.snap->ps.stats[STAT_HEALTH] < 40)
		{
			cg.idleAnim = HD_DAMAGED_IDLE1;
		}
		else
		{
			cg.idleAnim = HD_IDLE1;
		}
	}

	CG_DrawPlayerHead(headRect, character, headcharacter, 180, 0, (cg.snap->ps.eFlags & EF_HEADSHOT) ? qfalse : qtrue, anim, painshader, cgs.clientinfo[cg.snap->ps.clientNum].rank, qfalse, cgs.clientinfo[cg.snap->ps.clientNum].team);
}

/**
 * @brief Get the current ammo and/or clip count of the holded weapon (if using ammo).
 * @param[out] ammo - the number of ammo left (in the current clip if using clip)
 * @param[out] clips - the total ammount of ammo in all clips (if using clip)
 * @param[out] akimboammo - the number of ammo left in the second pistol of akimbo (if using akimbo)
 */
void CG_PlayerAmmoValue(int *ammo, int *clips, int *akimboammo)
{
	centity_t     *cent;
	playerState_t *ps;
	weapon_t      weap;

	*ammo = *clips = *akimboammo = -1;

	if (cg.snap->ps.clientNum == cg.clientNum)
	{
		cent = &cg.predictedPlayerEntity;
	}
	else
	{
		cent = &cg_entities[cg.snap->ps.clientNum];
	}
	ps = &cg.snap->ps;

	weap = (weapon_t)cent->currentState.weapon;

	if (!IS_VALID_WEAPON(weap))
	{
		return;
	}

	// some weapons don't draw ammo count
	if (!GetWeaponTableData(weap)->useAmmo)
	{
		return;
	}

	if (BG_PlayerMounted(cg.snap->ps.eFlags))
	{
		return;
	}

	// total ammo in clips, grenade launcher is not a clip weapon but show the clip anyway
	if (GetWeaponTableData(weap)->useClip || (weap == WP_M7 || weap == WP_GPG40))
	{
		// current reserve
		*clips = cg.snap->ps.ammo[GetWeaponTableData(weap)->ammoIndex];

		// current clip
		*ammo = ps->ammoclip[GetWeaponTableData(weap)->clipIndex];
	}
	else
	{
		// some weapons don't draw ammo clip count text
		*ammo = ps->ammoclip[GetWeaponTableData(weap)->clipIndex] + cg.snap->ps.ammo[GetWeaponTableData(weap)->ammoIndex];
	}

	// akimbo ammo clip
	if (GetWeaponTableData(weap)->attributes & WEAPON_ATTRIBUT_AKIMBO)
	{
		*akimboammo = ps->ammoclip[GetWeaponTableData(GetWeaponTableData(weap)->akimboSideArm)->clipIndex];
	}
	else
	{
		*akimboammo = -1;
	}

	if (weap == WP_LANDMINE)
	{
		if (!cgs.gameManager)
		{
			*ammo = 0;
		}
		else
		{
			if (cgs.clientinfo[ps->clientNum].team == TEAM_AXIS)
			{
				*ammo = cgs.gameManager->currentState.otherEntityNum;
			}
			else
			{
				*ammo = cgs.gameManager->currentState.otherEntityNum2;
			}
		}
	}
}

/**
 * @brief Check if we are underwater
 * @details This check has changed to make it work for spectators following another player.
 * That's why ps.stats[STAT_AIRLEFT] has been added..
 *
 * While following high-pingers, You sometimes see the breathbar, even while they are not submerged..
 * So we check for underwater status differently when we are following others.
 * (It doesn't matter to do a more complex check for spectators.. they are not playing)
 * @return
 */
static qboolean CG_CheckPlayerUnderwater()
{
	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		vec3_t origin;

		VectorCopy(cg.snap->ps.origin, origin);
		origin[2] += 36;
		return (qboolean)(CG_PointContents(origin, cg.snap->ps.clientNum) & CONTENTS_WATER);
	}

	return cg.snap->ps.stats[STAT_AIRLEFT] < HOLDBREATHTIME;
}

vec4_t bgcolor = { 1.f, 1.f, 1.f, .3f };    // bars backgound

/**
 * @brief CG_DrawPlayerHealthBar
 * @param[in] rect
 */
static void CG_DrawPlayerHealthBar(hudComponent_t *comp)
{
	vec4_t colour;
	int    flags = 1 | 4 | 16 | 64;
	float  frac;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	CG_ColorForHealth(colour);
	colour[3] = 0.5f;

	frac = cg.snap->ps.stats[STAT_HEALTH] / (float) cg.snap->ps.stats[STAT_MAX_HEALTH];

	CG_FilledBar(comp->location.x, comp->location.y + (comp->location.h * 0.1f), comp->location.w, comp->location.h * 0.84f, colour, NULL, bgcolor, frac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.hudSprintBar);
	CG_DrawPic(comp->location.x, comp->location.y + comp->location.h + 4, comp->location.w, comp->location.w, cgs.media.hudHealthIcon);
}

/**
 * @brief CG_DrawStaminaBar
 * @param[in] rect
 */
static void CG_DrawStaminaBar(hudComponent_t *comp)
{
	vec4_t colour = { 0.1f, 1.0f, 0.1f, 0.5f };
	vec_t  *color = colour;
	int    flags  = 1 | 4 | 16 | 64;
	float  frac   = cg.snap->ps.stats[STAT_SPRINTTIME] / (float)SPRINTTIME;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (CG_CheckPlayerUnderwater())
	{
		return;
	}

	if (cg.snap->ps.powerups[PW_ADRENALINE])
	{
		if (cg.snap->ps.pm_flags & PMF_FOLLOW)
		{
			Vector4Average(colour, colorWhite, (float)sin(cg.time * .005), colour);
		}
		else
		{
			float msec = cg.snap->ps.powerups[PW_ADRENALINE] - cg.time;

			if (msec >= 0)
			{
				Vector4Average(colour, colorMdRed, (float)(.5 + sin(.2 * sqrt((double)msec) * M_TAU_F) * .5), colour);
			}
		}
	}
	else
	{
		color[0] = 1.0f - frac;
		color[1] = frac;
	}

	CG_FilledBar(comp->location.x, comp->location.y + (comp->location.h * 0.1f), comp->location.w, comp->location.h * 0.84f, color, NULL, bgcolor, frac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.hudSprintBar);
	CG_DrawPic(comp->location.x, comp->location.y + comp->location.h + 4, comp->location.w, comp->location.w, cgs.media.hudSprintIcon);
}

/**
 * @brief Draw the breath bar
 * @param[in] rect
 */
static void CG_DrawBreathBar(hudComponent_t *comp)
{
	static vec4_t colour = { 0.1f, 0.1f, 1.0f, 0.5f };
	vec_t         *color = colour;
	int           flags  = 1 | 4 | 16 | 64;
	float         frac   = cg.snap->ps.stats[STAT_AIRLEFT] / (float)HOLDBREATHTIME;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (!CG_CheckPlayerUnderwater())
	{
		return;
	}

	color[0] = 1.0f - frac;
	color[2] = frac;

	CG_FilledBar(comp->location.x, comp->location.y + (comp->location.h * 0.1f), comp->location.w, comp->location.h * 0.84f, color, NULL, bgcolor, frac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.hudSprintBar);
	CG_DrawPic(comp->location.x, comp->location.y + comp->location.h + 4, comp->location.w, comp->location.w, cgs.media.waterHintShader);
}

/**
 * @brief Draw weapon recharge bar
 * @param rect
 */
static void CG_DrawWeapRecharge(hudComponent_t *comp)
{
	float    barFrac, chargeTime;
	int      flags  = 1 | 4 | 16;
	qboolean charge = qtrue;
	vec4_t   color;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	// Draw power bar
	switch (cg.snap->ps.stats[STAT_PLAYER_CLASS])
	{
	case PC_ENGINEER:
		chargeTime = cg.engineerChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_MEDIC:
		chargeTime = cg.medicChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_FIELDOPS:
		chargeTime = cg.fieldopsChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_COVERTOPS:
		chargeTime = cg.covertopsChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	default:
		chargeTime = cg.soldierChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	}

	// display colored charge bar if charge bar isn't full enough
	if (GetWeaponTableData(cg.predictedPlayerState.weapon)->attributes & WEAPON_ATTRIBUT_CHARGE_TIME)
	{
		int index = BG_IsSkillAvailable(cgs.clientinfo[cg.clientNum].skill,
		                                GetWeaponTableData(cg.predictedPlayerState.weapon)->skillBased,
		                                GetWeaponTableData(cg.predictedPlayerState.weapon)->chargeTimeSkill);

		float coeff = GetWeaponTableData(cg.predictedPlayerState.weapon)->chargeTimeCoeff[index];

		if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * coeff)
		{
			charge = qfalse;
		}
	}
	else if ((cg.predictedPlayerState.eFlags & EF_ZOOMING || cg.predictedPlayerState.weapon == WP_BINOCULARS)
	         && cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_FIELDOPS)
	{
		int index = BG_IsSkillAvailable(cgs.clientinfo[cg.clientNum].skill,
		                                GetWeaponTableData(WP_ARTY)->skillBased,
		                                GetWeaponTableData(WP_ARTY)->chargeTimeSkill);

		float coeff = GetWeaponTableData(WP_ARTY)->chargeTimeCoeff[index];

		if (cg.time - cg.snap->ps.classWeaponTime < chargeTime * coeff)
		{
			charge = qfalse;
		}
	}

	barFrac = (cg.time - cg.snap->ps.classWeaponTime) / chargeTime; // FIXME: potential DIV 0 when charge times are set to 0!

	if (barFrac > 1.0f)
	{
		barFrac = 1.0f;
	}

	if (!charge)
	{
		color[0] = 1.0f;
		color[1] = color[2] = 0.1f;
		color[3] = 0.5f;
	}
	else
	{
		color[0] = color[1] = 1.0f;
		color[2] = barFrac;
		color[3] = 0.25f + barFrac * 0.5f;
	}

	CG_FilledBar(comp->location.x, comp->location.y + (comp->location.h * 0.1f), comp->location.w, comp->location.h * 0.84f, color, NULL, bgcolor, barFrac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.hudSprintBar);

	if (cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_FIELDOPS)
	{
		if (cg.snap->ps.ammo[WP_ARTY] & NO_AIRSTRIKE && cg.snap->ps.ammo[WP_ARTY] & NO_ARTILLERY)
		{
			trap_R_SetColor(colorRed);
		}
		else if (cg.snap->ps.ammo[WP_ARTY] & NO_AIRSTRIKE)
		{
			trap_R_SetColor(colorOrange);
		}
		else if (cg.snap->ps.ammo[WP_ARTY] & NO_ARTILLERY)
		{
			trap_R_SetColor(colorYellow);
		}
		CG_DrawPic(comp->location.x + (comp->location.w * 0.25f) - 1, comp->location.y + comp->location.h + 4, (comp->location.w * 0.5f) + 2, comp->location.w + 2, cgs.media.hudPowerIcon);
		trap_R_SetColor(NULL);
	}
	else
	{
		CG_DrawPic(comp->location.x + (comp->location.w * 0.25f) - 1, comp->location.y + comp->location.h + 4, (comp->location.w * 0.5f) + 2, comp->location.w + 2, cgs.media.hudPowerIcon);
	}
}

/**
 * @brief CG_DrawGunIcon
 * @param[in] location
 */
static void CG_DrawGunIcon(hudComponent_t *comp)
{
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	// Draw weapon icon and overheat bar
	CG_DrawWeapHeat(&comp->location, HUD_HORIZONTAL);

	// drawn the common white icon, usage of mounted weapons don't change cg.snap->ps.weapon for real
	if (BG_PlayerMounted(cg.snap->ps.eFlags))
	{
		CG_DrawPlayerWeaponIcon(&comp->location, qtrue, ITEM_ALIGN_RIGHT, &comp->color);
		return;
	}

	if (
#ifdef FEATURE_MULTIVIEW
		cg.mvTotalClients < 1 &&
#endif
		cg_drawWeaponIconFlash.integer == 0)
	{
		CG_DrawPlayerWeaponIcon(&comp->location, qtrue, ITEM_ALIGN_RIGHT, &comp->color);
	}
	else
	{
		int ws =
#ifdef FEATURE_MULTIVIEW
			(cg.mvTotalClients > 0) ? cgs.clientinfo[cg.snap->ps.clientNum].weaponState :
#endif
			BG_simpleWeaponState(cg.snap->ps.weaponstate);

		CG_DrawPlayerWeaponIcon(&comp->location, (qboolean)(ws != WSTATE_IDLE), ITEM_ALIGN_RIGHT, ((ws == WSTATE_SWITCH || ws == WSTATE_RELOAD) ? &colorYellow : (ws == WSTATE_FIRE) ? &colorRed : &comp->color));
	}
}

/**
 * @brief CG_DrawAmmoCount
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawAmmoCount(hudComponent_t *comp)
{
	int  value, value2, value3;
	char buffer[16] = { 0 };

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	// Draw ammo
	CG_PlayerAmmoValue(&value, &value2, &value3);

	// .25f
	if (value3 >= 0)
	{
		Com_sprintf(buffer, sizeof(buffer), "%i|%i/%i", value3, value, value2);
	}
	else if (value2 >= 0)
	{
		Com_sprintf(buffer, sizeof(buffer), "%i/%i", value, value2);
	}
	else if (value >= 0)
	{
		Com_sprintf(buffer, sizeof(buffer), "%i", value);
	}

	if (buffer[0] != '\0')
	{
		float w, w2, h, h2;

		w  = CG_Text_Width_Ext(buffer, comp->scale, 0, &cgs.media.limboFont1);
		h  = CG_Text_Height_Ext(buffer, comp->scale, 0, &cgs.media.limboFont1);
		w2 = MAX(comp->location.w, w);
		h2 = MAX(comp->location.h, h);

		if (comp->style == STYLE_NORMAL)
		{
			CG_FillRect(comp->location.x, comp->location.y, w2, comp->location.h, HUD_Background);
			CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, w2, comp->location.h, 1, HUD_Border);
		}

		CG_Text_Paint_Ext(comp->location.x + ((w2 - w) / 2), comp->location.y + ((h2 + h) / 2), comp->scale, comp->scale, comp->color, buffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
}

/**
 * @brief CG_DrawSkillBar
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] skillLvl
 */
static void CG_DrawSkillBar(float x, float y, float w, float h, int skillLvl, skillType_t skill)
{
	int    i;
	float  blockheight = (h - 4) / (float)(NUM_SKILL_LEVELS - 1);
	float  draw_y      = y + h - blockheight;
	vec4_t colour;
	float  x1, y1, w1, h1;

	for (i = 1; i < NUM_SKILL_LEVELS; i++)
	{

		if (GetSkillTableData(skill)->skillLevels[i] < 0)
		{
			Vector4Set(colour, 1.f, 0.f, 0.f, .15f);
		}
		else if (skillLvl >= i)
		{
			Vector4Set(colour, 0.f, 0.f, 0.f, .4f);
		}
		else
		{
			Vector4Set(colour, 1.f, 1.f, 1.f, .15f);
		}

		CG_FillRect(x, draw_y, w, blockheight, colour);

		// draw the star only if the skill is reach and available
		if (skillLvl >= i && GetSkillTableData(skill)->skillLevels[i] >= 0)
		{
			x1 = x;
			y1 = draw_y;
			w1 = w;
			h1 = blockheight;
			CG_AdjustFrom640(&x1, &y1, &w1, &h1);

			trap_R_DrawStretchPic(x1, y1, w1, h1, 0, 0, 1.f, 0.5f, cgs.media.limboStar_roll);
		}

		CG_DrawRect_FixedBorder(x, draw_y, w, blockheight, 1, colorBlack);
		draw_y -= (blockheight + 1);
	}
}

/**
 * @brief CG_ClassSkillForPosition
 * @param[in] ci
 * @param[in] pos
 * @return
 */
skillType_t CG_ClassSkillForPosition(clientInfo_t *ci, int pos)
{
	switch (pos)
	{
	case 0:
		return BG_ClassSkillForClass(ci->cls);
	case 1:
		return SK_BATTLE_SENSE;
	case 2:
		// draw soldier level if using a heavy weapon instead of light weapons icon
		if ((BG_PlayerMounted(cg.snap->ps.eFlags) || GetWeaponTableData(cg.snap->ps.weapon)->skillBased == SK_HEAVY_WEAPONS) && ci->cls != PC_SOLDIER)
		{
			return SK_HEAVY_WEAPONS;
		}
		return SK_LIGHT_WEAPONS;
	default:
		break;
	}

	return SK_BATTLE_SENSE;
}

/**
 * @brief CG_DrawPlayerHealth
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawPlayerHealth(hudComponent_t *comp)
{
	const char *str = va("%i HP", cg.snap->ps.stats[STAT_HEALTH]);
	vec4_t     color;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (cg_healthDynamicColor.integer)
	{
		CG_GetColorForHealth(cg.snap->ps.stats[STAT_HEALTH], color);
		color[3] = comp->color[3];
	}
	else
	{
		Vector4Copy(comp->color, color);
	}

	CG_DrawCompText(comp, str, color, ITEM_TEXTSTYLE_SHADOWED);
}

/**
 * @brief CG_DrawPlayerSprint
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawPlayerSprint(hudComponent_t *comp)
{
	const char *str;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (CG_CheckPlayerUnderwater())
	{
		return;
	}

	if (cg.snap->ps.powerups[PW_ADRENALINE])
	{
		str = va("%d s", (cg.snap->ps.powerups[PW_ADRENALINE] - cg.time) / 1000);
	}
	else
	{
		str = va("%.0f %%", (cg.snap->ps.stats[STAT_SPRINTTIME] / (float)SPRINTTIME) * 100);
	}

	CG_DrawCompText(comp, str, comp->color, ITEM_TEXTSTYLE_SHADOWED);
}

/**
 * @brief CG_DrawPlayerBreath
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawPlayerBreath(hudComponent_t *comp)
{
	const char *str = va("%.0f %%", (cg.snap->ps.stats[STAT_AIRLEFT] / (float)HOLDBREATHTIME) * 100);

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (!CG_CheckPlayerUnderwater())
	{
		return;
	}

	CG_DrawCompText(comp, str, comp->color, ITEM_TEXTSTYLE_SHADOWED);
}

/**
 * @brief CG_DrawWeaponCharge
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawWeaponCharge(hudComponent_t *comp)
{
	const char *str;
	float      chargeTime;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	switch (cg.snap->ps.stats[STAT_PLAYER_CLASS])
	{
	case PC_ENGINEER:
		chargeTime = cg.engineerChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_MEDIC:
		chargeTime = cg.medicChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_FIELDOPS:
		chargeTime = cg.fieldopsChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	case PC_COVERTOPS:
		chargeTime = cg.covertopsChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	default:
		chargeTime = cg.soldierChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		break;
	}

	str = va("%.0f %%", MIN(((cg.time - cg.snap->ps.classWeaponTime) / chargeTime) * 100, 100));

	CG_DrawCompText(comp, str, comp->color, ITEM_TEXTSTYLE_SHADOWED);
}

/**
 * @brief CG_DrawSkills
 * @param[in] comp
 */
static void CG_DrawSkills(hudComponent_t *comp)
{
	playerState_t *ps = &cg.snap->ps;
	clientInfo_t  *ci = &cgs.clientinfo[ps->clientNum];
	skillType_t   skill;
	int           i;
	float         temp;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cgs.gametype == GT_WOLF_LMS)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	for (i = 0; i < 3; i++)
	{
		skill = CG_ClassSkillForPosition(ci, i);
		if (comp->style == STYLE_NORMAL)
		{
			CG_DrawSkillBar(comp->location.x + i * comp->location.w, comp->location.y, comp->location.w, comp->location.h - comp->location.w, ci->skill[skill], skill);
			CG_DrawPic(comp->location.x + i * comp->location.w, comp->location.y + comp->location.h - comp->location.w, comp->location.w, comp->location.w, cgs.media.skillPics[skill]);
		}
		else
		{
			int j        = 1;
			int skillLvl = 0;

			for (; j < NUM_SKILL_LEVELS; ++j)
			{
				if (BG_IsSkillAvailable(ci->skill, skill, j))
				{
					skillLvl++;
				}
			}

			temp = comp->location.y + (i * comp->location.w * 1.7f);
			//CG_DrawPic
			CG_DrawPicShadowed(comp->location.x, temp, comp->location.w, comp->location.w, cgs.media.skillPics[skill]);
			CG_Text_Paint_Ext(comp->location.x + 3, temp + 24, comp->scale, comp->scale, comp->color, va("%i", skillLvl), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}
	}
}

/**
 * @brief CG_DrawXP
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawXP(hudComponent_t *comp)
{
	const char *str;
	vec_t      *clr;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cgs.gametype == GT_WOLF_LMS)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (cg.time - cg.xpChangeTime < 1000)
	{
		clr = colorYellow;
	}
	else
	{
		clr = comp->color;
	}

	str = va("%i XP", cg.snap->ps.stats[STAT_XP]);

	CG_DrawCompText(comp, str, clr, ITEM_TEXTSTYLE_SHADOWED);
}

/**
 * @brief CG_DrawRank
 * @param[in] x
 * @param[in] y
 */
static void CG_DrawRank(hudComponent_t *comp)
{
	const char    *str;
	playerState_t *ps = &cg.snap->ps;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cgs.gametype == GT_WOLF_LMS)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	str = va("%s", GetRankTableData(cgs.clientinfo[ps->clientNum].team, cgs.clientinfo[ps->clientNum].rank)->miniNames);

	CG_DrawCompText(comp, str, comp->color, ITEM_TEXTSTYLE_SHADOWED);
}

/**
 * @brief CG_DrawPowerUps
 * @param[in] rect
 */
static void CG_DrawPowerUps(hudComponent_t *comp)
{
	playerState_t *ps = &cg.snap->ps;

	if (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR && !cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	// draw treasure icon if we have the flag
	if (ps->powerups[PW_REDFLAG] || ps->powerups[PW_BLUEFLAG])
	{
		trap_R_SetColor(NULL);
		CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveShader);
	}
	else if (ps->powerups[PW_OPS_DISGUISED])       // Disguised?
	{
		CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
		// show the class to the client
		CG_DrawPic(comp->location.x + 9, comp->location.y + 9, 18, 18, cgs.media.skillPics[BG_ClassSkillForClass((cg_entities[ps->clientNum].currentState.powerups >> PW_OPS_CLASS_1) & 7)]);
	}
	else if (ps->powerups[PW_ADRENALINE] > 0)       // adrenaline
	{
		vec4_t color = { 1.0, 0.0, 0.0, 1.0 };
		color[3] *= 0.5 + 0.5 * sin(cg.time / 150.0);
		trap_R_SetColor(color);
		CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.hudAdrenaline);
		trap_R_SetColor(NULL);
	}
	else if (ps->powerups[PW_INVULNERABLE] && !(ps->pm_flags & PMF_LIMBO))       // spawn shield
	{
		CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.spawnInvincibleShader);
	}
}

/**
 * @brief CG_DrawObjectiveStatus
 * @param[in] rect
 */
static void CG_DrawObjectiveStatus(hudComponent_t *comp)
{
	playerState_t *ps = &cg.snap->ps;

	if (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR && !cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	// draw objective status icon
	if ((cg.flagIndicator & (1 << PW_REDFLAG) || cg.flagIndicator & (1 << PW_BLUEFLAG)) && (!cgs.clientinfo[cg.clientNum].shoutcaster || (cg.snap->ps.pm_flags & PMF_FOLLOW)))
	{
		// draw objective info icon (if teammates or enemies are carrying one)
		vec4_t color = { 1.f, 1.f, 1.f, 1.f };
		color[3] *= 0.67 + 0.33 * sin(cg.time / 200.0);
		trap_R_SetColor(color);

		if (cg.flagIndicator & (1 << PW_REDFLAG) && cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			if (cg.redFlagCounter > 0 && cg.blueFlagCounter > 0)
			{
				// both own and enemy flags stolen
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveBothTEShader);
			}
			else if (cg.redFlagCounter > 0 && !cg.blueFlagCounter)
			{
				// own flag stolen and enemy flag dropped
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.objectiveBothTDShader : cgs.media.objectiveBothDEShader);
			}
			else if (!cg.redFlagCounter && cg.blueFlagCounter > 0)
			{
				// own flag dropped and enemy flag stolen
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, ps->persistant[PERS_TEAM] == TEAM_ALLIES ? cgs.media.objectiveBothTDShader : cgs.media.objectiveBothDEShader);
			}
			else
			{
				// both own and enemy flags dropped
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveDroppedShader);
			}
			trap_R_SetColor(NULL);

			// display team flag
			color[3] = 1.f;
			trap_R_SetColor(color);
			CG_DrawPic(comp->location.x + comp->location.w / 2 - 20, comp->location.y + 28, 12, 8, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.axisFlag : cgs.media.alliedFlag);
			CG_DrawPic(comp->location.x + comp->location.w / 2 + 8, comp->location.y + 28, 12, 8, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.alliedFlag : cgs.media.axisFlag);
		}
		else if (cg.flagIndicator & (1 << PW_REDFLAG))
		{
			if (cg.redFlagCounter > 0)
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, ps->persistant[PERS_TEAM] == TEAM_ALLIES ? cgs.media.objectiveTeamShader : cgs.media.objectiveEnemyShader);
			}
			else
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveDroppedShader);
			}
			trap_R_SetColor(NULL);

			// display team flag
			color[3] = 1.f;
			trap_R_SetColor(color);
			CG_DrawPic(comp->location.x + comp->location.w / 2 + (ps->persistant[PERS_TEAM] == TEAM_AXIS ? 8 : -20), comp->location.y + 28, 12, 8, cgs.media.alliedFlag);
		}
		else if (cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			if (cg.blueFlagCounter > 0)
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, ps->persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.objectiveTeamShader : cgs.media.objectiveEnemyShader);
			}
			else
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveDroppedShader);
			}
			trap_R_SetColor(NULL);

			// display team flag
			color[3] = 1.f;
			trap_R_SetColor(color);
			CG_DrawPic(comp->location.x + comp->location.w / 2 + (ps->persistant[PERS_TEAM] == TEAM_ALLIES ? 8 : -20), comp->location.y + 28, 12, 8, cgs.media.axisFlag);
		}

		// display active flag counter
		if (cg.redFlagCounter > 1)
		{
			CG_Text_Paint_Ext(comp->location.x + comp->location.w / 2 + (ps->persistant[PERS_TEAM] == TEAM_ALLIES ? -16 : 12), comp->location.y + 38, 0.18, 0.18, colorWhite, va("%i", cg.redFlagCounter), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}
		if (cg.blueFlagCounter > 1)
		{
			CG_Text_Paint_Ext(comp->location.x + comp->location.w / 2 + (ps->persistant[PERS_TEAM] == TEAM_AXIS ? -16 : 12), comp->location.y + 38, 0.18, 0.18, colorWhite, va("%i", cg.blueFlagCounter), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}

		trap_R_SetColor(NULL);
	}
	else if (cgs.clientinfo[cg.clientNum].shoutcaster && !(cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		// simplified version for shoutcaster when not following players
		vec4_t color = { 1.f, 1.f, 1.f, 1.f };
		color[3] *= 0.67 + 0.33 * sin(cg.time / 200.0);
		trap_R_SetColor(color);

		if (cg.flagIndicator & (1 << PW_REDFLAG) && cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			if (cg.redFlagCounter > 0 && cg.blueFlagCounter > 0)
			{
				// both team stole an enemy flags
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveTeamShader);
			}
			else if ((cg.redFlagCounter > 0 && !cg.blueFlagCounter) || (!cg.redFlagCounter && cg.blueFlagCounter > 0))
			{
				// one flag stolen and the other flag dropped
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveTeamShader);
			}
			else
			{
				// both team dropped flags
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveDroppedShader);
			}
		}
		else if (cg.flagIndicator & (1 << PW_REDFLAG))
		{
			if (cg.redFlagCounter > 0)
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveTeamShader);
			}
			else
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveDroppedShader);
			}
		}
		else if (cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			if (cg.blueFlagCounter > 0)
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveTeamShader);
			}
			else
			{
				CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cgs.media.objectiveDroppedShader);
			}
		}
		trap_R_SetColor(NULL);

		// display team flag
		color[3] = 1.f;
		trap_R_SetColor(color);

		if (cg.flagIndicator & (1 << PW_REDFLAG))
		{
			CG_DrawPic(comp->location.x + comp->location.w / 2 + 8, comp->location.y + 28, 12, 8, cgs.media.alliedFlag);
		}

		if (cg.flagIndicator & (1 << PW_BLUEFLAG))
		{
			CG_DrawPic(comp->location.x + comp->location.w / 2 - 20, comp->location.y + 28, 12, 8, cgs.media.axisFlag);
		}

		// display active flag counter
		if (cg.redFlagCounter > 1)
		{
			CG_Text_Paint_Ext(comp->location.x + comp->location.w / 2 + 12, comp->location.y + 38, 0.18, 0.18, colorWhite, va("%i", cg.redFlagCounter), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}
		if (cg.blueFlagCounter > 1)
		{
			CG_Text_Paint_Ext(comp->location.x + comp->location.w / 2 - 16, comp->location.y + 38, 0.18, 0.18, colorWhite, va("%i", cg.blueFlagCounter), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}

		trap_R_SetColor(NULL);
	}
}

static int lastDemoScoreTime = 0;

/**
 * @brief CG_DrawDemoMessage
 */
static void CG_DrawDemoMessage(hudComponent_t *comp)
{
	char status[1024];
	char demostatus[128];
	char wavestatus[128];

	if (!comp->visible)
	{
		return;
	}

	if (!cl_demorecording.integer && !cl_waverecording.integer && !cg.demoPlayback)
	{
		return;
	}

	// poll for score
	if ((!lastDemoScoreTime || cg.time > lastDemoScoreTime) && !cg.demoPlayback)
	{
		trap_SendClientCommand("score");
		lastDemoScoreTime = cg.time + 5000; // 5 secs
	}

	if (comp->style == STYLE_NORMAL)
	{
		if (cl_demorecording.integer)
		{
			Com_sprintf(demostatus, sizeof(demostatus), __(" demo %s: %ik "), cl_demofilename.string, cl_demooffset.integer / 1024);
		}
		else
		{
			Q_strncpyz(demostatus, "", sizeof(demostatus));
		}

		if (cl_waverecording.integer)
		{
			Com_sprintf(wavestatus, sizeof(demostatus), __(" audio %s: %ik "), cl_wavefilename.string, cl_waveoffset.integer / 1024);
		}
		else
		{
			Q_strncpyz(wavestatus, "", sizeof(wavestatus));
		}
	}
	else
	{
		Q_strncpyz(demostatus, "", sizeof(demostatus));
		Q_strncpyz(wavestatus, "", sizeof(wavestatus));
	}

	Com_sprintf(status, sizeof(status), "%s%s%s", cg.demoPlayback ? __("REPLAY") : __("RECORD"), demostatus, wavestatus);


}

/**
 * @brief CG_DrawField
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] value
 * @param[in] charWidth
 * @param[in] charHeight
 * @param[in] dodrawpic
 * @param[in] leftAlign
 * @return
 */
int CG_DrawField(int x, int y, int width, int value, int charWidth, int charHeight, qboolean dodrawpic, qboolean leftAlign)
{
	char num[16], *ptr;
	int  l;
	int  frame;
	int  startx;

	if (width < 1)
	{
		return 0;
	}

	// draw number string
	if (width > 5)
	{
		width = 5;
	}

	switch (width)
	{
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf(num, sizeof(num), "%i", value);
	l = (int)strlen(num);
	if (l > width)
	{
		l = width;
	}

	if (!leftAlign)
	{
		x -= 2 + charWidth * (l);
	}

	startx = x;

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
		{
			frame = STAT_MINUS;
		}
		else
		{
			frame = *ptr - '0';
		}

		if (dodrawpic)
		{
			CG_DrawPic(x, y, charWidth, charHeight, cgs.media.numberShaders[frame]);
		}
		x += charWidth;
		ptr++;
		l--;
	}

	return startx;
}

/**
 * @brief CG_DrawLivesLeft
 * @param[in] comp
 */
void CG_DrawLivesLeft(hudComponent_t *comp)
{
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_RESPAWNS_LEFT] < 0)
	{
		return;
	}

	CG_DrawPic(comp->location.x, comp->location.y, comp->location.w, comp->location.h, cg.snap->ps.persistant[PERS_TEAM] == TEAM_ALLIES ? cgs.media.hudAlliedHelmet : cgs.media.hudAxisHelmet);

	CG_DrawField(comp->location.w - 4, comp->location.y, 3, cg.snap->ps.persistant[PERS_RESPAWNS_LEFT], 14, 20, qtrue, qtrue);
}

/**
 * @brief CG_DrawCursorhint_f
 * @param comp
 * @todo FIXME Find a better way to do it
 */
static void CG_DrawCursorhint_f(hudComponent_t *comp)
{
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	CG_DrawCursorhint(&comp->location);
}

/**
 * @brief CG_DrawWeapStability_f
 * @param[in] comp
 * @todo FIXME Find a better way to do it
 */
static void CG_DrawWeapStability_f(hudComponent_t *comp)
{
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	CG_DrawWeapStability(&comp->location);
}

static char statsDebugStrings[6][512];
static int  statsDebugTime[6];
static int  statsDebugTextWidth[6];
static int  statsDebugPos;

/**
 * @brief CG_InitStatsDebug
 */
void CG_InitStatsDebug(void)
{
	Com_Memset(&statsDebugStrings, 0, sizeof(statsDebugStrings));
	Com_Memset(&statsDebugTime, 0, sizeof(statsDebugTime));
	statsDebugPos = -1;
}

/**
 * @brief CG_StatsDebugAddText
 * @param[in] text
 */
void CG_StatsDebugAddText(const char *text)
{
	if (cg_debugSkills.integer)
	{
		statsDebugPos++;

		if (statsDebugPos >= 6)
		{
			statsDebugPos = 0;
		}

		Q_strncpyz(statsDebugStrings[statsDebugPos], text, 512);
		statsDebugTime[statsDebugPos]      = cg.time;
		statsDebugTextWidth[statsDebugPos] = CG_Text_Width_Ext(text, .15f, 0, &cgs.media.limboFont2);

		CG_Printf("%s\n", text);
	}
}

/**
 * @brief CG_GetCompassIcon
 * @param[in] ent
 * @param[in] drawAllVoicesChat get all icons voices chat, otherwise only request relevant icons voices chat (need medic/ammo ...)
 * @param[in] drawFireTeam draw fireteam members position
 * @param[in] drawPrimaryObj draw primary objective position
 * @param[in] drawSecondaryObj draw secondary objective position
 * @param[in] drawDynamic draw dynamic elements position (player revive, command map marker)
 * @return A valid compass icon handle otherwise 0
 */
qhandle_t CG_GetCompassIcon(entityState_t *ent, qboolean drawAllVoicesChat, qboolean drawFireTeam, qboolean drawPrimaryObj, qboolean drawSecondaryObj, qboolean drawDynamic, char *name)
{
	centity_t *cent = &cg_entities[ent->number];

	if (!cent->currentValid)
	{
		return 0;
	}

	switch (ent->eType)
	{
	case ET_PLAYER:
	{
		qboolean sameTeam = cg.predictedPlayerState.persistant[PERS_TEAM] == cgs.clientinfo[ent->clientNum].team;

		if (!cgs.clientinfo[ent->clientNum].infoValid)
		{
			return 0;
		}

		if (sameTeam && cgs.clientinfo[ent->clientNum].powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
		{
			return cgs.media.objectiveShader;
		}

		if (ent->eFlags & EF_DEAD)
		{
			if (drawDynamic &&
			    ((cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_MEDIC &&
			      cg.predictedPlayerState.stats[STAT_HEALTH] > 0 && ent->number == ent->clientNum && sameTeam) ||
			     (!(cg.snap->ps.pm_flags & PMF_FOLLOW) && cgs.clientinfo[cg.clientNum].shoutcaster)))
			{
				return cgs.media.medicReviveShader;
			}

			return 0;
		}

		if (sameTeam && cent->voiceChatSpriteTime > cg.time &&
		    (drawAllVoicesChat ||
		     (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_MEDIC && cent->voiceChatSprite == cgs.media.medicIcon) ||
		     (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_FIELDOPS && cent->voiceChatSprite == cgs.media.ammoIcon)))
		{
			// FIXME: not the best place to reset it
			if (cgs.clientinfo[ent->clientNum].health <= 0)
			{
				// reset
				cent->voiceChatSpriteTime = cg.time;
				return 0;
			}

			return cent->voiceChatSprite;
		}

		if (drawFireTeam && (CG_IsOnSameFireteam(cg.clientNum, ent->clientNum) || cgs.clientinfo[cg.clientNum].shoutcaster))
		{
			// draw overlapping no-shoot icon if disguised and in same team
			if (ent->powerups & (1 << PW_OPS_DISGUISED) && cg.predictedPlayerState.persistant[PERS_TEAM] == cgs.clientinfo[ent->clientNum].team)
			{
				return cgs.clientinfo[ent->clientNum].selected ? cgs.media.friendShader : 0;
			}
			return cgs.clientinfo[ent->clientNum].selected ? cgs.media.buddyShader : 0;
		}
		break;
	}
	case ET_ITEM:
	{
		gitem_t *item;

		item = BG_GetItem(ent->modelindex);

		if (item && item->giType == IT_TEAM)
		{
			if ((item->giPowerUp == PW_BLUEFLAG && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
			    || (item->giPowerUp == PW_REDFLAG && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES))
			{
				return cgs.media.objectiveBlueShader;
			}

			return cgs.media.objectiveRedShader;
		}
		break;
	}
	case ET_EXPLOSIVE_INDICATOR:
	{
		if (drawPrimaryObj)
		{
			oidInfo_t *oidInfo = &cgs.oidInfo[cent->currentState.modelindex2];
			int       entNum   = Q_atoi(
				CG_ConfigString(ent->teamNum == TEAM_AXIS ? CS_MAIN_AXIS_OBJECTIVE : CS_MAIN_ALLIES_OBJECTIVE));

			if (name)
			{
				Q_strncpyz(name, oidInfo->name, MAX_QPATH);
			}

			if (entNum == oidInfo->entityNum || oidInfo->spawnflags & (1 << 4))
			{
				if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.defendShader : cgs.media.attackShader;
				}
				else
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.attackShader : cgs.media.defendShader;
				}
			}
		}

		if (drawSecondaryObj)
		{
			// draw explosives if an engineer
			if (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_ENGINEER ||
			    (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_COVERTOPS && ent->effect1Time == 1))
			{
				if (ent->teamNum == 1 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return 0;
				}

				if (ent->teamNum == 2 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES)
				{
					return 0;
				}

				return cgs.media.destroyShader;
			}
		}
		break;
	}
	case ET_CONSTRUCTIBLE_INDICATOR:
	{
		if (drawPrimaryObj)
		{
			oidInfo_t *oidInfo = &cgs.oidInfo[cent->currentState.modelindex2];
			int       entNum   = Q_atoi(CG_ConfigString(ent->teamNum == TEAM_AXIS ? CS_MAIN_AXIS_OBJECTIVE : CS_MAIN_ALLIES_OBJECTIVE));

			if (name)
			{
				Q_strncpyz(name, oidInfo->name, MAX_QPATH);
			}

			if (entNum == oidInfo->entityNum || oidInfo->spawnflags & (1 << 4))
			{
				if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.defendShader : cgs.media.attackShader;
				}
				else
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.attackShader : cgs.media.defendShader;
				}
			}
		}

		if (drawSecondaryObj)
		{
			// draw construction if an engineer
			if (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_ENGINEER)
			{
				if (ent->teamNum == 1 && cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_AXIS)
				{
					return 0;
				}

				if (ent->teamNum == 2 && cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_ALLIES)
				{
					return 0;
				}

				return cgs.media.constructShader;
			}
		}
		break;
	}
	case ET_TANK_INDICATOR:
	{
		if (drawPrimaryObj)
		{
			oidInfo_t *oidInfo = &cgs.oidInfo[cent->currentState.modelindex2];
			int       entNum   = Q_atoi(CG_ConfigString(ent->teamNum == TEAM_AXIS ? CS_MAIN_AXIS_OBJECTIVE : CS_MAIN_ALLIES_OBJECTIVE));

			if (name)
			{
				Q_strncpyz(name, oidInfo->name, MAX_QPATH);
			}

			if (entNum == oidInfo->entityNum || oidInfo->spawnflags & (1 << 4))
			{
				if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.defendShader : cgs.media.attackShader;
				}
				else
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.attackShader : cgs.media.defendShader;
				}
			}
		}

		if (drawSecondaryObj)
		{
			// FIXME: show only when relevant
			if ((ent->teamNum == 1 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
			    || (ent->teamNum == 2 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES))
			{
				return cgs.media.escortShader;
			}

			return cgs.media.destroyShader;
		}
		break;
	}
	case ET_TANK_INDICATOR_DEAD:
	{
		if (drawPrimaryObj)
		{
			oidInfo_t *oidInfo = &cgs.oidInfo[cent->currentState.modelindex2];
			int       entNum   = Q_atoi(CG_ConfigString(ent->teamNum == TEAM_AXIS ? CS_MAIN_AXIS_OBJECTIVE : CS_MAIN_ALLIES_OBJECTIVE));

			if (name)
			{
				Q_strncpyz(name, oidInfo->name, MAX_QPATH);
			}

			if (entNum == oidInfo->entityNum || oidInfo->spawnflags & (1 << 4))
			{
				if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.defendShader : cgs.media.attackShader;
				}
				else
				{
					return ent->teamNum == TEAM_AXIS ? cgs.media.attackShader : cgs.media.defendShader;
				}
			}
		}

		if (drawSecondaryObj)
		{
			// FIXME: show only when relevant
			// draw repair if an engineer
			if (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_ENGINEER && (
					(ent->teamNum == 1 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS)
					|| (ent->teamNum == 2 && cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES)))
			{
				return cgs.media.constructShader;
			}
		}
		break;
	}
	case ET_TRAP:
	{
		if (drawSecondaryObj)
		{
			if (ent->frame == 0)
			{
				return cgs.media.regroupShader;
			}

			if (ent->frame == 4)
			{
				return cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_AXIS ? cgs.media.regroupShader : cgs.media.defendShader;
			}

			if (ent->frame == 3)
			{
				return cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_ALLIES ? cgs.media.regroupShader : cgs.media.defendShader;
			}
		}
		break;
	}
	// FIXME: ET_COMMANDMAP_MARKER, ET_HEALER, ET_SUPPLIER
	//case ET_MG42_BARREL:
	//{
	//    return cgs.media.mg42HintShader;
	//}
	//case ET_CABINET_H:
	//{
	//	return cgs.media.healthHintShader;
	//}
	//caseET_CABINET_A:
	//{
	//	return cgs.media.ammoHintShader;
	//}
	default:
		break;
	}

	return 0;
}

/**
 * @brief CG_CompasMoveLocationCalc
 * @param[out] locationvalue
 * @param[in] directionplus
 * @param[in] animationout
 */
static void CG_CompasMoveLocationCalc(float *locationvalue, qboolean directionplus, qboolean animationout)
{
	if (animationout)
	{
		if (directionplus)
		{
			*locationvalue += ((cg.time - cgs.autoMapExpandTime) / 100.f) * 128.f;
		}
		else
		{
			*locationvalue -= ((cg.time - cgs.autoMapExpandTime) / 100.f) * 128.f;
		}
	}
	else
	{
		if (!directionplus)
		{
			*locationvalue += (((cg.time - cgs.autoMapExpandTime - 150.f) / 100.f) * 128.f) - 128.f;
		}
		else
		{
			*locationvalue -= (((cg.time - cgs.autoMapExpandTime - 150.f) / 100.f) * 128.f) - 128.f;
		}
	}
}

/**
 * @brief CG_CompasMoveLocation
 * @param[in] basex
 * @param[in] basey
 * @param[in] basew
 * @param[in] animationout
 */
static void CG_CompasMoveLocation(float *basex, float *basey, float basew, qboolean animationout)
{
	float x    = *basex;
	float y    = *basey;
	float cent = basew / 2;
	x += cent;
	y += cent;

	if (x < Ccg_WideX(320))
	{
		if (y < 240)
		{
			if (x < y)
			{
				//move left
				CG_CompasMoveLocationCalc(basex, qfalse, animationout);
			}
			else
			{
				//move up
				CG_CompasMoveLocationCalc(basey, qfalse, animationout);
			}
		}
		else
		{
			if (x < (SCREEN_HEIGHT - y))
			{
				//move left
				CG_CompasMoveLocationCalc(basex, qfalse, animationout);
			}
			else
			{
				//move down
				CG_CompasMoveLocationCalc(basey, qtrue, animationout);
			}
		}
	}
	else
	{
		if (y < 240)
		{
			if ((Ccg_WideX(SCREEN_WIDTH) - x) < y)
			{
				//move right
				CG_CompasMoveLocationCalc(basex, qtrue, animationout);
			}
			else
			{
				//move up
				CG_CompasMoveLocationCalc(basey, qfalse, animationout);
			}
		}
		else
		{
			if ((Ccg_WideX(SCREEN_WIDTH) - x) < (SCREEN_HEIGHT - y))
			{
				//move right
				CG_CompasMoveLocationCalc(basex, qtrue, animationout);
			}
			else
			{
				//move down
				CG_CompasMoveLocationCalc(basey, qtrue, animationout);
			}
		}
	}
}

/**
 * @brief CG_DrawNewCompass
 * @param location
 */
void CG_DrawNewCompass(hudComponent_t *comp)
{
	float      basex = comp->location.x, basey = comp->location.y, basew = comp->location.w, baseh = comp->location.h;
	snapshot_t *snap;

	if (cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport)
	{
		snap = cg.nextSnap;
	}
	else
	{
		snap = cg.snap;
	}

	if ((snap->ps.pm_flags & PMF_LIMBO && !cgs.clientinfo[cg.clientNum].shoutcaster)
#ifdef FEATURE_MULTIVIEW
	    || cg.mvTotalClients > 0
#endif
	    )
	{
		CG_DrawExpandedAutoMap();
		return;
	}

	if (cgs.autoMapExpanded)
	{
		if (cg.time - cgs.autoMapExpandTime < 100.f)
		{
			CG_CompasMoveLocation(&basex, &basey, basew, qtrue);
		}
		else
		{
			CG_DrawExpandedAutoMap();
			return;
		}
	}
	else
	{
		if (cg.time - cgs.autoMapExpandTime <= 150.f)
		{
			CG_DrawExpandedAutoMap();
			return;
		}
		else if ((cg.time - cgs.autoMapExpandTime > 150.f) && (cg.time - cgs.autoMapExpandTime < 250.f))
		{
			CG_CompasMoveLocation(&basex, &basey, basew, qfalse);
		}
	}

	if ((snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR && !cgs.clientinfo[cg.clientNum].shoutcaster))
	{
		return;
	}

	CG_DrawAutoMap(basex, basey, basew, baseh, comp->style == STYLE_SIMPLE);
}
/**
 * @brief CG_DrawStatsDebug
 */
static void CG_DrawStatsDebug(void)
{
	int textWidth = 0;
	int i, x, y, w, h;

	if (!cg_debugSkills.integer)
	{
		return;
	}

	for (i = 0; i < 6; i++)
	{
		if (statsDebugTime[i] + 9000 > cg.time)
		{
			if (statsDebugTextWidth[i] > textWidth)
			{
				textWidth = statsDebugTextWidth[i];
			}
		}
	}

	w = textWidth + 6;
	h = 9;
	x = SCREEN_WIDTH - w;
	y = (SCREEN_HEIGHT - 5 * (12 + 2) + 6 - 4) - 6 - h;     // don't ask

	i = statsDebugPos;

	do
	{
		vec4_t colour;

		if (statsDebugTime[i] + 9000 <= cg.time)
		{
			break;
		}

		colour[0] = colour[1] = colour[2] = .5f;
		if (cg.time - statsDebugTime[i] > 5000)
		{
			colour[3] = .5f - .5f * ((cg.time - statsDebugTime[i] - 5000) / 4000.f);
		}
		else
		{
			colour[3] = .5f ;
		}
		CG_FillRect(x, y, w, h, colour);

		colour[0] = colour[1] = colour[2] = 1.f;
		if (cg.time - statsDebugTime[i] > 5000)
		{
			colour[3] = 1.f - ((cg.time - statsDebugTime[i] - 5000) / 4000.f);
		}
		else
		{
			colour[3] = 1.f ;
		}
		CG_Text_Paint_Ext(640.f - 3 - statsDebugTextWidth[i], y + h - 2, .15f, .15f, colour, statsDebugStrings[i], 0, 0, ITEM_TEXTSTYLE_NORMAL, &cgs.media.limboFont2);

		y -= h;

		i--;
		if (i < 0)
		{
			i = 6 - 1;
		}
	}
	while (i != statsDebugPos);
}

/*
===========================================================================================
  UPPER RIGHT CORNER
===========================================================================================
*/

#define UPPERRIGHT_X 634
#define UPPERRIGHT_W 52

/**
 * @brief CG_DrawSnapshot
 * @param[in] comp
 * @return
 */
static void CG_DrawSnapshot(hudComponent_t *comp)
{
	char  *s = va("t:%i\nsn:%i\ncmd:%i", cg.snap->serverTime, cg.latestSnapshotNum, cgs.serverCommandSequence);
	float w, w2, h, h2;

	w  = CG_Text_Width_Ext("XXXXXXX", comp->scale, 0, &cgs.media.limboFont1);
	h  = CG_Text_Height_Ext(s, comp->scale, 0, &cgs.media.limboFont1);
	w2 = MAX(comp->location.w, w);
	h2 = MAX(comp->location.h, h);

	if (comp->style == STYLE_NORMAL)
	{
		CG_FillRect(comp->location.x, comp->location.y, w2, comp->location.h, HUD_Background);
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, w2, comp->location.h, 1, HUD_Border);
	}

	CG_DrawMultilineText(comp->location.x + ((w2 - w) / 2), comp->location.y + ((h2 / 3 + h) / 2), comp->scale, comp->scale, comp->color, s, h * 2, 0, 0, 0, 0, &cgs.media.limboFont1);

	//CG_Text_Paint_Ext(comp->location.x + ((w2 - w) / 2), comp->location.y + ((h2 + h) / 2), comp->scale, comp->scale, comp->color, s, 0, 0, 0, &cgs.media.limboFont1);

	//CG_Text_Paint_Ext(comp->location.x + ((w2 - w) / 2), comp->location.y + ((h2 / 3 + h) / 2), comp->scale, comp->scale, comp->color, s, 0, 0, 0, &cgs.media.limboFont1);
	//s = va("sn:%i", cg.latestSnapshotNum);
	//CG_Text_Paint_Ext(comp->location.x + ((w2 - w) / 2), comp->location.y + ((h2 / 3 + h) / 2), comp->scale, comp->scale, comp->color, s, 0, 0, 0, &cgs.media.limboFont1);
	//s = va("cmd:%i", cgs.serverCommandSequence);
	//CG_Text_Paint_Ext(comp->location.x + ((w2 - w) / 2), comp->location.y + ((h2 / 3 + h) / 2), comp->scale, comp->scale, comp->color, s, 0, 0, 0, &cgs.media.limboFont1);
}

/**
 * @brief CG_DrawSpeed
 * @param[in] comp
 * @return
 */
static void CG_DrawSpeed(hudComponent_t *comp)
{
	static vec_t highestSpeed, speed;
	static int   lasttime;
	char         *s, *s2 = NULL;
	float        w, w2, w3, w4;
	int          thistime;
	int          h;

	if (resetmaxspeed)
	{
		highestSpeed  = 0;
		resetmaxspeed = qfalse;
	}

	thistime = trap_Milliseconds();

	if (thistime > lasttime + 100)
	{
		speed = VectorLength(cg.predictedPlayerState.velocity);

		if (speed > highestSpeed)
		{
			highestSpeed = speed;
		}

		lasttime = thistime;
	}

	switch (cg_drawUnit.integer)
	{
	case 0:
		// Units per second
		s  = va("%.1f UPS", speed);
		s2 = va("%.1f MAX", highestSpeed);
		break;
	case 1:
		// Kilometers per hour
		s  = va("%.1f KPH", (speed / SPEED_US_TO_KPH));
		s2 = va("%.1f MAX", (highestSpeed / SPEED_US_TO_KPH));
		break;
	case 2:
		// Miles per hour
		s  = va("%.1f MPH", (speed / SPEED_US_TO_MPH));
		s2 = va("%.1f MAX", (highestSpeed / SPEED_US_TO_MPH));
		break;
	default:
		s  = "";
		s2 = "";
		break;
	}

	h = comp->location.h * ((cg_drawspeed.integer == 2) ? 2 : 1);

	w  = CG_Text_Width_Ext(s, comp->scale, 0, &cgs.media.limboFont1);
	w2 = (comp->location.w > w) ? comp->location.w : w;

	if (comp->style == STYLE_NORMAL)
	{
		CG_FillRect(comp->location.x, comp->location.y, w2, h, HUD_Background);
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, w2, h, 1, HUD_Border);
	}
	CG_Text_Paint_Ext(comp->location.x + ((w2 - w) / 2), comp->location.y + 11, comp->scale, comp->scale, comp->color, s, 0, 0, 0, &cgs.media.limboFont1);

	// draw max speed on second line
	if (cg_drawspeed.integer == 2)
	{
		w3 = CG_Text_Width_Ext(s2, 0.19f, 0, &cgs.media.limboFont1);
		w4 = (UPPERRIGHT_W > w3) ? UPPERRIGHT_W : w3;
		CG_Text_Paint_Ext(comp->location.x + ((w4 - w3) / 2) + 2, comp->location.y + comp->location.h + 11, comp->scale, comp->scale, comp->color, s2, 0, 0, 0, &cgs.media.limboFont1);
	}
}

#define MAX_FPS_FRAMES  500

/**
 * @brief CG_DrawFPS
 * @param[in] comp
 * @return
 */
static void CG_DrawFPS(hudComponent_t *comp)
{
	static int previousTimes[MAX_FPS_FRAMES];
	static int previous;
	static int index;
	static int oldSamples;
	const char *s;
	int        t;
	int        frameTime;
	int        samples = cg_drawFPS.integer;

	t = trap_Milliseconds(); // don't use serverTime, because that will be drifting to correct for internet lag changes, timescales, timedemos, etc

	frameTime = t - previous;
	previous  = t;

	if (samples < 4)
	{
		samples = 4;
	}
	if (samples > MAX_FPS_FRAMES)
	{
		samples = MAX_FPS_FRAMES;
	}
	if (samples != oldSamples)
	{
		index = 0;
	}

	oldSamples                     = samples;
	previousTimes[index % samples] = frameTime;
	index++;

	if (index > samples)
	{
		int i, fps;
		// average multiple frames together to smooth changes out a bit
		int total = 0;

		for (i = 0 ; i < samples ; ++i)
		{
			total += previousTimes[i];
		}

		total = total ? total : 1;

		fps = 1000 * samples / total;

		s = va("%i FPS", fps);
	}
	else
	{
		s = "estimating";
	}

	CG_DrawCompText(comp, s, comp->color, ITEM_TEXTSTYLE_NORMAL);
}

/**
 * @brief CG_SpawnTimerText red colored spawn time text in reinforcement time HUD element.
 * @return red colored text or NULL when its not supposed to be rendered
*/
char *CG_SpawnTimerText()
{
	int msec = (cgs.timelimit * 60000.f) - (cg.time - cgs.levelStartTime);
	int seconds;
	int secondsThen;

	if (cg_spawnTimer_set.integer != -1 && cgs.gamestate == GS_PLAYING && !cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR || (cg.snap->ps.pm_flags & PMF_FOLLOW))
		{
			int period = cg_spawnTimer_period.integer > 0 ? cg_spawnTimer_period.integer : (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS ? cg_bluelimbotime.integer / 1000 : cg_redlimbotime.integer / 1000);
			if (period > 0) // prevent division by 0 for weird cases like limbtotime < 1000
			{
				seconds     = msec / 1000;
				secondsThen = ((cgs.timelimit * 60000.f) - cg_spawnTimer_set.integer) / 1000;
				return va("%i", period + (seconds - secondsThen) % period);
			}
		}
	}
	else if (cg_spawnTimer_set.integer != -1 && cg_spawnTimer_period.integer > 0 && cgs.gamestate != GS_PLAYING)
	{
		// We are not playing and the timer is set so reset/disable it
		// this happens for example when custom period is set by timerSet and map is restarted or changed
		trap_Cvar_Set("cg_spawnTimer_set", "-1");
	}
	return NULL;
}

/**
 * @brief CG_SpawnTimersText
 * @param[out] respawn
 * @param[out] spawntimer
 * @return
 */
static qboolean CG_SpawnTimersText(char **s, char **rt)
{
	if (cgs.gamestate != GS_PLAYING)
	{
		int limbotimeOwn, limbotimeEnemy;
		if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS)
		{
			limbotimeOwn   = cg_redlimbotime.integer;
			limbotimeEnemy = cg_bluelimbotime.integer;
		}
		else
		{
			limbotimeOwn   = cg_bluelimbotime.integer;
			limbotimeEnemy = cg_redlimbotime.integer;
		}

		*rt = va("%2.0i", limbotimeEnemy / 1000);
		*s  = cgs.gametype == GT_WOLF_LMS ? va("%s", CG_TranslateString("WARMUP")) : va("%2.0i", limbotimeOwn / 1000);
		return qtrue;
	}
	else if (cgs.gametype != GT_WOLF_LMS && (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR || (cg.snap->ps.pm_flags & PMF_FOLLOW)) && cg_drawReinforcementTime.integer > 0)
	{
		*s  = va("%2.0i", CG_CalculateReinfTime(qfalse));
		*rt = CG_SpawnTimerText();
	}
	return qfalse;
}

/**
 * @brief CG_RoundTimerText
 * @return
 */
static char *CG_RoundTimerText()
{
	qtime_t qt;
	int     msec = CG_RoundTime(&qt);
	if (msec < 0 && cgs.timelimit > 0.0f)
	{
		return "0:00"; // round ended
	}

	char *seconds = qt.tm_sec > 9 ? va("%i", qt.tm_sec) : va("0%i", qt.tm_sec);
	char *minutes = qt.tm_min > 9 ? va("%i", qt.tm_min) : va("0%i", qt.tm_min);

	return va("%s:%s", minutes, seconds);
}

/**
 * @brief CG_LocalTimeText
 * @return
 */
static char *CG_LocalTimeText()
{
	qtime_t  time;
	char     *s;
	qboolean pmtime = qfalse;

	//Fetch the local time
	trap_RealTime(&time);

	if (cg_drawTime.integer & LOCALTIME_SECOND)
	{
		if (cg_drawTime.integer & LOCALTIME_12HOUR)
		{
			if (time.tm_hour > 12)
			{
				pmtime = qtrue;
			}
			s = va("%i:%02i:%02i %s", (pmtime ? time.tm_hour - 12 : time.tm_hour), time.tm_min, time.tm_sec, (pmtime ? "PM" : "AM"));
		}
		else
		{
			s = va("%02i:%02i:%02i", time.tm_hour, time.tm_min, time.tm_sec);
		}
	}
	else
	{
		if (cg_drawTime.integer & LOCALTIME_12HOUR)
		{
			if (time.tm_hour > 12)
			{
				pmtime = qtrue;
			}
			s = va("%i:%02i %s", (pmtime ? time.tm_hour - 12 : time.tm_hour), time.tm_min, (pmtime ? "PM" : "AM"));
		}
		else
		{
			s = va("%02i:%02i", time.tm_hour, time.tm_min);
		}
	}
	return s;
}

/**
 * @brief CG_DrawRespawnTimer
 * @param respawn
 */
static void CG_DrawRespawnTimer(hudComponent_t *comp)
{
	char     *s = NULL, *rt = NULL;
	qboolean blink;

	if (cg_paused.integer)
	{
		return;
	}

	blink = CG_SpawnTimersText(&s, &rt);

	if (s)
	{
		CG_DrawCompText(comp, s, comp->color, blink ? ITEM_TEXTSTYLE_BLINK : ITEM_TEXTSTYLE_NORMAL);
	}
}

/**
 * @brief CG_DrawSpawnTimer
 * @param respawn
 */
static void CG_DrawSpawnTimer(hudComponent_t *comp)
{
	char     *s = NULL, *rt = NULL;
	qboolean blink;

	if (cg_paused.integer)
	{
		return;
	}

	blink = CG_SpawnTimersText(&s, &rt);

	if (rt)
	{
		CG_DrawCompText(comp, s, comp->color, blink ? ITEM_TEXTSTYLE_BLINK : ITEM_TEXTSTYLE_NORMAL);
	}
}

/**
 * @brief CG_DrawRoundTimerSimple
 * @param roundtimer
 */
static void CG_DrawRoundTimerSimple(hudComponent_t *comp)
{
	char     *s = NULL, *rt = NULL;
	qboolean blink;

	if (cg_paused.integer)
	{
		return;
	}

	blink = CG_SpawnTimersText(&s, &rt);

	CG_DrawCompText(comp, s, comp->color, blink ? ITEM_TEXTSTYLE_BLINK : ITEM_TEXTSTYLE_NORMAL);
}

/**
 * @brief CG_DrawTimerNormal
 * @param[in] y
 * @return
 */
static void CG_DrawRoundTimerNormal(hudComponent_t *comp)
{
	char     *s = NULL, *rt = NULL, *mt;
	qboolean blink;

	if (cg_paused.integer)
	{
		return;
	}

	blink = CG_SpawnTimersText(&s, &rt);

	mt = va("%s%s", "^7", CG_RoundTimerText());

	if (s)
	{
		s = va("^$%s%s%s", s, " ", mt);
	}
	else
	{
		s = mt;
	}

	if (rt)
	{
		s = va("^1%s%s%s", rt, " ", s);
	}

	CG_DrawCompText(comp, s, comp->color, blink ? ITEM_TEXTSTYLE_BLINK : ITEM_TEXTSTYLE_NORMAL);
}

/**
 * @brief CG_DrawRoundTimer
 * @param comp
 */
static void CG_DrawRoundTimer(hudComponent_t *comp)
{
	if (comp->style == STYLE_NORMAL)
	{
		CG_DrawRoundTimerNormal(comp);
	}
	else
	{
		CG_DrawRoundTimerSimple(comp);
	}
}

/**
 * @brief CG_DrawLocalTime
 * @param[in] y
 * @return
 */
static void CG_DrawLocalTime(hudComponent_t *comp)
{
	char *s;

	if (!(cg_drawTime.integer & LOCALTIME_ON))
	{
		return;
	}

	s = CG_LocalTimeText();

	CG_DrawCompText(comp, s, comp->color, ITEM_TEXTSTYLE_NORMAL);
}

/**
 * @brief Adds the current interpolate / extrapolate bar for this frame
 */
void CG_AddLagometerFrameInfo(void)
{
	lagometer.frameSamples[lagometer.frameCount & (LAG_SAMPLES - 1)] = cg.time - cg.latestSnapshotTime;
	lagometer.frameCount++;
}

/**
 * @brief Log the ping time, server framerate and number of dropped snapshots
 * before it each time a snapshot is received.
 * @param[in] snap
 */
void CG_AddLagometerSnapshotInfo(snapshot_t *snap)
{
	unsigned int index = lagometer.snapshotCount & (LAG_SAMPLES - 1);
	int          oldest;

	// dropped packet
	if (!snap)
	{
		lagometer.snapshotSamples[index] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	if (cg.demoPlayback)
	{
		static int lasttime = 0;

		snap->ping = (snap->serverTime - snap->ps.commandTime) - (1000 / cgs.sv_fps);

		// display snapshot time delta instead of ping
		lagometer.snapshotSamples[index] = snap->serverTime - lasttime;
		lasttime                         = snap->serverTime;
	}
	else
	{
		lagometer.snapshotSamples[index] = MAX(snap->ping - snap->ps.stats[STAT_ANTIWARP_DELAY], 0);
	}
	lagometer.snapshotAntiwarp[index] = snap->ping;  // TODO: check this for demoPlayback
	lagometer.snapshotFlags[index]    = snap->snapFlags;
	lagometer.snapshotCount++;

	// compute server framerate
	index = cgs.sampledStat.count;

	if (cgs.sampledStat.count < LAG_SAMPLES)
	{
		cgs.sampledStat.count++;
	}
	else
	{
		index -= 1;
	}

	cgs.sampledStat.samples[index].elapsed = snap->serverTime - cgs.sampledStat.lastSampleTime;
	cgs.sampledStat.samples[index].time    = snap->serverTime;

	if (cgs.sampledStat.samples[index].elapsed < 0)
	{
		cgs.sampledStat.samples[index].elapsed = 0;
	}

	cgs.sampledStat.lastSampleTime = snap->serverTime;

	cgs.sampledStat.samplesTotalElpased += cgs.sampledStat.samples[index].elapsed;

	oldest = snap->serverTime - PERIOD_SAMPLES;
	for (index = 0; index < cgs.sampledStat.count; index++)
	{
		if (cgs.sampledStat.samples[index].time > oldest)
		{
			break;
		}

		cgs.sampledStat.samplesTotalElpased -= cgs.sampledStat.samples[index].elapsed;
	}

	if (index)
	{
		memmove(cgs.sampledStat.samples, cgs.sampledStat.samples + index, sizeof(sample_t) * (cgs.sampledStat.count - index));
		cgs.sampledStat.count -= index;
	}

	cgs.sampledStat.avg = cgs.sampledStat.samplesTotalElpased > 0
	                      ? (int) (cgs.sampledStat.count / (cgs.sampledStat.samplesTotalElpased / 1000.0f) + 0.5f)
	                      : 0;
}

/**
 * @brief Draw disconnect icon for long lag
 * @param[in] y
 * @return
 */
static void CG_DrawDisconnect(hudComponent_t *comp)
{
	int        cmdNum;
	float      w, w2;
	usercmd_t  cmd;
	const char *s;

	// dont draw if a demo and we're running at a different timescale
	if (cg.demoPlayback && cg_timescale.value != 1.0f)
	{
		return;
	}

	// don't draw if the server is respawning
	if (cg.serverRespawning)
	{
		return;
	}

	// don't draw if intermission is about to start
	if (cg.intermissionStarted)
	{
		return;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if (cmd.serverTime <= cg.snap->ps.commandTime
	    || cmd.serverTime > cg.time)        // special check for map_restart
	{
		return;
	}

	// also add text in center of screen
	s = CG_TranslateString("Connection Interrupted");
	w = CG_Text_Width_Ext(s, comp->scale, 0, &cgs.media.limboFont2);
	CG_Text_Paint_Ext(Ccg_WideX(320) - w / 2, 100, comp->scale, comp->scale, comp->color, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

	// blink the icon
	if ((cg.time >> 9) & 1)
	{
		return;
	}

	// use same dimension as timer
	w  = CG_Text_Width_Ext("xx:xx:xx", 0.19f, 0, &cgs.media.limboFont1);
	w2 = (comp->location.w > w) ? comp->location.w : w;

	CG_DrawPic(comp->location.x, comp->location.y, w2 + 3, w2 + 3, cgs.media.disconnectIcon);
}

/**
 * @brief CG_DrawPing
 * @param[in] y
 * @return
 */
static void CG_DrawPing(hudComponent_t *comp)
{
	char *s;

	s = va("Ping %d", cg.snap->ping < 999 ? cg.snap->ping : 999);

	CG_DrawCompText(comp, s, comp->color, ITEM_TEXTSTYLE_NORMAL);
}

vec4_t colorAW = { 0, 0.5, 0, 0.5f };

/**
 * @brief CG_DrawLagometer
 * @param[in] y
 * @return
 */
static void CG_DrawLagometer(hudComponent_t *comp)
{
	int   a, i;
	float v, w, w2;
	float ax, ay, aw, ah, mid, range;
	int   color;
	float vscale;

	// use same dimension as timer
	w  = CG_Text_Width_Ext("xx:xx:xx", comp->scale, 0, &cgs.media.limboFont1);
	w2 = (comp->location.w > w) ? comp->location.w : w;

	// draw the graph
	trap_R_SetColor(NULL);
	if (comp->style == STYLE_NORMAL)
	{
		CG_FillRect(comp->location.x, comp->location.y, w2, comp->location.h, HUD_Background);
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, w2, comp->location.h, 1, HUD_Border);
	}

	ax = comp->location.x;
	ay = comp->location.y;
	aw = w2;
	ah = w2;
	CG_AdjustFrom640(&ax, &ay, &aw, &ah);

	color = -1;
	range = ah / 3;
	mid   = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for (a = 0 ; a < aw ; a++)
	{
		i  = (lagometer.frameCount - 1 - a) & (LAG_SAMPLES - 1);
		v  = lagometer.frameSamples[i];
		v *= vscale;
		if (v > 0)
		{
			if (color != 1)
			{
				color = 1;
				trap_R_SetColor(colorYellow);
			}
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if (v < 0)
		{
			if (color != 2)
			{
				color = 2;
				trap_R_SetColor(colorBlue);
			}
			v = -v;
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	// draw the snapshot latency / drop graph
	range  = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for (a = 0 ; a < aw ; a++)
	{
		i = (lagometer.snapshotCount - 1 - a) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if (v > 0)
		{
			// antiwarp indicator
			if (lagometer.snapshotAntiwarp[i] > 0)
			{
				w = lagometer.snapshotAntiwarp[i] * vscale;

				if (color != 6)
				{
					color = 6;
					trap_R_SetColor(colorAW);
				}

				if (w > range)
				{
					w = range;
				}
				trap_R_DrawStretchPic(ax + aw - a, ay + ah - w - 2, 1, w, 0, 0, 0, 0, cgs.media.whiteShader);
			}

			if (lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED)
			{
				if (color != 5)
				{
					color = 5;  // YELLOW for rate delay
					trap_R_SetColor(colorYellow);
				}
			}
			else
			{
				if (color != 3)
				{
					color = 3;
					trap_R_SetColor(colorGreen);
				}
			}
			v = v * vscale;
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if (v < 0)
		{
			if (color != 4)
			{
				color = 4;      // RED for dropped snapshots
				trap_R_SetColor(colorRed);
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	trap_R_SetColor(NULL);

	if (cg_nopredict.integer
#ifdef ALLOW_GSYNC
	    || cg_synchronousClients.integer
#endif // ALLOW_GSYNC
	    )
	{
		CG_Text_Paint_Ext(ax, ay, cg_fontScaleTP.value, cg_fontScaleTP.value, colorWhite, "snc", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	}

	// don't draw if a demo and we're running at a different timescale
	if (!cg.demoPlayback)
	{
		CG_DrawDisconnect(&activehud->disconnect);
	}

	// add snapshots/s in top-right corner of meter
	{
		char   *result;
		vec4_t *clr;

		if (cgs.sampledStat.avg < cgs.sv_fps * 0.5f)
		{
			clr = &colorRed;
		}
		else if (cgs.sampledStat.avg < cgs.sv_fps * 0.75f)
		{
			clr = &colorYellow;
		}
		else
		{
			clr = &comp->color;
		}

		// FIXME: see warmup blinky blinky
		//if (cgs.gamestate != GS_PLAYING)
		//{
		//	color[3] = fabs(sin(cg.time * 0.002));
		//}

		// FIXME: we might do different views x/Y or in %
		//result = va("%i/%i", cgs.sampledStat.avg, cgs.sv_fps);
		result = va("%i", cgs.sampledStat.avg);

		w  = CG_Text_Width_Ext(result, comp->scale, 0, &cgs.media.limboFont1);
		w2 = (comp->location.w > w) ? comp->location.w : w;

		CG_Text_Paint_Ext(comp->location.x + ((w2 - w) / 2), comp->location.y + 11, comp->scale, comp->scale, *clr, result, 0, 0, 0, &cgs.media.limboFont1);
	}
}

/**
 * @brief CG_Hud_Setup
 */
void CG_Hud_Setup(void)
{
	hudStucture_t hud0;

	// Hud0 aka the Default hud
	CG_setDefaultHudValues(&hud0);
	activehud = CG_addHudToList(&hud0);

	// Read the hud files
	CG_ReadHudScripts();
}

#ifdef ETLEGACY_DEBUG

/**
 * @brief CG_PrintHudComponent
 * @param[in] name
 * @param[in] comp
 */
static void CG_PrintHudComponent(const char *name, hudComponent_t *comp)
{
	Com_Printf("%s location: X %.f Y %.f W %.f H %.f "
	           "style %i visible: "
	           "%i scale : %f "
	           "color %.f %.f %.f %.f\n",
	           name, comp->location.x, comp->location.y, comp->location.w, comp->location.h,
	           comp->style, comp->visible,
	           comp->scale,
	           comp->color[0], comp->color[1], comp->color[2], comp->color[3]);
}

/**
 * @brief CG_PrintHud
 * @param[in] hud
 */
static void CG_PrintHud(hudStucture_t *hud)
{
	int i;

	for (i = 0; hudComponentFields[i].name; i++)
	{
		if (!hudComponentFields[i].isAlias)
		{
			CG_PrintHudComponent(hudComponentFields[i].name, (hudComponent_t *)((char * )hud + hudComponentFields[i].offset));
		}
	}
}
#endif

/**
 * @brief CG_SetHud
 */
void CG_SetHud(void)
{
	if (cg_altHud.integer && activehud->hudnumber != cg_altHud.integer)
	{
		activehud = CG_getHudByNumber(cg_altHud.integer);
		if (!activehud)
		{
			Com_Printf("^3WARNING hud with number %i is not available, defaulting to 0\n", cg_altHud.integer);
			activehud = CG_getHudByNumber(0);
			trap_Cvar_Set("cg_altHud", "0");
			return;
		}

#ifdef ETLEGACY_DEBUG
		CG_PrintHud(activehud);
#endif

		Com_Printf("Setting hud to: %i\n", cg_altHud.integer);
	}
	else if (!cg_altHud.integer && activehud->hudnumber)
	{
		activehud = CG_getHudByNumber(0);
	}
}

/**
 * @brief CG_DrawActiveHud
 */
void CG_DrawActiveHud(void)
{
	unsigned int   i;
	hudComponent_t *comp;

	for (i = 0; i < HUD_COMPONENTS_NUM; i++)
	{
		comp = activehud->components[i];

		if (comp && comp->visible && comp->draw)
		{
			comp->draw(comp);
		}
	}

	// Stats Debugging
	CG_DrawStatsDebug();
}

////////////////////////////////////////////
//
//
//  Hud editor
//
//
///////////////////////////////////////////

#define SOUNDEVENT(sound) trap_S_StartLocalSound(sound, CHAN_LOCAL_SOUND)

#define SOUND_SELECT    SOUNDEVENT(cgs.media.sndLimboSelect)
#define SOUND_FILTER    SOUNDEVENT(cgs.media.sndLimboFilter)

void CG_HUDSave_WriteComponent(fileHandle_t fh, int hudNumber, hudStucture_t *hud)
{
	int  j;
	char *s;

	s = va("\thud {\n\t\thudnumber %d\n", hudNumber);
	trap_FS_Write(s, strlen(s), fh);

	for (j = 0; hudComponentFields[j].name; j++)
	{
		if (!hudComponentFields[j].isAlias)
		{
			hudComponent_t *comp = (hudComponent_t *)((char *)hud + hudComponentFields[j].offset);
			s = va("\t\t%-16s\t"
			       "%-6.1f\t%-6.1f\t%-6.1f\t%-6.1f\t"
			       "%i\t%i\t"
			       "%-4.2f\t"
			       "%-4.2f\t%-4.2f\t%-4.2f\t%-4.2f\n",
			       hudComponentFields[j].name,
			       Ccg_Is43Screen() ? comp->location.x : comp->location.x / cgs.adr43, comp->location.y, comp->location.w, comp->location.h,
			       comp->style, comp->visible,
			       comp->scale,
			       comp->color[0], comp->color[1], comp->color[2], comp->color[3]);
			trap_FS_Write(s, strlen(s), fh);
		}
	}

	s = "\t}\n";
	trap_FS_Write(s, strlen(s), fh);
}

/**
 * @brief CG_HudSave
 * @param[in] HUDToDuplicate
 * @param[in] HUDToDelete
 */
qboolean CG_HudSave(int HUDToDuplicate, int HUDToDelete)
{
	int           i;
	fileHandle_t  fh;
	char          *s;
	hudStucture_t *hud;

	if (trap_FS_FOpenFile("hud.dat", &fh, FS_WRITE) < 0)
	{
		CG_Printf(S_COLOR_RED "ERROR CG_HudSave: failed to save hud to 'hud.dat\n");
		return qfalse;
	}

	if (HUDToDelete == 0)
	{
		CG_Printf(S_COLOR_RED "ERROR CG_HudSave: can't delete default HUD\n");
		return qfalse;
	}

	if (HUDToDuplicate && hudCount == MAXHUDS)
	{
		CG_Printf(S_COLOR_RED "ERROR CG_HudSave: no more free HUD slots for clone\n");
		return qfalse;
	}

	if (HUDToDuplicate >= 0)
	{
		int num = 1;

		// find a free number
		for (i = 1; i < MAXHUDS; i++)
		{
			hud = &hudlist[i];

			if (hud->hudnumber == num)
			{
				num++;
				i = 1;
			}
		}

		activehud         = CG_addHudToList(CG_getHudByNumber(HUDToDuplicate));
		cg_altHud.integer = activehud->hudnumber = num;

		CG_Printf("Clone hud %d on number %d\n", HUDToDuplicate, num);
	}

	s = "hudDef {\n";
	trap_FS_Write(s, strlen(s), fh);

	for (i = 1; i < hudCount; i++)
	{
		hud = &hudlist[i];

		if (hud->hudnumber == HUDToDelete)
		{
			int j;

			memmove(&hudlist[i], &hudlist[i + 1], sizeof(hudStucture_t) * (hudCount - i));
			i--;
			hudCount--;

			// FIXME: found a more elegant way for keeping sorting
			for (j = i; j < hudCount; j++)
			{
				CG_HudComponentsFill(&hudlist[j]);
			}

			// Back to default HUD
			cg_altHud.integer = 0;
			activehud         = CG_getHudByNumber(0);

			continue;
		}

		CG_HUDSave_WriteComponent(fh, hud->hudnumber, hud);
	}

	s = "}\n";
	trap_FS_Write(s, strlen(s), fh);

	trap_FS_FCloseFile(fh);

	CG_Printf("Saved huds to 'hud.dat'\n");

	return qtrue;
}

/**
* @brief CG_HudEditor_RenderEdit
* @param button
*/
static void CG_HudEditor_RenderEdit(panel_button_t *button)
{
	char  label[32];
	float offsetX, offsetY;

	// FIXME: get proper names and adjust alignment after
	Com_sprintf(label, sizeof(label), "%c:", button->text[strlen(button->text) - 1]);

	offsetX = CG_Text_Width_Ext(label, button->font->scalex, 0, button->font->font);
	offsetY = CG_Text_Height_Ext(label, button->font->scaley, 0, button->font->font);

	// FIXME(?): +6 because the drawing in BG_PanelButton_RenderEdit is quite misaligned for some reason
	CG_Text_Paint_Ext(button->rect.x - offsetX - 1, button->rect.y + (button->rect.h / 2) + (offsetY / 2) + 6, button->font->scalex, button->font->scaley, colorWhite, label, 0, 0, button->font->style, button->font->font);

	CG_DrawRect_FixedBorder(button->rect.x, button->rect.y + 6, button->rect.w, button->rect.h, 1, colorBlack);

	BG_PanelButton_RenderEdit(button);
}

/**
* @brief CG_HudEditorX_Finish
* @param button
*/
static void CG_HudEditorX_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->location.x = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorY_Finish
* @param button
*/
static void CG_HudEditorY_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->location.y = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorWidth_Finish
* @param button
*/
static void CG_HudEditorWidth_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->location.w = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorHeight_Finish
* @param button
*/
static void CG_HudEditorHeight_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->location.h = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorScale_Finish
* @param button
*/
static void CG_HudEditorScale_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->scale = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorColor_Finish colors
* @param button
*/
static void CG_HudEditorColor_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->color[button->data[3]] = Com_Clamp(0, 1.0f, Q_atof(buffer) / 255.0f);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorStyle_CheckboxKeyDown
* @param button
*/
qboolean CG_HudEditorVisible_CheckboxKeyDown(panel_button_t *button, int key)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);

	comp->visible = button->data[2] = !button->data[2];

	BG_PanelButtons_SetFocusButton(NULL);

	SOUND_FILTER;

	return qtrue;
}

/**
* @brief CG_HudEditorStyle_CheckboxKeyDown
* @param button
*/
qboolean CG_HudEditorStyle_CheckboxKeyDown(panel_button_t *button, int key)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);

	comp->style = button->data[2] = !button->data[2];

	BG_PanelButtons_SetFocusButton(NULL);

	SOUND_FILTER;

	return qtrue;
}

/**
* @brief CG_HudEditor_RenderCheckbox
* @param button
*/
static void CG_HudEditor_RenderCheckbox(panel_button_t *button)
{
	CG_DrawRect_FixedBorder(button->rect.x, button->rect.y, button->rect.w, button->rect.h, 2, colorBlack);

	if (button->data[2])
	{
		CG_DrawPic(button->rect.x + 2, button->rect.y + 2, 12, 12, cgs.media.readyShader);
	}
}

/**
* @brief CG_HudEditor_HudRenderDropdown
* @param[in] button
*/
static void CG_HudEditor_HudRenderDropdown(panel_button_t *button)
{
	CG_DropdownMainBox(button->rect.x, button->rect.y, button->rect.w, button->rect.h,
	                   button->font->scalex, button->font->scaley, colorBlack, va("%i", activehud->hudnumber), button == BG_PanelButtons_GetFocusButton(),
	                   button->font->colour, button->font->style, button->font->font);

	if (button == BG_PanelButtons_GetFocusButton())
	{
		float  y = button->rect.y;
		vec4_t colour;
		int    i;

		for (i = 0; i < hudCount; i++)
		{
			hudStucture_t *hud = &hudlist[i];

			if (hud->hudnumber == activehud->hudnumber)
			{
				continue;
			}

			y = CG_DropdownBox(button->rect.x, y, button->rect.w, button->rect.h,
			                   button->font->scalex, button->font->scaley, colorBlack, va("%i", hud->hudnumber), button == BG_PanelButtons_GetFocusButton(),
			                   button->font->colour, button->font->style, button->font->font);
		}

		VectorCopy(colorBlack, colour);
		colour[3] = 0.3f;
		CG_DrawRect(button->rect.x, button->rect.y + button->rect.h, button->rect.w, y - button->rect.y, 1.0f, colour);
	}
}

/**
* @brief CG_HudEditor_Dropdown_KeyDown
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_Dropdown_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		BG_PanelButtons_SetFocusButton(button);
		return qtrue;
	}

	return qfalse;
}

/**
* @brief CG_HudEditor_HudDropdown_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_HudDropdown_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (button == BG_PanelButtons_GetFocusButton())
		{
			rectDef_t rect;
			int       i;

			Com_Memcpy(&rect, &button->rect, sizeof(rect));

			for (i = 0; i < hudCount; i++)
			{
				hudStucture_t *hud = &hudlist[i];

				if (hud->hudnumber == activehud->hudnumber)
				{
					continue;
				}

				rect.y += button->rect.h;

				if (BG_CursorInRect(&rect))
				{
					cg_altHud.integer = hud->hudnumber;
					break;
				}
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief CG_HudEditorSave_KeyDown
 * @param button
 */
static qboolean CG_HudEditorSave_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		CG_HudSave(-1, -1);

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_HudEditorClone_KeyDown
 * @param button
 */
static qboolean CG_HudEditorClone_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		CG_HudSave(activehud->hudnumber, -1);

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_HudEditorDelete_KeyDown
 * @param button
 */
static qboolean  CG_HudEditorDelete_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		CG_HudSave(-1, activehud->hudnumber);

		return qtrue;
	}

	return qfalse;
}

/**
 * @brief CG_HudEditorRender_Button_Ext
 * @param[in] r
 * @param[in] text
 * @param[in] font
 */
void CG_HudEditorRender_Button_Ext(rectDef_t *r, const char *text, panel_button_text_t *font)
{
	vec4_t clrBdr = { 0.1f, 0.1f, 0.1f, 0.5f };
	vec4_t clrBck = { 0.3f, 0.3f, 0.3f, 0.4f };

	vec4_t clrBck_hi = { 0.5f, 0.5f, 0.5f, 0.4f };
	vec4_t clrTxt_hi = { 0.9f, 0.9f, 0.9f, 1.f };

	qboolean hilight = BG_CursorInRect(r);

	CG_FillRect(r->x, r->y, r->w, r->h, hilight ? clrBck_hi : clrBck);
	CG_DrawRect_FixedBorder(r->x, r->y, r->w, r->h, 1, clrBdr);

	if (text)
	{
		float w;

		w = CG_Text_Width_Ext(text, font->scalex, 0, font->font);

		CG_Text_Paint_Ext(r->x + ((r->w + 2) - w) * 0.5f, r->y + 11, font->scalex, font->scaley, hilight ? clrTxt_hi : font->colour, text, font->align, 0, font->style, font->font);
	}
}

/**
 * @brief CG_PanelButtonsRender_Button
 * @param[in] CG_HudEditorRender_Button
 */
void CG_HudEditorRender_Button(panel_button_t *button)
{
	CG_HudEditorRender_Button_Ext(&button->rect, button->text, button->font);
}

static panel_button_text_t hudEditorFont =
{
	0.20f,                   0.22f,
	{ 1.f,                   1.f,  1.f,0.8f },
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

static panel_button_text_t hudEditorFont_Dropdown =
{
    0.16f,                   0.18f,
    { 1.f,                   1.f,  1.f,0.8f },
    ITEM_TEXTSTYLE_SHADOWED, 0,
    &cgs.media.limboFont2,
};

// grouping hud editing fields
#define SCREEN_OFFSETX 240
#define SCREEN_OFFSETY 150
#define INPUT_WIDTH 40
#define INPUT_HEIGHT 20
#define INPUT_OFFSET_WIDTH (INPUT_WIDTH + 20)
#define CHECKBOX_SIZE 15
#define BUTTON_WIDTH (82 - 4)
#define BUTTON_HEIGHT (18 - 4)

static panel_button_t hudEditorHudDropdown =
{
	NULL,
	"hudeditor_huds",
	{ 0 + SCREEN_OFFSETX,          SCREEN_OFFSETY + 10,      40, 12 },
	{ 0,                           0,                        0,  0, 1, 0, 0, 0},
	&hudEditorFont_Dropdown,                // font
	CG_HudEditor_Dropdown_KeyDown, // keyDown
	CG_HudEditor_HudDropdown_KeyUp,// keyUp
	CG_HudEditor_HudRenderDropdown,
	NULL,
	0,
};


static void CG_HudEditor_CompRenderDropdown(panel_button_t *button);
static qboolean CG_HudEditor_CompDropdown_KeyUp(panel_button_t *button, int key);

static panel_button_t hudEditorCompDropdown =
{
	NULL,
	"hudeditor_comps",
	{ 0 + 100,           0,     120, 10 },
	{ 0,                            0,          0,   0, 1, 0, 0, 0},
	&hudEditorFont_Dropdown,    // font
	CG_HudEditor_Dropdown_KeyDown,  // keyDown
	CG_HudEditor_CompDropdown_KeyUp,// keyUp
	CG_HudEditor_CompRenderDropdown,
	NULL,
	0,
};

static panel_button_t hudEditorX =
{
	NULL,
	"hudeditor_x",
	{ 60 + SCREEN_OFFSETX,  0 + SCREEN_OFFSETY,          INPUT_WIDTH, INPUT_HEIGHT },
	// [0] used by ui_shared EditClick, [1] link to hud, [2] used by ui_shared EditClick [3] additional data like colorRGB, [4] differentiate between hud editor element and hud element
	{ 0,                    0,                           0,           0, 1, 0, 0, 0},
	&hudEditorFont,         // font
	BG_PanelButton_EditClick,// keyDown
	NULL,                   // keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorX_Finish,
	0
};

static panel_button_t hudEditorY =
{
	NULL,
	"hudeditor_y",
	{ 60 + INPUT_OFFSET_WIDTH + SCREEN_OFFSETX,0 + SCREEN_OFFSETY,                               INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                    0,                                                0,           0, 1, 0, 0, 0},
	&hudEditorFont,         // font
	BG_PanelButton_EditClick,// keyDown
	NULL,                   // keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorY_Finish,
	0
};

static panel_button_t hudEditorW =
{
	NULL,
	"hudeditor_w",
	{ 60 + SCREEN_OFFSETX,   SCREEN_OFFSETY + INPUT_HEIGHT + 4,          INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                          0,           0, 1, 0, 0, 0},
	&hudEditorFont,          // font
	BG_PanelButton_EditClick,// keyDown
	NULL,                    // keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorWidth_Finish,
	0
};

static panel_button_t hudEditorH =
{
	NULL,
	"hudeditor_h",
	{ 60 + INPUT_OFFSET_WIDTH + SCREEN_OFFSETX,SCREEN_OFFSETY + INPUT_HEIGHT + 4,                               INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                      0,                                                               0,           0, 1, 0, 0, 0},
	&hudEditorFont,           // font
	BG_PanelButton_EditClick, // keyDown
	NULL,                     // keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorHeight_Finish,
	0
};

static panel_button_t hudEditorScale =
{
	NULL,
	"hudeditor_s",
	{ 60 + (2 * INPUT_OFFSET_WIDTH) + SCREEN_OFFSETX,SCREEN_OFFSETY + INPUT_HEIGHT + 4,                                     INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                     0,           0, 1, 0, 0, 0},
	&hudEditorFont,          // font
	BG_PanelButton_EditClick,// keyDown
	NULL,                    // keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorScale_Finish,
	0
};

static panel_button_t hudEditorColorR =
{
	NULL,
	"hudeditor_colorR",
	{ 60 + SCREEN_OFFSETX,   SCREEN_OFFSETY + 2 * (INPUT_HEIGHT + 4),     INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                           0,           0, 1, 0, 0, 0},
	&hudEditorFont,          // font
	BG_PanelButton_EditClick,// keyDown
	NULL,                    // keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorColor_Finish,
	0
};

static panel_button_t hudEditorColorG =
{
	NULL,
	"hudeditor_colorG",
	{ 60 + INPUT_OFFSET_WIDTH + SCREEN_OFFSETX,SCREEN_OFFSETY + 2 * (INPUT_HEIGHT + 4),                          INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                0,           1, 1, 0, 0, 0},
	&hudEditorFont,          // font
	BG_PanelButton_EditClick,// keyDown
	NULL,                    // keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorColor_Finish,
	0
};

static panel_button_t hudEditorColorB =
{
	NULL,
	"hudeditor_colorB",
	{ 60 + (2 * INPUT_OFFSET_WIDTH) + SCREEN_OFFSETX,SCREEN_OFFSETY + 2 * (INPUT_HEIGHT + 4),                                INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                      0,           2, 1, 0, 0, 0},
	&hudEditorFont,          // font
	BG_PanelButton_EditClick,// keyDown
	NULL,                    // keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorColor_Finish,
	0
};

static panel_button_t hudEditorColorA =
{
	NULL,
	"hudeditor_colorA",
	{ 60 + (3 * INPUT_OFFSET_WIDTH) + SCREEN_OFFSETX,SCREEN_OFFSETY + 2 * (INPUT_HEIGHT + 4),                                INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                      0,           3, 1, 0, 0, 0},
	&hudEditorFont,          // font
	BG_PanelButton_EditClick,// keyDown
	NULL,                    // keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorColor_Finish,
	0
};

static panel_button_t hudEditorVisible =
{
	NULL,
	"hudeditor_visible",
	{ 60 + SCREEN_OFFSETX,      SCREEN_OFFSETY + 3 * (INPUT_HEIGHT + 6),    CHECKBOX_SIZE, CHECKBOX_SIZE },
	{ 0,                        0,                                          0,             0, 1, 0, 0, 0 },
	NULL,                       // font
	CG_HudEditorVisible_CheckboxKeyDown,// keyDown
	NULL,                       // keyUp
	CG_HudEditor_RenderCheckbox,
	NULL,
	0
};

static panel_button_t hudEditorStyle =
{
	NULL,
	"hudeditor_style",
	{ 60 + INPUT_OFFSET_WIDTH + SCREEN_OFFSETX,SCREEN_OFFSETY + 3 * (INPUT_HEIGHT + 6),                           CHECKBOX_SIZE, CHECKBOX_SIZE },
	{ 0,                        0,                                                                 0,             0, 1, 0, 0, 0 },
	NULL,                       // font
	CG_HudEditorStyle_CheckboxKeyDown,// keyDown
	NULL,                       // keyUp
	CG_HudEditor_RenderCheckbox,
	NULL,
	0
};

static panel_button_t hudEditorSave =
{
	NULL,
	"Save",
	{ 60 + SCREEN_OFFSETX,    SCREEN_OFFSETY + 4 * (INPUT_HEIGHT + 6),                 BUTTON_WIDTH, BUTTON_HEIGHT },
	{ 0,                      0,                                                       0,            0, 0, 0, 0, 0 },
	&hudEditorFont,           // font
	CG_HudEditorSave_KeyDown, // keyDown
	NULL,                     // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorClone =
{
	NULL,
	"Clone",
	{ 60 + BUTTON_WIDTH + 4 + SCREEN_OFFSETX,SCREEN_OFFSETY + 4 * (INPUT_HEIGHT + 6),                                   BUTTON_WIDTH, BUTTON_HEIGHT },
	{ 0,                      0,                                                                         0,            0, 0, 0, 0, 0 },
	&hudEditorFont,           // font
	CG_HudEditorClone_KeyDown,// keyDown
	NULL,                     // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorDelete =
{
	NULL,
	"Delete",
	{ 60 + (2 * (BUTTON_WIDTH + 4)) + SCREEN_OFFSETX,SCREEN_OFFSETY + 4 * (INPUT_HEIGHT + 6),                                          BUTTON_WIDTH, BUTTON_HEIGHT },
	{ 0,                      0,                                                                                0,            0, 0, 0, 0, 0 },
	&hudEditorFont,           // font
	CG_HudEditorDelete_KeyDown,// keyDown
	NULL,                     // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t *hudEditor[] =
{
	&hudEditorX,           &hudEditorY,
	&hudEditorW,           &hudEditorH,           &hudEditorScale,
	&hudEditorColorR,      &hudEditorColorG,      &hudEditorColorB, &hudEditorColorA,
	&hudEditorVisible,     &hudEditorStyle,
	&hudEditorHudDropdown, &hudEditorCompDropdown,
	&hudEditorSave,        &hudEditorClone,       &hudEditorDelete,
	NULL,
};

static panel_button_t *lastFocusButton;

/**
* @brief CG_HudEditorUpdateFields
* @param[in] button
*/
static void CG_HudEditorUpdateFields(panel_button_t *button)
{
	hudComponent_t *comp;
	char           buffer[256];

	comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[0]].offset);

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->location.x);
	trap_Cvar_Set("hudeditor_x", buffer);
	hudEditorX.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->location.y);
	trap_Cvar_Set("hudeditor_y", buffer);
	hudEditorY.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->location.w);
	trap_Cvar_Set("hudeditor_w", buffer);
	hudEditorW.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->location.h);
	trap_Cvar_Set("hudeditor_h", buffer);
	hudEditorH.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->scale);
	trap_Cvar_Set("hudeditor_s", buffer);
	hudEditorScale.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.2f", comp->color[0] * 255.0f);
	trap_Cvar_Set("hudeditor_colorR", buffer);
	hudEditorColorR.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.2f", comp->color[1] * 255.0f);
	trap_Cvar_Set("hudeditor_colorG", buffer);
	hudEditorColorG.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.2f", comp->color[2] * 255.0f);
	trap_Cvar_Set("hudeditor_colorB", buffer);
	hudEditorColorB.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.2f", comp->color[3] * 255.0f);
	trap_Cvar_Set("hudeditor_colorA", buffer);
	hudEditorColorA.data[1] = button->data[0];

	hudEditorVisible.data[1] = button->data[0];
	hudEditorVisible.data[2] = comp->visible;

	hudEditorStyle.data[1] = button->data[0];
	hudEditorStyle.data[2] = comp->style;
}

/**
* @brief CG_HudEditorRender draw borders for hud elements
* @param[in] button
*/
static void CG_HudEditor_Render(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[0]].offset);
	vec4_t         color;

	Vector4Copy(button == lastFocusButton ? colorGreen : colorRed, color);

	if (!comp->visible)
	{
		color[3] = 0.25f;
	}

	button->rect = comp->location;

	CG_DrawRect_FixedBorder(button->rect.x - 1, button->rect.y - 1, button->rect.w + 2, button->rect.h + 2, 2, color);
}

/**
* @brief CG_HudEditor_KeyDown
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		CG_HudEditorUpdateFields(button);
		BG_PanelButtons_SetFocusButton(button);
		button->data[4] = 0;
		return qtrue;
	}

	return qfalse;
}

/**
* @brief CG_HudEditor_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		lastFocusButton = button;
		BG_PanelButtons_SetFocusButton(NULL);
		button->data[4] = 1;
		return qtrue;
	}

	return qfalse;
}

panel_button_t *hudComponentsPanel[HUD_COMPONENTS_NUM + 1];
panel_button_t hudComponents[HUD_COMPONENTS_NUM];

/**
* @brief CG_HudEditor_CompRenderDropdown
* @param[in] button
*/
static void CG_HudEditor_CompRenderDropdown(panel_button_t *button)
{
	CG_DropdownMainBox(button->rect.x, button->rect.y, button->rect.w, button->rect.h,
	                   button->font->scalex, button->font->scaley, colorBlack, lastFocusButton ? lastFocusButton->text : "Select Comp", button == BG_PanelButtons_GetFocusButton(),
	                   button->font->colour, button->font->style, button->font->font);

	if (button == BG_PanelButtons_GetFocusButton())
	{
		float          y = button->rect.y;
		vec4_t         colour;
		panel_button_t **buttons = hudComponentsPanel;
		panel_button_t *parsedButton;

		for ( ; *buttons; buttons++)
		{
			parsedButton = (*buttons);

			if (parsedButton == lastFocusButton)
			{
				continue;
			}

			y = CG_DropdownBox(button->rect.x, y, button->rect.w, button->rect.h,
			                   button->font->scalex, button->font->scaley, colorBlack, parsedButton->text, button == BG_PanelButtons_GetFocusButton(),
			                   button->font->colour, button->font->style, button->font->font);
		}

		VectorCopy(colorBlack, colour);
		colour[3] = 0.3f;
		CG_DrawRect(button->rect.x, button->rect.y + button->rect.h, button->rect.w, y - button->rect.y, 1.0f, colour);
	}
}

/**
* @brief CG_HudEditor_CompDropdown_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_CompDropdown_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (button == BG_PanelButtons_GetFocusButton())
		{
			rectDef_t      rect;
			int            i;
			panel_button_t **buttons = hudComponentsPanel;
			panel_button_t *parsedButton;

			Com_Memcpy(&rect, &button->rect, sizeof(rect));

			for ( ; *buttons; buttons++)
			{
				parsedButton = (*buttons);

				if (parsedButton == lastFocusButton)
				{
					continue;
				}

				rect.y += button->rect.h;

				if (BG_CursorInRect(&rect))
				{
					lastFocusButton = parsedButton;
					break;
				}
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

/**
* @brief CG_HudEditorSetup
*/
void CG_HudEditorSetup(void)
{
	int i, j;

	for (i = 0, j = 0; hudComponentFields[i].name; i++, j++)
	{
		hudComponent_t *comp;

		if (hudComponentFields[i].isAlias)
		{
			j--;
			continue;
		}

		comp = (hudComponent_t *)((char *)activehud + hudComponentFields[i].offset);

		hudComponents[j].text      = hudComponentFields[i].name;
		hudComponents[j].rect      = comp->location;
		hudComponents[j].onKeyDown = CG_HudEditor_KeyDown;
		hudComponents[j].onKeyUp   = CG_HudEditor_KeyUp;
		hudComponents[j].onDraw    = CG_HudEditor_Render;
		hudComponents[j].data[0]   = i; // link button to hud component

		hudComponentsPanel[j] = &hudComponents[j];
	}

	// last element needs to be NULL
	hudComponentsPanel[HUD_COMPONENTS_NUM] = NULL;

	// clear last selected button
	lastFocusButton = NULL;
}

/**
 * @brief CG_DrawHudEditor_ToolTip
 * @param[in] name
 */
static void CG_DrawHudEditor_ToolTip(panel_button_t *button)
{
	int offsetX = CG_Text_Width_Ext(button->text, 0.20f, 0, &cgs.media.limboFont1);

	if (cgDC.cursorx + 10 + offsetX >= 640)
	{
		CG_Text_Paint_Ext(cgDC.cursorx - 10 - offsetX, cgDC.cursory, 0.20f, 0.22f, colorGreen, button->text, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
	else
	{
		CG_Text_Paint_Ext(cgDC.cursorx + 10, cgDC.cursory, 0.20f, 0.22f, colorGreen, button->text, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
}

/**
* @brief CG_DrawHudEditor
*/
void CG_DrawHudEditor(void)
{
	panel_button_t **buttons = hudComponentsPanel;
	panel_button_t *button;

	BG_PanelButtonsRender(hudComponentsPanel);
	BG_PanelButtonsRender(hudEditor);

	trap_R_SetColor(NULL);
	CG_DrawPic(cgDC.cursorx, cgDC.cursory, 32, 32, cgs.media.cursorIcon);

	// start parsing hud components from the last focused button
	if (lastFocusButton)
	{
		qboolean skip = qtrue;

		for ( ; *buttons; buttons++)
		{
			button = (*buttons);

			if (skip)
			{
				if (button != lastFocusButton)
				{
					continue;
				}

				skip = qfalse;
			}

			if (BG_CursorInRect(&button->rect))
			{
				CG_DrawHudEditor_ToolTip(button);
				return;
			}
		}

		// start for beginning
		buttons = hudComponentsPanel;
	}

	for ( ; *buttons; buttons++)
	{
		button = (*buttons);

		// early return
		if (lastFocusButton && lastFocusButton == button)
		{
			break;
		}

		if (BG_CursorInRect(&button->rect))
		{
			CG_DrawHudEditor_ToolTip(button);
			break;
		}
	}
}


/**
* @brief CG_HudEditor_KeyHandling
* @param[in] key
* @param[in] down
*/
void CG_HudEditor_KeyHandling(int key, qboolean down)
{
	if (BG_PanelButtonsKeyEvent(key, down, hudEditor))
	{
		return;
	}

	if (key == K_MOUSE2)
	{
		lastFocusButton = NULL;
		return;
	}

	// start parsing hud components from the last focused button
	if (lastFocusButton)
	{
		panel_button_t **buttons = hudComponentsPanel;
		panel_button_t *button;

		for ( ; *buttons; buttons++)
		{
			button = (*buttons);

			if (button == lastFocusButton)
			{
				if (BG_PanelButtonsKeyEvent(key, down, ++buttons))
				{
					return;
				}
				break;
			}
		}
	}

	if (BG_PanelButtonsKeyEvent(key, down, hudComponentsPanel))
	{
		return;
	}

	if (lastFocusButton && down)
	{
		hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[lastFocusButton->data[0]].offset);
		qboolean       changeSize;
		float          offset;
		float          *pValue;

		changeSize = (trap_Key_IsDown(K_RALT) || trap_Key_IsDown(K_LALT));

		if (trap_Key_IsDown(K_RCTRL) || trap_Key_IsDown(K_LCTRL))
		{
			offset = 0.1f;
		}
		else if (trap_Key_IsDown(K_RSHIFT) || trap_Key_IsDown(K_LSHIFT))
		{
			offset = 5;
		}
		else
		{
			offset = 1;
		}

		switch (key)
		{
		case K_LEFTARROW:  pValue = (changeSize ? &comp->location.w : &comp->location.x); *pValue -= offset ; break;
		case K_RIGHTARROW: pValue = (changeSize ? &comp->location.w : &comp->location.x); *pValue += offset ; break;
		case K_UPARROW:    pValue = (changeSize ? &comp->location.h : &comp->location.y); *pValue -= offset ; break;
		case K_DOWNARROW:  pValue = (changeSize ? &comp->location.h : &comp->location.y); *pValue += offset ; break;
		default: return;
		}

		CG_HudEditorUpdateFields(lastFocusButton);

		return;
	}
}

/**
* @brief CG_HudEditorMouseMove_Handling
* @param[in] x
* @param[in] y
*/
void CG_HudEditorMouseMove_Handling(int x, int y)
{
	if (!cg.editingHud)
	{
		return;
	}

	panel_button_t *button = BG_PanelButtons_GetFocusButton();
	static float   offsetX = 0;
	static float   offsetY = 0;

	if (button && !button->data[4] && BG_CursorInRect(&button->rect))
	{
		hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[0]].offset);

		if (!offsetX && !offsetY)
		{
			offsetX = (x - comp->location.x);
			offsetY = (y - comp->location.y);
		}

		comp->location.x = x - offsetX;
		comp->location.y = y - offsetY;
		CG_HudEditorUpdateFields(button);
	}
	else
	{
		offsetX = 0;
		offsetY = 0;
	}
}
