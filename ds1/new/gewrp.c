/*
 * =====================================================================================
 *
 *       Filename: gewrp.c
 *        Created: 10/09/2013 10:39:22 PM
 *  Last Modified: 10/18/2013 12:21:19 AM
 *
 *    Description: wrp for allegro
 *                 you can also wrp other game library
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#include "gewrp.h"


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  gewrp_init
 *  Description:  init the game engine wrp
 * =====================================================================================
 */
int gewrp_init ( uint32_t arg )
{
    if( !al_init() ){
        fprintf ( stderr, "fatal error: open allegro library failed...\n" );
        exit(0);
    }
    al_install_keyboard();
    al_install_mouse();


    glb_gewrp_inst.disp = al_create_display(640, 480);

    if ( glb_gewrp_inst.disp == NULL ) {
        fprintf ( stderr, "fatal error: open display failed...\n" );
        exit(0);
    }


    glb_gewrp_inst.fps = 60;
    glb_gewrp_inst.timer  = al_create_timer(1.0 / glb_gewrp_inst.fps);
    glb_gewrp_inst.evtq = al_create_event_queue();
    al_register_event_source(glb_gewrp_inst.evtq, al_get_keyboard_event_source());
    al_register_event_source(glb_gewrp_inst.evtq, al_get_mouse_event_source());
    al_register_event_source(glb_gewrp_inst.evtq, al_get_display_event_source(glb_gewrp_inst.disp));
    al_register_event_source(glb_gewrp_inst.evtq, al_get_timer_event_source(glb_gewrp_inst.timer));

    fprintf(stdout, "here!!!!!!!!!!!!\n" );

    return 0;
}



