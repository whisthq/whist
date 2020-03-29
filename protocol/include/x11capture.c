//
// Created by mathieu on 28/03/2020.
//
#include "x11capture.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <X11/extensions/Xdamage.h>

int CreateCaptureDevice( struct CaptureDevice* device, UINT width, UINT height )
{
    device->width = width;
    device->height = height;
    device->display = XOpenDisplay( NULL );
    device->root = DefaultRootWindow( device->display );
    XWindowAttributes window_attributes;
    if( !XGetWindowAttributes( device->display, device->root, &window_attributes ) )
    {
        fprintf( stderr, "Error while getting window attributes" );
        return -1;
    }
    Screen* screen = window_attributes.screen;
    device->image = XShmCreateImage( device->display,
                                     DefaultVisualOfScreen( screen ), //DefaultVisual(device->display, 0), // Use a correct visual. Omitted for brevity
                                     DefaultDepthOfScreen( screen ), //24,   // Determine correct depth from the visual. Omitted for brevity
                                     ZPixmap, NULL, &device->segment, width, height );


    device->segment.shmid = shmget( IPC_PRIVATE,
                                    device->image->bytes_per_line * device->image->height,
                                    IPC_CREAT | 0777 );


    device->segment.shmaddr = device->image->data = shmat( device->segment.shmid, 0, 0 );
    device->segment.readOnly = False;


    if( !XShmAttach( device->display, &device->segment ) )
    {
        fprintf( stderr, "Error while attaching display" );
        return -1;
    }
    device->frame_data = device->image->data;
    int damage_event, damage_error;
    XDamageQueryExtension( device->display, &damage_event, &damage_error );
    device->damage = XDamageCreate( device->display, device->root, XDamageReportRawRectangles );
    device->event = damage_event;
    return 0;
}

int CaptureScreen( struct CaptureDevice* device )
{
    int update = 0;
    while( XPending( device->display ) )
    {

        XDamageNotifyEvent* dev;
        XEvent ev;
        XNextEvent( device->display, &ev );
        if( ev.type == device->event + XDamageNotify )
        {
            update = 1;
        }

    }
    if( update )
    {
        XDamageSubtract( device->display, device->damage, None, None );
        if( !XShmGetImage( device->display,
                           device->root,
                           device->image,
                           0,
                           0,
                           AllPlanes ) )
        {
            fprintf( stderr, "Error while capturing the screen" );
            return -1;
        }
    }
    return update;
}

void ReleaseScreen( struct CaptureDevice* device )
{

}

void DestroyCaptureDevice( struct CaptureDevice* device )
{
    XFree( device->image );
    XCloseDisplay( device->display );
}
