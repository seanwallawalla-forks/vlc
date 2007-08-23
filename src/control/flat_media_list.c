/*****************************************************************************
 * flat_media_list.c: libvlc flat media list functions. (extension to
 * media_list.c).
 *****************************************************************************
 * Copyright (C) 2007 the VideoLAN team
 * $Id: flat_media_list.c 21287 2007-08-20 01:28:12Z pdherbemont $
 *
 * Authors: Pierre d'Herbemont <pdherbemont # videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "libvlc_internal.h"
#include <vlc/libvlc.h>
#include <assert.h>
#include "vlc_arrays.h"

/*
 * Private functions
 */
static void add_item( libvlc_media_list_t * p_mlist, libvlc_media_descriptor_t * p_md );
static void remove_item( libvlc_media_list_t * p_mlist, libvlc_media_descriptor_t * p_md );
static void subitems_created( const libvlc_event_t * p_event , void * p_user_data);
static void sublist_item_added( const libvlc_event_t * p_event, void * p_user_data );
static void sublist_item_removed( const libvlc_event_t * p_event, void * p_user_data );
static void add_media_list( libvlc_media_list_t * p_mlist, libvlc_media_list_t * p_submlist );
static void remove_media_list( libvlc_media_list_t * p_fmlist, libvlc_media_list_t * p_mlist );

/**************************************************************************
 *       add_item (private) 
 **************************************************************************/
static void
add_item( libvlc_media_list_t * p_mlist, libvlc_media_descriptor_t * p_md )
{
    /* Only add the media descriptor once to our flat list */
    if( libvlc_media_list_index_of_item( p_mlist->p_flat_mlist, p_md, NULL ) < 0 )
    {
        if( p_md->p_subitems )
        {
            add_media_list( p_mlist->p_flat_mlist, p_md->p_subitems );
        }
        else
        {
            libvlc_event_attach( p_md->p_event_manager,
                                 libvlc_MediaDescriptorSubItemAdded,
                                 subitems_created, p_mlist, NULL );
            libvlc_media_list_add_media_descriptor( p_mlist->p_flat_mlist,
                                                    p_md, NULL );
        }
    }
}

/**************************************************************************
 *       remove_item (private) 
 **************************************************************************/
static void
remove_item( libvlc_media_list_t * p_mlist, libvlc_media_descriptor_t * p_md )
{
    if( p_md->p_subitems ) /* XXX: Don't access that directly */
        remove_media_list( p_mlist, p_md->p_subitems );
    libvlc_event_detach( p_md->p_event_manager,
                         libvlc_MediaDescriptorSubItemAdded,
                         subitems_created, p_mlist, NULL );
    int i = libvlc_media_list_index_of_item( p_mlist->p_flat_mlist, p_md, NULL );
    libvlc_media_list_remove_index( p_mlist->p_flat_mlist, i, NULL );
}

/**************************************************************************
 *       subitems_created (private) (Event Callback)
 **************************************************************************/
static void
subitems_created( const libvlc_event_t * p_event , void * p_user_data)
{
    libvlc_media_list_t * p_mlist = p_user_data;
    libvlc_media_descriptor_t * p_md = p_event->u.media_list_item_deleted.item;

    /* Remove the item and add the playlist */
    remove_item( p_mlist->p_flat_mlist, p_md );

    add_media_list( p_mlist->p_flat_mlist, p_md->p_subitems );
}

/**************************************************************************
 *       sublist_item_added (private) (Event Callback)
 *
 * This is called if the dynamic sublist's data provider adds a new item.
 **************************************************************************/
static void
sublist_item_added( const libvlc_event_t * p_event, void * p_user_data )
{
    libvlc_media_list_t * p_mlist = p_user_data;
    libvlc_media_descriptor_t * p_md = p_event->u.media_list_item_added.item;
    add_item( p_mlist, p_md );
}

/**************************************************************************
 *       sublist_remove_item (private) (Event Callback)
 **************************************************************************/
static void
sublist_item_removed( const libvlc_event_t * p_event, void * p_user_data )
{
    libvlc_media_list_t * p_mlist = p_user_data;
    libvlc_media_descriptor_t * p_md = p_event->u.media_list_item_deleted.item;
    remove_item( p_mlist, p_md );
}

/**************************************************************************
 *       add_media_list (Private)
 **************************************************************************/
void
add_media_list( libvlc_media_list_t * p_mlist,
                libvlc_media_list_t * p_submlist )
{
    int count = libvlc_media_list_count( p_submlist, NULL );
    int i;

    libvlc_event_attach( p_submlist->p_event_manager,
                         libvlc_MediaListItemAdded,
                         sublist_item_added, p_mlist, NULL );
    libvlc_event_attach( p_submlist->p_event_manager,
                         libvlc_MediaListItemDeleted,
                         sublist_item_removed, p_mlist, NULL );
    for( i = 0; i < count; i++ )
    {
        libvlc_media_descriptor_t * p_md;
        p_md = libvlc_media_list_item_at_index( p_submlist, i, NULL );
        add_item( p_mlist->p_flat_mlist, p_md );    
    }
}

/**************************************************************************
 *       remove_media_list (Private)
 **************************************************************************/
void
remove_media_list( libvlc_media_list_t * p_mlist,
                   libvlc_media_list_t * p_submlist )
{
    int count = libvlc_media_list_count( p_submlist, NULL );    
    int i;
    for( i = 0; i < count; i++ )
    {
        libvlc_media_descriptor_t * p_md;
        p_md = libvlc_media_list_item_at_index( p_submlist, i, NULL );
        remove_item( p_mlist, p_md );
    }
    
    libvlc_event_detach( p_mlist->p_event_manager,
                         libvlc_MediaListItemAdded,
                         sublist_item_added, p_mlist, NULL );
    libvlc_event_detach( p_mlist->p_event_manager,
                         libvlc_MediaListItemDeleted,
                         sublist_item_removed, p_mlist, NULL );
}


/*
 * Public libvlc functions
 */

/**************************************************************************
 *       media_list (Public)
 **************************************************************************/
libvlc_media_list_t *
libvlc_media_list_flat_media_list( libvlc_media_list_t * p_mlist,
                                   libvlc_exception_t * p_e )
{
    if( !p_mlist->p_flat_mlist )
    {
        p_mlist->p_flat_mlist = libvlc_media_list_new(
                                            p_mlist->p_libvlc_instance,
                                            p_e );
        add_media_list( p_mlist->p_flat_mlist, p_mlist );
    }
    libvlc_media_list_retain( p_mlist->p_flat_mlist );
    return p_mlist->p_flat_mlist;
}
