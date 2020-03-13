#include "clipboard.h"
#include "fractal.h"

ClipboardData GetClipboard()
{
	if( !OpenClipboard( NULL ) ) return;

	int cf_types[] = {
		CF_TEXT,
		CF_UNICODETEXT,
		CF_DIBV5,
	};

	int data_size = -1;
	char* data = NULL;
	int cf_type = -1;

	for( int i = 0; i < sizeof( cf_types ) / sizeof( cf_types[0] ) && data == NULL; i++ )
	{
		if( IsClipboardFormatAvailable( cf_types[i] ) )
		{
			HGLOBAL hglb = GetClipboardData( cf_types[i] );
			if( hglb != NULL )
			{
				LPTSTR lptstr = GlobalLock( hglb );
				if( lptstr != NULL )
				{
					data_size = GlobalSize( hglb );
					data = lptstr;
					cf_type = cf_types[i];

					// Don't forget to release the lock after you are done.
					GlobalUnlock( hglb );
				} else
				{
					mprintf( "GlobalLock failed! (Type: %d) (Error: %d)\n", cf_types[i], GetLastError() );
				}
			}
		}
	}

	ClipboardData cb;
	cb.size = 0;

	if( data && data_size < 800 )
	{
		cb.size = data_size;
		memcpy( cb.data, data, data_size );

		switch( cf_type )
		{
		case CF_TEXT:
			cb.type = CLIPBOARD_TEXT;
			// Read the contents of lptstr which just a pointer to the string.
			mprintf( "CLIPBOARD STRING: %s\n", data );
			mprintf( "CLIPBOARD STRING: %s\n", cb.data );
			mprintf( "Len %d\n Strlen %d\n", data_size, strlen( data ) );
			break;
		case CF_DIBV5:
			cb.type = CLIPBOARD_IMAGE;
			mprintf( "Dib! Size: %d\n", data_size );

			break;
		default:
			mprintf( "Clipboard type unknown: %d\n", cf_type );

			cb.size = 0;
			cb.type = CLIPBOARD_NONE;
			break;
		}
	}

	CloseClipboard();

	return cb;
}