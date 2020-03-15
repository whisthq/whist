#include "clipboard.h"
#include "fractal.h"

#if defined(_WIN32)
static int last_clipboard_sequence_number = -1;
#endif

static char clipboard_buf[9000000];
static ClipboardData* cb;

void StartTrackingClipboardUpdates()
{
#if defined(_WIN32)
	last_clipboard_sequence_number = GetClipboardSequenceNumber();
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
#endif

	return hasUpdated;
}

ClipboardData* GetClipboard()
{
	cb = clipboard_buf;

	cb->size = 0;
	cb->type = CLIPBOARD_NONE;

#if defined(_WIN32)
	if( !OpenClipboard( NULL ) )
	{
		return cb;
	}

	int cf_types[] = {
		CF_TEXT,
		CF_DIBV5,
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
			mprintf( "CLIPBOARD STRING: %s\n", cb->data );
			mprintf( "Len %d\n Strlen %d\n", cb->size, strlen( cb->data ) );
			break;
		case CF_DIBV5:
			cb->type = CLIPBOARD_IMAGE;
			mprintf( "Dib! Size: %d\n", cb->size );

			break;
		default:
			cb->type = CLIPBOARD_NONE;
			mprintf( "Clipboard type unknown: %d\n", cf_type );
			break;
		}
	}

	CloseClipboard();
#endif

	return cb;
}

void SetClipboard( ClipboardData* cb )
{
#if defined(_WIN32)
	if( cb->size == 0 || cb->type == CLIPBOARD_NONE )
	{
		return;
	}

	HGLOBAL hMem = GlobalAlloc( GMEM_MOVEABLE, cb->size );
	LPTSTR lptstr = GlobalLock( hMem );

	if( lptstr == NULL )
	{
		mprintf( "SetClipboard GlobalLock failed!\n" );
		return;
	}

	memcpy( lptstr, cb->data, cb->size );
	GlobalUnlock( hMem );

	int cf_type = -1;

	switch( cb->type )
	{
	case CLIPBOARD_TEXT:
		cf_type = CF_TEXT;
		mprintf( "SetClipboard to Text %s\n", cb->data );
		break;
	case CLIPBOARD_IMAGE:
		cf_type = CF_DIBV5;
		mprintf( "SetClipboard to Image with size %d\n", cb->size );
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
#endif

	// Update the status so that this specific update doesn't count
	hasClipboardUpdated();
}
