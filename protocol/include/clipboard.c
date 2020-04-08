#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "clipboard.h"
#include "fractal.h"

#ifdef _WIN32
WCHAR* lget_clipboard_directory();
WCHAR* lset_clipboard_directory();
#endif
char* get_clipboard_directory()
{
	static char buf[MAX_PATH];
	wcstombs( buf, lget_clipboard_directory(), sizeof( buf ) );
	return buf;
}
char* set_clipboard_directory()
{
	static char buf[MAX_PATH];
	wcstombs( buf, lset_clipboard_directory(), sizeof( buf ) );
	return buf;
}

#ifdef _WIN32
#define LGET_CLIPBOARD (lget_clipboard_directory())
#define GET_CLIPBOARD (get_clipboard_directory())
#else
#define GET_CLIPBOARD "./get_clipboard"
#endif

#ifdef _WIN32
#define LSET_CLIPBOARD (lset_clipboard_directory())
#define SET_CLIPBOARD (set_clipboard_directory())
#else
#define SET_CLIPBOARD "./set_clipboard"
#endif

void initClipboard()
{
	get_clipboard_directory();
	set_clipboard_directory();
}

#ifdef _WIN32
#include "shlwapi.h"
#pragma comment (lib, "Shlwapi.lib")
#include "Shellapi.h"
#include "shlobj_core.h"
#include "Knownfolders.h"

WCHAR* lclipboard_directory()
{
	static WCHAR* directory = NULL;
	if( directory == NULL )
	{
		static WCHAR szPath[MAX_PATH];
		static WCHAR* path;
		if( SUCCEEDED( SHGetKnownFolderPath( &FOLDERID_RoamingAppData,
											 CSIDL_APPDATA | CSIDL_FLAG_CREATE,
											 0,
											 &path ) ) )
		{
			wcscpy( szPath, path );
			CoTaskMemFree( path );
			PathAppendW( szPath, L"FractalCache" );
			if( !PathFileExistsW( szPath ) )
			{
				if( !CreateDirectoryW( szPath, NULL ) )
				{
					mprintf( "Could not create directory: %S (Error %d)\n", szPath, GetLastError() );
					return NULL;
				}
			}
		} else
		{
			mprintf( "Could not SHGetKnownFolderPath" );
			return NULL;
		}
		directory = szPath;
	}
	mprintf( "Directory: %S\n", directory );
	return directory;
}

WCHAR* lget_clipboard_directory()
{
	WCHAR path[MAX_PATH];
	WCHAR* cb_dir = lclipboard_directory();
	wcscpy( path, cb_dir );
	PathAppendW( path, L"get_clipboard" );
	if( !PathFileExistsW( path ) )
	{
		if( !CreateDirectoryW( path, NULL ) )
		{
			mprintf( "Could not create directory: %S (Error %d)\n", path, GetLastError() );
			return NULL;
		}
	}
	return path;
}

WCHAR* lset_clipboard_directory()
{
	WCHAR path[MAX_PATH];
	WCHAR* cb_dir = lclipboard_directory();
	wcscpy( path, cb_dir );
	PathAppendW( path, L"set_clipboard" );
	if( !PathFileExistsW( path ) )
	{
		if( !CreateDirectoryW( path, NULL ) )
		{
			mprintf( "Could not create directory: %S (Error %d)\n", path, GetLastError() );
			return NULL;
		}
	}
	return path;
}

#define REPARSE_MOUNTPOINT_HEADER_SIZE 8

typedef struct
{
	DWORD ReparseTag;
	DWORD ReparseDataLength;
	WORD Reserved;
	WORD ReparseTargetLength;
	WORD ReparseTargetMaximumLength;
	WORD Reserved1;
	WCHAR ReparseTarget[1];
} REPARSE_MOUNTPOINT_DATA_BUFFER, * PREPARSE_MOUNTPOINT_DATA_BUFFER;

#include <winioctl.h>

bool CreateJunction( WCHAR* szJunction, WCHAR* szPath );

bool CreateJunction( WCHAR* szJunction, WCHAR* szPath )
{
	BYTE buf[sizeof( REPARSE_MOUNTPOINT_DATA_BUFFER ) + MAX_PATH * sizeof( WCHAR )];
	REPARSE_MOUNTPOINT_DATA_BUFFER* ReparseBuffer = (REPARSE_MOUNTPOINT_DATA_BUFFER*) buf;
	WCHAR szTarget[MAX_PATH] = L"\\??\\";

	wcscat( szTarget, szPath );
	wcscat( szTarget, L"\\" );

	if( !CreateDirectoryW( szJunction, NULL ) )
	{
		mprintf("CreateDirectoryW Error: %d\n", GetLastError());
		return false;
	}

	// Obtain SE_RESTORE_NAME privilege (required for opening a directory)
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES tp;
	if( !OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken ) )
	{
		mprintf( "OpenProcessToken Error: %d\n", GetLastError() );
		return false;
	}
	if( !LookupPrivilegeValueW( NULL, L"SeRestorePrivilege", &tp.Privileges[0].Luid ) )
	{
		mprintf( "LookupPrivilegeValueW Error: %d\n", GetLastError() );
		return false;
	}
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if( !AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof( TOKEN_PRIVILEGES ), NULL, NULL ) )
	{
		mprintf( "AdjustTokenPrivileges Error: %d\n", GetLastError() );
		return false;
	}
	if( hToken ) CloseHandle( hToken );
	// End Obtain SE_RESTORE_NAME privilege

	HANDLE hDir = CreateFileW( szJunction, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL );
	if( hDir == INVALID_HANDLE_VALUE )
	{
		mprintf( "CreateFileW Error: %d\n", GetLastError() );
		return false;
	}

	memset( buf, 0, sizeof( buf ) );
	ReparseBuffer->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
	int len = (int) wcslen( szTarget );
	wcscpy( ReparseBuffer->ReparseTarget, szTarget );
	ReparseBuffer->ReparseTargetMaximumLength = (WORD) len * sizeof( WCHAR );
	ReparseBuffer->ReparseTargetLength = (WORD) (len - 1) * sizeof( WCHAR );
	ReparseBuffer->ReparseDataLength = ReparseBuffer->ReparseTargetLength + 12;

	DWORD dwRet;
	if( !DeviceIoControl( hDir, FSCTL_SET_REPARSE_POINT, ReparseBuffer, ReparseBuffer->ReparseDataLength+REPARSE_MOUNTPOINT_HEADER_SIZE, NULL, 0, &dwRet, NULL ) )
	{
		CloseHandle( hDir );
		RemoveDirectoryW( szJunction );

		mprintf( "DeviceIoControl Error: %d\n", GetLastError() );
		return false;
	}

	CloseHandle( hDir );

	return true;
}

#endif

#if defined (_WIN32)
	static int last_clipboard_sequence_number = -1;
#elif __APPLE__
	#include "clipboard_osx.h"
	#include "mac_utils.h"
	#include <unistd.h>
	#include <sys/syslimits.h>

	bool clipboardHasImage;
	bool clipboardHasString;
	bool clipboardHasFiles;
	static int last_clipboard_sequence_number = -1;
#endif

static char clipboard_buf[9000000];

void StartTrackingClipboardUpdates()
{
#if defined(_WIN32)
	last_clipboard_sequence_number = GetClipboardSequenceNumber();
#elif __APPLE__
	last_clipboard_sequence_number = -1; // to capture the first event
	clipboardHasImage = false;
	clipboardHasString = false;
	clipboardHasFiles = false;
#else
// TODO: LINUX UBUNTU/DEBIAN
#endif
}

bool hasClipboardUpdated()
{
	bool hasUpdated = false;

#if defined(_WIN32)
	int new_clipboard_sequence_number = GetClipboardSequenceNumber();
	if( new_clipboard_sequence_number > last_clipboard_sequence_number )
	{
		hasUpdated = true;
		last_clipboard_sequence_number = new_clipboard_sequence_number;
	}
#elif __APPLE__
	int new_clipboard_sequence_number = GetClipboardChangecount();
	if (new_clipboard_sequence_number > last_clipboard_sequence_number) {
		// check if new clipboard is an image or a string
		clipboardHasImage = ClipboardHasImage();
		clipboardHasString = ClipboardHasString();
		clipboardHasFiles = ClipboardHasFiles();
		hasUpdated = (clipboardHasImage || clipboardHasString || clipboardHasFiles); // should be always set to true in here
		last_clipboard_sequence_number = new_clipboard_sequence_number;
	}
#else
// TODO: LINUX UBUNTU/DEBIAN
#endif
	return hasUpdated;
}

ClipboardData* GetClipboard()
{
	ClipboardData* cb = (ClipboardData *) clipboard_buf;

	cb->size = 0;
	cb->type = CLIPBOARD_NONE;

#if defined(_WIN32)
	if( !OpenClipboard( NULL ) )
	{
		return cb;
	}

	int cf_types[] = {
		CF_TEXT,
		CF_DIB,
		CF_HDROP,
	};

	int cf_type = -1;

	for( int i = 0; i < sizeof( cf_types ) / sizeof( cf_types[0] ) && cf_type == -1; i++ )
	{
		if( IsClipboardFormatAvailable( cf_types[i] ) )
		{
			HGLOBAL hglb = GetClipboardData( cf_types[i] );
			if( hglb != NULL )
			{
				LPTSTR lptstr = GlobalLock( hglb );
				if( lptstr != NULL )
				{
					int data_size = (int)GlobalSize( hglb );
					if( data_size < sizeof( clipboard_buf ) )
					{
						cb->size = data_size;
						memcpy( cb->data, lptstr, data_size );
						cf_type = cf_types[i];
					} else
					{
						mprintf( "Could not copy, clipboard too large! %d bytes\n", data_size );
					}

					// Don't forget to release the lock after you are done.
					GlobalUnlock( hglb );
				} else
				{
					mprintf( "GlobalLock failed! (Type: %d) (Error: %d)\n", cf_types[i], GetLastError() );
				}
			}
		}
	}

	if( cf_type == -1 )
	{
		mprintf( "Clipboard not found\n" );
	} else
	{
		switch( cf_type )
		{
		case CF_TEXT:
			// Read the contents of lptstr which just a pointer to the string.
			//mprintf( "CLIPBOARD STRING: %s\n", cb->data );
			cb->type = CLIPBOARD_TEXT;
			break;
		case CF_DIB:
			//mprintf( "Clipboard bitmap received! Size: %d\n", cb->size );
			cb->type = CLIPBOARD_IMAGE;
			break;
		case CF_HDROP:
			mprintf( "Hdrop! Size: %d\n", cb->size );
			DROPFILES drop;
			memcpy( &drop, cb->data, sizeof( DROPFILES ) );

			mprintf( "Drop pFiles: %d\n", drop.pFiles );

			// Begin copy to clipboard
			WCHAR* filename = (WCHAR*)(cb->data + sizeof( drop ));

			SHFILEOPSTRUCTW sh = { 0 };
			sh.wFunc = FO_DELETE;
			sh.fFlags = 0;
			sh.fFlags |= FOF_SILENT;
			sh.fFlags |= FOF_NOCONFIRMMKDIR;
			sh.fFlags |= FOF_NOCONFIRMATION;
			sh.fFlags |= FOF_WANTMAPPINGHANDLE;
			sh.fFlags |= FOF_NOERRORUI;
			sh.pFrom = LGET_CLIPBOARD;

			clock t;
			StartTimer( &t );
			//SHFileOperationW( &sh );

			WIN32_FIND_DATAW data;

			WCHAR first_file_path[MAX_PATH] = L"";
			wcscat( first_file_path, LGET_CLIPBOARD );
			wcscat( first_file_path, L"\\*" );

			HANDLE hFind = FindFirstFileW( first_file_path, &data );
			if( hFind != INVALID_HANDLE_VALUE )
			{
				WCHAR* ignore1 = L".";
				WCHAR* ignore2 = L"..";

				do
				{
					if( wcscmp( data.cFileName, ignore1 ) == 0 || wcscmp( data.cFileName, ignore2 ) == 0 )
					{
						continue;
					}

					WCHAR filename[MAX_PATH] = L"";
					wcscat( (wchar_t*)filename, LGET_CLIPBOARD );
					wcscat((wchar_t *) filename, L"\\" );
					wcscat((wchar_t *) filename, data.cFileName );

					mprintf( "Deleting %S...\n", filename );

					DWORD fileattributes = GetFileAttributesW((LPCWSTR) filename );
					if( fileattributes == INVALID_FILE_ATTRIBUTES )
					{
						mprintf( "GetFileAttributesW Error: %d\n", GetLastError() );
					}

					if( fileattributes & FILE_ATTRIBUTE_DIRECTORY )
					{
						if( !RemoveDirectoryW( (LPCWSTR)filename ) )
						{
							mprintf( "Delete Folder Error: %d\n", GetLastError() );
						}
					} else
					{
						if (!DeleteFileW((LPCWSTR) filename ))
						{
							mprintf( "Delete Folder Error: %d\n", GetLastError() );
						}
					}
				} while( FindNextFileW( hFind, &data ) );
				FindClose( hFind );
			}

			RemoveDirectoryW( LGET_CLIPBOARD );
			if( !CreateDirectoryW( LGET_CLIPBOARD, NULL ) )
			{
				mprintf( "Could not create directory: %S (Error %d)\n", LGET_CLIPBOARD, GetLastError() );
			}

			// Go through filenames
			while( *filename != L'\0' )
			{
				WCHAR* fileending = PathFindFileNameW( filename );
				DWORD fileattributes = GetFileAttributesW( filename );

				WCHAR target_file[MAX_PATH] = L"";
				wcscat( target_file, LGET_CLIPBOARD );
				wcscat( target_file, L"\\" );
				wcsncat( target_file, fileending, sizeof(target_file) );

				mprintf( "Target: %S\n", target_file );
				mprintf( "Src: %S\n", filename );

				if( fileattributes & FILE_ATTRIBUTE_DIRECTORY )
				{
					if( !CreateJunction( target_file, filename ) )
					{
						mprintf( "CreateJunction Error: %d\n", GetLastError() );
					}
				} else
				{
					if( !CreateHardLinkW( target_file, filename, 0 ) )
					{
						mprintf( "CreateHardLinkW Error: %d\n", GetLastError() );
					}
				}

				mprintf( "TARGET FILENAME: %S\n", target_file );
				mprintf( "FILENAME: %S\n", filename );
				mprintf( "FILENAME ENDING: %S\n", fileending );

				filename += wcslen( filename ) + 1;
			}
			mprintf( "Time: %f\n", GetTimer( t ) );

			cb->type = CLIPBOARD_FILES;
			cb->size = 0;

			break;
		default:
			mprintf( "Clipboard type unknown: %d\n", cf_type );
			cb->type = CLIPBOARD_NONE;
			break;
		}
	}

	CloseClipboard();
#elif __APPLE__
// do clipboardHasFiles check first because it is more restrictive
// otherwise gets multiple files confused with string and thinks both are true
	if( clipboardHasFiles )
	{
		mprintf( "Getting files from clipboard\n" );

		// allocate memory for filenames and paths
		OSXFilenames* filenames[MAX_URLS];
		for( size_t i = 0; i < MAX_URLS; i++ )
		{
			filenames[i] = (OSXFilenames*)malloc( sizeof( OSXFilenames ) );
			filenames[i]->filename = (char*)malloc( PATH_MAX * sizeof( char ) );
			filenames[i]->fullPath = (char*)malloc( PATH_MAX * sizeof( char ) );

			// pad with null terminators
			memset( filenames[i]->filename, '\0', PATH_MAX * sizeof( char ) );
			memset( filenames[i]->fullPath, '\0', PATH_MAX * sizeof( char ) );
		}

		// populate filenames array with file URLs
		ClipboardGetFiles( filenames );

		// set clipboard data attributes to be returned
		cb->type = CLIPBOARD_FILES;
		cb->size = 0;

		// delete clipboard directory and all its files
		if( dir_exists( GET_CLIPBOARD ) > 0 )
		{
			mac_rm_rf( GET_CLIPBOARD );
		}

		// make new clipboard directory
		mkdir( GET_CLIPBOARD, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );


		// make symlinks for all files in clipboard and store in directory
		for( size_t i = 0; i < MAX_URLS; i++ )
		{
			// if there are no more file URLs in clipboard, exit loop
			if( *filenames[i]->fullPath != '\0' )
			{
				char symlinkName[PATH_MAX] = "";
				strcat( symlinkName, GET_CLIPBOARD );
				strcat( symlinkName, "/" );
				strcat( symlinkName, filenames[i]->filename );
				symlink( filenames[i]->fullPath, symlinkName );
			} else
			{
				break;
			}
		}

		// free heap memory
		for( size_t i = 0; i < MAX_URLS; i++ )
		{
			free( filenames[i]->filename );
			free( filenames[i]->fullPath );
			free( filenames[i] );
		}

	} else if( clipboardHasString )
	{
		// get the string
		const char* clipboard_string = ClipboardGetString();
		int data_size = strlen(clipboard_string) + 1; // for null terminator
		// copy the data
		if ((unsigned long) data_size < sizeof(clipboard_buf)) {
			cb->size = data_size;
			memcpy(cb->data, clipboard_string, data_size);
			cb->type = CLIPBOARD_TEXT;
			mprintf( "CLIPBOARD STRING: %s\n", cb->data );
			mprintf( "Len %d, Strlen %d\n", cb->size, strlen( cb->data ) );
		}
		else {
			mprintf( "Could not copy, clipboard too large! %d bytes\n", data_size );
		}
	}
	else if (clipboardHasImage) {
		// malloc some space for the image
        OSXImage *clipboard_image = (OSXImage *) malloc(sizeof(OSXImage));
        memset(clipboard_image, 0, sizeof(OSXImage));	

		// get the image and its size
		ClipboardGetImage(clipboard_image);
		int data_size = clipboard_image->size + 14;

		// copy the data
		if ((unsigned long) data_size < sizeof(clipboard_buf)) {
			cb->size = data_size;
			memcpy(cb->data, clipboard_image->data + 14, data_size);
			// dimensions for sanity check
			mprintf( "Width: %d\n", (*(int*)&cb->data[4]) );
			mprintf( "Height: %d\n", (*(int*)&cb->data[8]) );
			// data type and length
			cb->type = CLIPBOARD_IMAGE;
			mprintf( "OSX Image! Size: %d\n", cb->size );
			// now that the image is in Clipboard struct, we can free this struct
			free(clipboard_image);
		}
		else {
			mprintf( "Could not copy, clipboard too large! %d bytes\n", data_size );
		}
	}
	else {
		mprintf("Nothing in the clipboard!\n");
	}
#else
// TODO: LINUX UBUNTU/DEBIAN
#endif
	return cb;
}

#if defined(_WIN32)
HGLOBAL getGlobalAlloc( void* buf, int len )
{
	HGLOBAL hMem = GlobalAlloc( GMEM_MOVEABLE, len );
	LPTSTR lptstr = GlobalLock( hMem );

	if( lptstr == NULL )
	{
		mprintf( "getGlobalAlloc GlobalLock failed! Size %d\n", len );
		return hMem;
	}

	memcpy( lptstr, buf, len );
	GlobalUnlock( hMem );

	return hMem;
}
#endif

void SetClipboard( ClipboardData* cb )
{
	if( cb->type == CLIPBOARD_NONE )
	{
		return;
	}

#if defined(_WIN32)
	int cf_type = -1;
	HGLOBAL hMem = NULL;

	switch( cb->type )
	{
	case CLIPBOARD_TEXT:
		mprintf( "SetClipboard to Text: %s\n", cb->data );
		if( cb->size > 0 )
		{
			cf_type = CF_TEXT;
			hMem = getGlobalAlloc( cb->data, cb->size );
		}
		break;
	case CLIPBOARD_IMAGE:
		mprintf( "SetClipboard to Image with size %d\n", cb->size );
		if( cb->size > 0 )
		{
			cf_type = CF_DIB;
			hMem = getGlobalAlloc( cb->data, cb->size );
		}
		break;
	case CLIPBOARD_FILES:
		mprintf( "SetClipboard to Files\n" );

		WCHAR first_file_path[MAX_PATH] = L"";
		wcscat( first_file_path, LSET_CLIPBOARD );
		wcscat( first_file_path, L"\\*" );

		WIN32_FIND_DATAW data;
		HANDLE hFind = FindFirstFileW( first_file_path, &data );

		DROPFILES* drop = (DROPFILES*)clipboard_buf;
		memset( drop, 0, sizeof( DROPFILES ) );
		WCHAR* file_ptr = (WCHAR*)(clipboard_buf + sizeof( DROPFILES ));
		int total_len = sizeof( DROPFILES );
		int num_files = 0;
		drop->pFiles = total_len;
		drop->fWide = true;
		drop->fNC = false;

		WCHAR file_prefix[MAX_PATH] = L"";
		wcscat( file_prefix, LSET_CLIPBOARD );
		wcscat( file_prefix, L"\\" );

		int file_prefix_len = wcslen( file_prefix ); // Including null terminator

		if( hFind != INVALID_HANDLE_VALUE )
		{
			WCHAR* ignore1 = L".";
			WCHAR* ignore2 = L"..";

			do
			{
				if( wcscmp( data.cFileName, ignore1 ) == 0 || wcscmp( data.cFileName, ignore2 ) == 0 )
				{
					continue;
				}

				memcpy( file_ptr, file_prefix, file_prefix_len );
				// file_ptr moves in terms of WCHAR
				file_ptr += file_prefix_len;
				// total_len moves in terms of bytes
				total_len += file_prefix_len * sizeof(WCHAR);

				int len = (int)wcslen( data.cFileName ) + 1;

				mprintf( "FILENAME: %S\n", data.cFileName );

				memcpy( file_ptr, data.cFileName, sizeof(WCHAR)*len );
				file_ptr += len;

				num_files++;
				total_len += sizeof( WCHAR )*len;
			} while( FindNextFileW( hFind, &data ) );
			FindClose( hFind );
		}

		*file_ptr = L'\0';
		total_len += sizeof( L'\0' );

		cf_type = CF_HDROP;
		hMem = getGlobalAlloc( drop, total_len );

		break;
	default:
		mprintf( "Unknown clipboard type!\n" );
		break;
	}

	if( cf_type != -1 )
	{
		if( !OpenClipboard( NULL ) ) return;

		EmptyClipboard();
		SetClipboardData( cf_type, hMem );

		CloseClipboard();
	}

#elif __APPLE__
	// check the type of the data
	switch(cb->type) {
		case CLIPBOARD_TEXT:
		mprintf("SetClipboard to Text: %s\n", cb->data);
		ClipboardSetString(cb->data);
		break;
	case CLIPBOARD_IMAGE:
		mprintf("SetClipboard to Image with size %d\n", cb->size);
		// fix the CGImage header back
		char* data = malloc(cb->size + 14);
		*((char*)(&data[0])) = 'B';
		*((char*)(&data[1])) = 'M';
		*((int*)(&data[2])) = cb->size + 14;
		*((int*)(&data[10])) = 54;
		memcpy(data+14, cb->data, cb->size);
		// set the image and free the temp data
		ClipboardSetImage(data, cb->size + 14);
		free(data);
		break;
	case CLIPBOARD_FILES:
		mprintf( "SetClipboard to Files\n" );

		// allocate memory to store filenames in clipboard
		char* filenames[MAX_URLS];
		for( size_t i = 0; i < MAX_URLS; i++ )
		{
			filenames[i] = (char*)malloc( PATH_MAX * sizeof( char ) );
			memset( filenames[i], '\0', PATH_MAX * sizeof( char ) );
		}

		// populate filenames
		get_filenames( SET_CLIPBOARD, filenames );

		// add files to clipboard
		ClipboardSetFiles( filenames );

		// free memory
		for( size_t i = 0; i < MAX_URLS; i++ )
		{
			free( filenames[i] );
		}

		break;
	default:
		mprintf("No clipboard data to set!\n");
		break;
	}
#else
// TODO: LINUX UBUNTU/DEBIAN
#endif
	// Update the status so that this specific update doesn't count
	hasClipboardUpdated();
}

// CLIPBOARD THREAD HANDLING

int UpdateClipboardThread( void* opaque );

bool updating_set_clipboard;
bool updating_clipboard;
bool pending_update_clipboard;
clock last_clipboard_update;
SDL_sem* clipboard_semaphore;
ClipboardData* clipboard;
SEND_FMSG* send_fmsg;
SDL_Thread* thread;
bool connected;
char* server_ip;
ClipboardData* clipboard;

bool isUpdatingClipboard()
{
	return updating_clipboard;
}

bool updateSetClipboard( ClipboardData* cb )
{
	if( updating_clipboard )
	{
		mprintf( "Tried to SetClipboard, but clipboard is updating\n" );
		return false;
	}

	updating_clipboard = true;
	updating_set_clipboard = true;
	clipboard = cb;

	SDL_SemPost( clipboard_semaphore );

	return true;
}

bool pendingUpdateClipboard()
{
	return pending_update_clipboard;
}

void initUpdateClipboard( SEND_FMSG* send_fmsg_local, char* server_ip_local )
{
	connected = true;

	server_ip = server_ip_local;
	send_fmsg = send_fmsg_local;

	updating_clipboard = false;
	pending_update_clipboard = false;
	StartTimer( (clock*)&last_clipboard_update );
	clipboard_semaphore = SDL_CreateSemaphore( 0 );

	thread = SDL_CreateThread( UpdateClipboardThread, "UpdateClipboardThread", NULL );

	updateClipboard();
	StartTrackingClipboardUpdates();
}

void destroyUpdateClipboard()
{
	connected = false;
	SDL_SemPost( clipboard_semaphore );
}

int UpdateClipboardThread( void* opaque )
{
	opaque;

	while( connected )
	{
		SDL_SemWait( clipboard_semaphore );

		if( !connected )
		{
			break;
		}

		//ClipboardData* clipboard = GetClipboard();

		if( updating_set_clipboard )
		{
			mprintf( "Trying to set clipboard!\n" );
			if( clipboard->type == CLIPBOARD_FILES )
			{

			}
			SetClipboard( clipboard );
		} else
		{
			clock clipboard_time;
			StartTimer( &clipboard_time );

			if( clipboard->type == CLIPBOARD_FILES )
			{
				char cmd[1000] = "";
#ifndef _WIN32
				strcat( cmd, "UNISON=./.unison; " );
#endif

#ifdef _WIN32
				strcat( cmd, "unison " );
#else
				strcat( cmd, "./unison -follow \"Path *\" " );
#endif

				strcat( cmd, "-ui text -sshargs \"-l vm1 -i sshkey\" " );
				strcat( cmd, GET_CLIPBOARD );
				strcat( cmd, " \"ssh://" );
				strcat( cmd, (char*)server_ip );
				strcat( cmd, "/" );
				strcat( cmd, "C:\\Users\\vm1\\AppData\\Roaming\\FractalCache\\set_clipboard" );
				strcat( cmd, "/\" -force " );
				strcat( cmd, GET_CLIPBOARD );
				strcat( cmd, " -ignorearchives -confirmbigdel=false -batch" );

				mprintf( "COMMAND: %s\n", cmd );
				runcmd( cmd );
			}

			FractalClientMessage* fmsg = malloc( sizeof( FractalClientMessage ) + sizeof( ClipboardData ) + clipboard->size );
			fmsg->type = CMESSAGE_CLIPBOARD;
			memcpy( &fmsg->clipboard, clipboard, sizeof( ClipboardData ) + clipboard->size );
			send_fmsg( fmsg );
			free( fmsg );

			// If it hasn't been 500ms yet, then wait 500ms to prevent too much spam
			const int spam_time_ms = 500;
			if( GetTimer( clipboard_time ) < spam_time_ms / 1000.0 )
			{
				SDL_Delay( max( (int)(spam_time_ms - 1000*GetTimer( clipboard_time )), 1 ) );
			}
		}

		mprintf( "Updated clipboard!\n" );
		updating_clipboard = false;
	}

	return 0;
}

void updateClipboard()
{
	if( updating_clipboard )
	{
		pending_update_clipboard = true;
	} else
	{
		mprintf( "Pushing update to clipboard\n" );
		pending_update_clipboard = false;
		updating_clipboard = true;
		updating_set_clipboard = false;
		clipboard = GetClipboard();
		SDL_SemPost( clipboard_semaphore );
	}
}
