#include <Windows.h>
#include <string>
#include "verinfo.h"

struct JassStringData
{
	DWORD vtable;
	DWORD refCount;
	DWORD dwUnk1;
	DWORD pUnk2;
	DWORD pUnk3;
	DWORD pUnk4;
	DWORD pUnk5;
	char *data;
};


struct CJassString
{
	DWORD vtable;
	DWORD dw0;
	JassStringData *data;
	DWORD dw1;
};

CJassString * ModeCM;
CJassString * Data;

char * GetStringFromJassString( CJassString * straddr )
{
	if ( straddr->data )
		return straddr->data->data;
	return 0;
}

int GetJassStringOffset = 0;

CJassString * GetJassString( char *szString, CJassString *String )
{
	int Address = GetJassStringOffset;

	__asm
	{
		PUSH szString;
		MOV ECX, String;
		CALL Address;
	}

	return String;
}

typedef void( __cdecl * pStoreInteger )( UINT cache, CJassString *missionKey, CJassString *key, int value );
pStoreInteger StoreInteger_org;

typedef void( __cdecl * pSyncStoredInteger )( UINT cache, CJassString *missionKey, CJassString *key );
pSyncStoredInteger SyncStoredInteger_org;
UINT retval;

void __cdecl StoreInteger_my( UINT cache, CJassString *missionKey, CJassString *key, int value )
{
	StoreInteger_org( cache, missionKey, key, value );
	StoreInteger_org( cache, Data, ModeCM, 0 );
}



typedef UINT( __cdecl * pInitGameCache )( CJassString * campaignFile );
pInitGameCache pInitGameCache_org;

UINT __cdecl InitGameCache_my( CJassString * campaignFile )
{
	retval = pInitGameCache_org( campaignFile );
	ModeCM = GetJassString( "Modecm", new CJassString( ) );
	Data = GetJassString( "Data", new CJassString( ) );

	for ( int i = 0; i < 15; i++ )
	{
		StoreInteger_org( retval, Data, ModeCM, i );
		SyncStoredInteger_org( retval, Data, ModeCM );
	}

	return 0;
}





int GameDll = 0;
int pJassEnvAddress = 0;


int CreateJassNativeHook( int oldaddress, int newaddress )
{
	int FirstAddress = *( int * ) pJassEnvAddress;
	if ( FirstAddress )
	{
		FirstAddress = *( int * ) ( FirstAddress + 20 );
		if ( FirstAddress )
		{

			FirstAddress = *( int * ) ( FirstAddress + 32 );
			if ( FirstAddress )
			{

				int NextAddress = FirstAddress;

				while ( TRUE )
				{
					if ( *( int * ) ( NextAddress + 12 ) == oldaddress )
					{
						*( int * ) ( NextAddress + 12 ) = newaddress;

						return NextAddress + 12;
					}

					NextAddress = *( int* ) NextAddress;

					if ( NextAddress == FirstAddress || NextAddress <= 0 )
						break;
				}
			}
		}

	}
	return 0;
}
HANDLE JustWatchForJassEnvHNDL;



DWORD WINAPI JustWatchForJassEnv( LPVOID )
{
	while ( true )
	{
		CreateJassNativeHook( ( int ) pInitGameCache_org, ( int ) &InitGameCache_my );

	//	CreateJassNativeHook( ( int ) StoreInteger_org, ( int ) &StoreInteger_my );
		Sleep( 1000 );
	}

	return 0;
}



void Init126aVer( )
{
	pJassEnvAddress = GameDll + 0xADA848;
	CreateThread( 0, 0, JustWatchForJassEnv, 0, 0, 0 );

	GetJassStringOffset = GameDll + 0x011300;
	StoreInteger_org = ( pStoreInteger ) ( GameDll + 0x3CA0A0 );
	SyncStoredInteger_org = ( pSyncStoredInteger ) ( GameDll + 0x3CA6E0 );
	pInitGameCache_org = ( pInitGameCache ) ( GameDll + 0x3D2CC0 );
}


void Init127aVer( )
{
	pJassEnvAddress = GameDll + 0xBE3740;
	CreateThread( 0, 0, JustWatchForJassEnv, 0, 0, 0 );
	GetJassStringOffset = GameDll + 0x51310;
	StoreInteger_org = ( pStoreInteger ) ( GameDll + 0x1F8280 );
	SyncStoredInteger_org = ( pSyncStoredInteger ) ( GameDll + 0x1F8940 );
	pInitGameCache_org = ( pInitGameCache ) ( GameDll + 0x1E73C0 );
}




void InitializeLocalPlayerHacker( )
{
	HMODULE hGameDll = GetModuleHandle( "Game.dll" );
	if ( !hGameDll )
	{
		MessageBox( 0, "LocalPlayerHacker problem!\nNo game.dll found.", "Game.dll not found", 0 );
		return;
	}

	GameDll = ( int ) hGameDll;

	CFileVersionInfo gdllver;
	gdllver.Open( hGameDll );
	// Game.dll version (1.XX)
	int GameDllVer = gdllver.GetFileVersionQFE( );
	gdllver.Close( );

	if ( GameDllVer == 6401 )
	{
		Init126aVer( );
	}
	else if ( GameDllVer == 52240 )
	{
		Init127aVer( );
	}
	else
	{
		MessageBox( 0, "LocalPlayerHacker problem!\nGame version not supported.", "\nGame version not supported", 0 );
		return;
	}



	// 6401 - 126
	// 52240 - 127

}



BOOL __stdcall DllMain( HINSTANCE i, DWORD r, LPVOID )
{
	if ( r == DLL_PROCESS_ATTACH )
	{
		InitializeLocalPlayerHacker( );
	}
	else if ( r == DLL_PROCESS_DETACH )
	{
		TerminateThread( JustWatchForJassEnvHNDL, 0 );
	}

	return TRUE;
}