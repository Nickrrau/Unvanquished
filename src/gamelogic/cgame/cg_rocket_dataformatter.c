/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "cg_local.h"

static int GCD( int a, int b )
{
	int c;

	while ( b != 0 )
	{
		c = a % b;
		a = b;
		b = c;
	}

	return a;
}

static const char *DisplayAspectString( int w, int h )
{
	int gcd = GCD( w, h );

	w /= gcd;
	h /= gcd;

	// For some reason 8:5 is usually referred to as 16:10
	if ( w == 8 && h == 5 )
	{
		w = 16;
		h = 10;
	}

	return va( "%d:%d", w, h );
}

static void CG_Rocket_DFResolution( int handle, const char *data )
{
	int w = atoi( Info_ValueForKey(data, "1" ) );
	int h = atoi( Info_ValueForKey(data, "2" ) );
	char *aspectRatio = BG_strdup( DisplayAspectString( w, h ) );
	trap_Rocket_DataFormatterFormattedData( handle, va( "%dx%d ( %s )", w, h, aspectRatio ), qfalse );
	BG_Free( aspectRatio );
}

static void CG_Rocket_DFServerPing( int handle, const char *data )
{
	const char *str = Info_ValueForKey( data, "1" );
	trap_Rocket_DataFormatterFormattedData( handle, *str && Q_isnumeric( *str ) ? va( "%s ms", Info_ValueForKey( data, "1" ) ) : "", qfalse );
}

static void CG_Rocket_DFServerPlayers( int handle, const char *data )
{
	char max[ 4 ];
	Q_strncpyz( max, Info_ValueForKey( data, "3" ), sizeof( max ) );
	trap_Rocket_DataFormatterFormattedData( handle, va( "%s + (%s) / %s", Info_ValueForKey( data, "1" ), Info_ValueForKey( data, "2" ), max ), qtrue );
}

static void CG_Rocket_DFPlayerName( int handle, const char *data )
{
	trap_Rocket_DataFormatterFormattedData( handle, va("%s",  cgs.clientinfo[ atoi( Info_ValueForKey( data, "1" ) ) ].name ), qtrue );
}

static void CG_Rocket_DFUpgradeName( int handle, const char *data )
{
	trap_Rocket_DataFormatterFormattedData( handle, BG_Upgrade( atoi( Info_ValueForKey( data, "1" ) ) )->humanName, qtrue );
}

static void CG_Rocket_DFWeaponName( int handle, const char *data )
{
	trap_Rocket_DataFormatterFormattedData( handle, BG_Weapon( atoi( Info_ValueForKey( data, "1" ) ) )->humanName, qtrue );
}

static void CG_Rocket_DFClassName( int handle, const char *data )
{
	trap_Rocket_DataFormatterFormattedData( handle, BG_Class( atoi( Info_ValueForKey( data, "1" ) ) )->name, qtrue );
}

static void CG_Rocket_DFServerLabel( int handle, const char *data )
{
	const char *str = Info_ValueForKey( data, "1" );
	trap_Rocket_DataFormatterFormattedData( handle, *data ? ++str : "&nbsp;", qfalse );
}

static void CG_Rocket_DFCMArmouryBuyWeapon( int handle, const char *data )
{
	weapon_t weapon = (weapon_t) atoi( Info_ValueForKey( data, "1" ) );
	const char *Class = "";
	const char *Icon = "";
	const char *action = "";
	playerState_t *ps = &cg.snap->ps;
	int credits = ps->persistant[ PERS_CREDIT ];
	weapon_t currentweapon = BG_PrimaryWeapon( ps->stats );
	credits += BG_Weapon( currentweapon )->price;

	if ( !BG_WeaponUnlocked( weapon ) || BG_WeaponDisabled( weapon ) )
	{
		Class = "locked";
		Icon = "<icon>\uf023</icon>";
	}
	else if(BG_Weapon( weapon )->price > credits){

		Class = "expensive";
		Icon = "<icon>\uf0d6</icon>";
	}
	else if( BG_InventoryContainsWeapon( weapon, cg.predictedPlayerState.stats ) ){
		Class = "active";
		action =  va( "onClick='exec \"sell +%s\"'", BG_Weapon( weapon )->name );
		Icon = "<icon class=\"current\">\uf00c</icon><icon class=\"sell\">\uf0d6</icon>";
	}
	else
	{
		Class = "available";
		action =  va( "onClick='exec \"buy +%s\"'", BG_Weapon( weapon )->name );
	}

	trap_Rocket_DataFormatterFormattedData( handle, va( "<button class='armourybuy %s' onMouseover='setDS armouryBuyList weapons %s' %s>%s<img src='/%s'/></button>", Class, Info_ValueForKey( data, "2" ), action, Icon, CG_GetShaderNameFromHandle( cg_weapons[ weapon ].ammoIcon )), qfalse );
}

static void CG_Rocket_DFCMArmouryBuyUpgrade( int handle, const char *data )
{
	upgrade_t upgrade = (upgrade_t) atoi( Info_ValueForKey( data, "1" ) );
	const char *Class;
	qboolean disabled = qfalse;

	if ( !BG_UpgradeUnlocked( upgrade ) || BG_UpgradeDisabled( upgrade ) || BG_InventoryContainsUpgrade( upgrade, cg.predictedPlayerState.stats ) )
	{
		Class = "armourybuy disabled";
		disabled = qtrue;
	}
	else
	{
		Class = "armourybuy";
	}

	trap_Rocket_DataFormatterFormattedData( handle, va( "<button class='%s' onMouseover='setDS armouryBuyList upgrades %s' %s><img src='/%s'/></button>", Class, Info_ValueForKey( data, "2" ), disabled ? va( "onClick='exec \"sell %s'", BG_Upgrade( upgrade )->name ) : va( "onClick='exec \"buy +%s'", BG_Upgrade( upgrade )->name ), CG_GetShaderNameFromHandle( cg_upgrades[ upgrade ].upgradeIcon ) ), qfalse );
}

static void CG_Rocket_DFGWeaponDamage( int handle, const char *data )
{
	weapon_t weapon = (weapon_t) atoi( Info_ValueForKey( data, "1" ) );
	int      width = 0;

	switch( weapon )
	{
		case WP_HBUILD: width = 0; break;
		case WP_MACHINEGUN: width = 10; break;
		case WP_PAIN_SAW: width = 90; break;
		case WP_SHOTGUN: width = 40; break;
		case WP_LAS_GUN: width = 30; break;
		case WP_MASS_DRIVER: width = 50; break;
		case WP_CHAINGUN: width = 60; break;
		case WP_FLAMER: width = 70; break;
		case WP_PULSE_RIFLE: width = 70; break;
		case WP_LUCIFER_CANNON: width = 100; break;
		default: width = 0; break;
	}

	trap_Rocket_DataFormatterFormattedData( handle, va( "<div class=\"barValue\" style=\"width:%d%%;\"></div>", width ), qfalse );
}

static void CG_Rocket_DFGWeaponRateOfFire( int handle, const char *data )
{
	weapon_t weapon = (weapon_t) atoi( Info_ValueForKey( data, "1" ) );
	int      width = 0;

	switch( weapon )
	{
		case WP_HBUILD: width = 0; break;
		case WP_MACHINEGUN: width = 70; break;
		case WP_PAIN_SAW: width = 100; break;
		case WP_SHOTGUN: width = 100; break;
		case WP_LAS_GUN: width = 40; break;
		case WP_MASS_DRIVER: width = 20; break;
		case WP_CHAINGUN: width = 80; break;
		case WP_FLAMER: width = 70; break;
		case WP_PULSE_RIFLE: width = 70; break;
		case WP_LUCIFER_CANNON: width = 10; break;
		default: width = 0; break;
	}

	trap_Rocket_DataFormatterFormattedData( handle, va( "<div class=\"barValue\" style=\"width:%d%%;\"></div>", width ), qfalse );
}

static void CG_Rocket_DFGWeaponRange( int handle, const char *data )
{
	weapon_t weapon = (weapon_t) atoi( Info_ValueForKey( data, "1" ) );
	int      width = 0;

	switch( weapon )
	{
		case WP_HBUILD: width = 0; break;
		case WP_MACHINEGUN: width = 75; break;
		case WP_PAIN_SAW: width = 10; break;
		case WP_SHOTGUN: width = 30; break;
		case WP_LAS_GUN: width = 100; break;
		case WP_MASS_DRIVER: width = 100; break;
		case WP_CHAINGUN: width = 50; break;
		case WP_FLAMER: width = 25; break;
		case WP_PULSE_RIFLE: width = 80; break;
		case WP_LUCIFER_CANNON: width = 75; break;
		default: width = 0; break;
	}

	trap_Rocket_DataFormatterFormattedData( handle, va( "<div class=\"barValue\" style=\"width:%d%%;\"></div>", width ), qfalse );
}

static void CG_Rocket_DFLevelShot( int handle, const char *data )
{
	trap_Rocket_DataFormatterFormattedData( handle, va( "<img class=\"levelshot\" src=\"/levelshots/%s\"/>", Info_ValueForKey( data, "1" ) ), qfalse );
}

static void CG_Rocket_DFGearOrReady( int handle, const char *data )
{
	int clientNum = atoi( Info_ValueForKey( data, "1" ) );
	if ( cg.intermissionStarted )
	{
		if ( CG_ClientIsReady( clientNum ) )
		{
			trap_Rocket_DataFormatterFormattedData( handle, "[check]", qtrue );
		}
		else
		{
			trap_Rocket_DataFormatterFormattedData( handle, "", qfalse );
		}
	}
	else
	{
		score_t *s = &cg.scores[ clientNum ];
		const char *rml = "";

		if ( s->team == cg.predictedPlayerState.persistant[ PERS_TEAM ] && s->weapon != WP_NONE )
		{
			rml = va( "<img src='/%s'/>", CG_GetShaderNameFromHandle( cg_weapons[ s->weapon ].weaponIcon ) );
		}

		if ( s->team == cg.predictedPlayerState.persistant[ PERS_TEAM ] && s->team == TEAM_HUMANS && s->upgrade != UP_NONE )
		{
			rml = va( "%s<img src='/%s'/>", rml, CG_GetShaderNameFromHandle( cg_upgrades[ s->upgrade ].upgradeIcon ) );
		}

		trap_Rocket_DataFormatterFormattedData( handle, rml, qfalse );
	}
}

typedef struct
{
	const char *name;
	void ( *exec ) ( int handle, const char *data );
} dataFormatterCmd_t;

static const dataFormatterCmd_t dataFormatterCmdList[] =
{
	{ "ClassName", &CG_Rocket_DFClassName },
	{ "CMArmouryBuyUpgrades", &CG_Rocket_DFCMArmouryBuyUpgrade },
	{ "CMArmouryBuyWeapons", &CG_Rocket_DFCMArmouryBuyWeapon },
	{ "GearOrReady", &CG_Rocket_DFGearOrReady },
	{ "GWeaponDamage", &CG_Rocket_DFGWeaponDamage },
	{ "GWeaponRange", &CG_Rocket_DFGWeaponRange },
	{ "GWeaponRateOfFire", &CG_Rocket_DFGWeaponRateOfFire },
	{ "LevelShot", &CG_Rocket_DFLevelShot },
	{ "PlayerName", &CG_Rocket_DFPlayerName },
	{ "Resolution", &CG_Rocket_DFResolution },
	{ "ServerLabel", &CG_Rocket_DFServerLabel },
	{ "ServerPing", &CG_Rocket_DFServerPing },
	{ "ServerPlayers", &CG_Rocket_DFServerPlayers },
	{ "UpgradeName", &CG_Rocket_DFUpgradeName },
	{ "WeaponName", &CG_Rocket_DFWeaponName },
};

static const size_t dataFormatterCmdListCount = ARRAY_LEN( dataFormatterCmdList );

static int dataFormatterCmdCmp( const void *a, const void *b )
{
	return Q_stricmp( ( const char * ) a, ( ( dataFormatterCmd_t * ) b )->name );
}

void CG_Rocket_FormatData( int handle )
{
	static char name[ 200 ], data[ BIG_INFO_STRING ];
	dataFormatterCmd_t *cmd;

	trap_Rocket_DataFormatterRawData( handle, name, sizeof( name ), data, sizeof( data ) );

	cmd = (dataFormatterCmd_t*) bsearch( name, dataFormatterCmdList, dataFormatterCmdListCount, sizeof( dataFormatterCmd_t ), dataFormatterCmdCmp );

	if ( cmd )
	{
		cmd->exec( handle, data );
	}
}

void CG_Rocket_RegisterDataFormatters( void )
{
	int i;

	for ( i = 0; i < dataFormatterCmdListCount; i++ )
	{
		// Check that the commands are in increasing order so that it can be used by bsearch
		if ( i != 0 && Q_stricmp( dataFormatterCmdList[ i - 1 ].name, dataFormatterCmdList[ i ].name ) > 0 )
		{
			CG_Printf( "CGame Rocket dataFormatterCmdList is in the wrong order for %s and %s\n", dataFormatterCmdList[i - 1].name, dataFormatterCmdList[ i ].name );
		}

		trap_Rocket_RegisterDataFormatter( dataFormatterCmdList[ i ].name );
	}
}