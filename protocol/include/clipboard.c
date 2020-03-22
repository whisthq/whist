#include "clipboard.h"
#include "fractal.h"

#if defined(_WIN32)
	#include "shlwapi.h"
	#pragma comment (lib, "Shlwapi.lib")
	#include "Shellapi.h"
	#include "shlobj_core.h"
#endif

#if defined __APPLE__
	#include "clipboard_osx.h"
	bool clipboardHasImage;
	bool clipboardHasString;
#endif

static int last_clipboard_sequence_number = -1;
static char clipboard_buf[9000000];

void StartTrackingClipboardUpdates()
{
#if defined(_WIN32)
	last_clipboard_sequence_number = GetClipboardSequenceNumber();
#elif __APPLE__
	last_clipboard_sequence_number = -1; // to capture the first event
	clipboardHasImage = false;
	clipboardHasString = false;
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
		hasUpdated = (clipboardHasImage || clipboardHasString); // should be always set to true in here
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

	if( cf_type > -1 )
	{
		switch( cf_type )
		{
		case CF_TEXT:
			cb->type = CLIPBOARD_TEXT;
			// Read the contents of lptstr which just a pointer to the string.
			//mprintf( "CLIPBOARD STRING: %s\n", cb->data );
			break;
		case CF_DIB:
			cb->type = CLIPBOARD_IMAGE;
			//mprintf( "Clipboard bitmap received! Size: %d\n", cb->size );
			break;
		case CF_HDROP:
			mprintf( "Hdrop! Size: %d\n", cb->size );
			DROPFILES drop;
			//memcpy( &drop, cb->data, sizeof( drop ) );

			// Begin copy to clipboard
			WCHAR* filename = (WCHAR*)(cb->data + sizeof( drop ));
			cb->type = CLIPBOARD_FILES;
			cb->size = 0;

			SHFILEOPSTRUCTA sh = { 0 };
			sh.wFunc = FO_DELETE;
			sh.fFlags = 0;
			sh.fFlags |= FOF_SILENT;
			sh.fFlags |= FOF_NOCONFIRMMKDIR;
			sh.fFlags |= FOF_NOCONFIRMATION;
			sh.fFlags |= FOF_WANTMAPPINGHANDLE;
			sh.fFlags |= FOF_NOERRORUI;
			sh.pFrom = "clipboard";

			SHFileOperationA( &sh );

			RemoveDirectoryW( L"clipboard" );
			CreateDirectoryW( L"clipboard", NULL );

			// Go through filenames
			while( *filename != L'\0' )
			{
				WCHAR* fileending = PathFindFileNameW( filename );
				DWORD fileattributes = GetFileAttributesW( filename );

				WCHAR target_file[1000];
				WCHAR* clipboard = L"clipboard\\";
				int clipboard_len = wcslen( clipboard );
				memcpy( target_file, clipboard, sizeof(WCHAR)*clipboard_len );
				memcpy( target_file + clipboard_len, fileending, sizeof( WCHAR )*(wcslen( fileending ) + 1) );

				if( !CreateSymbolicLinkW( target_file, filename, fileattributes & FILE_ATTRIBUTE_DIRECTORY ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0 ) )
				{
					mprintf( "ERROR: %d\n", GetLastError() );
				}

				mprintf( "TARGET FILENAME: %S\n", target_file );
				mprintf( "FILENAME: %S\n", filename );
				mprintf( "FILENAME ENDING: %S\n", fileending );

				filename += wcslen( filename ) + 1;
			}

			break;
		default:
			cb->type = CLIPBOARD_NONE;
			mprintf( "Clipboard type unknown: %d\n", cf_type );
			break;
		}
	}

	CloseClipboard();
#elif __APPLE__
	if (clipboardHasString) {
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

void SetClipboard( ClipboardData* cb )
{
	if( cb->type == CLIPBOARD_NONE )
	{
		return;
	}

#if defined(_WIN32)
	int cf_type = -1;
	HGLOBAL hMem;

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

		WIN32_FIND_DATAW data;
		HANDLE hFind = FindFirstFileW( L"C:\\Program Files\\Fractal\\clipboard\\*", &data );


		DROPFILES* drop = (DROPFILES*)clipboard_buf;
		memset( drop, 0, sizeof( DROPFILES ) );
		WCHAR* file_ptr = (WCHAR*)(clipboard_buf + sizeof( DROPFILES ));
		int total_len = sizeof( DROPFILES );
		int num_files = 0;
		drop->pFiles = total_len;
		drop->fWide = true;
		drop->fNC = false;

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

				int len = wcslen( data.cFileName ) + 1;

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
