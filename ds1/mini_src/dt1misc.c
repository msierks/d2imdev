#include <string.h>
#include "structs.h"
#include "error.h"
#include "dt1_draw.h"
#include "misc.h"
#include "mpqview.h"
#include "dt1misc.h"

// ==========================================================================
// fill the sub-tile structure with the right sub-header data from the file
void dt1_fill_subt3(SUB_TILE_S * ptr, int i, long tiles_ptr, int s, char *p )
{
    char * st_ptr;

    char *ptr2 = glb_dt1[i].buffer2;
    ptr2 += glb_dt1[i].buff_len2;


    st_ptr = (char *) glb_dt1[i].buffer + tiles_ptr + (20 * s);
    ptr->x_pos       = * (WORD *)  p; p+=2;
    //memcpy(ptr2, st_ptr, 2);
    //ptr2+=2;
    ptr->y_pos       = * (WORD *)  p; p+=2;
    //memcpy(ptr2, st_ptr+2, 2);
    //ptr2+=2;
    //// skip 2 bytes : unknown1
    ptr->x_grid      = * (UBYTE *) p; p+=1;
    //memcpy(ptr2, st_ptr+6, 1);
    //ptr2+=1;
    ptr->y_grid      = * (UBYTE *) p; p+=1;
    //memcpy(ptr2, st_ptr+7, 1);
    //ptr2+=1;
    ptr->format      = * (WORD *) p; p+=2;
    //memcpy(ptr2, st_ptr+8, 2);
    //ptr2+=2;
    ptr->length      = * (long *) p; p+=4;
    //memcpy(ptr2, st_ptr+10, 4);
    //ptr2+=4;
    //// skip 2 bytes : unknown2
    //ptr->data_offset = * (long *)  ((UBYTE *)st_ptr + 16);
    ptr->data_offset = p - (char *)glb_dt1[i].buffer2;

    ////glb_dt1[i].buff_len2 += ptr2 - (char*)glb_dt1[i].buffer2;
    //glb_dt1[i].buff_len2 += 12;
}


void dt1_all_zoom_make3(int i, int offset)
{
    BLOCK_S       * b_ptr, * my_b_ptr; // pointers to current block header
    SUB_TILE_S    st_ptr;  // current sub-tile header
    BITMAP        * tmp_bmp, * sprite;
    int           b, w, h, s, x0, y0, length, y_add, z, mem_size;
    UBYTE         * data;
    WORD          format;
    long          orientation;
    char          tmp_str[100];
    int           t_mi, t_si, my_idx;

    char   * p;


    p = glb_dt1[i].buffer2 + offset;


    if( i!=2){
        return;
    }

    b_ptr    = (BLOCK_S *) glb_dt1[i].bh_buffer;

    fprintf(stderr, "making zoom for glb_dt1[%i] (%s) ", i, glb_dt1[i].name);
    fflush(stderr);
    // get mem for table of pointers
    for (z=0; z<ZM_MAX; z++) {
        //一个block就是一个tile???
        mem_size = sizeof(BITMAP *) * glb_dt1[i].block_num;
        glb_dt1[i].block_zoom[z] = (BITMAP **) malloc(mem_size);
        if (glb_dt1[i].block_zoom[z] == NULL) {
            FATAL_EXIT("dt1_all_zoom_make(%i), zoom %i, not enough mem for %i bytes\n", i, z, mem_size);
        }
        memset(glb_dt1[i].block_zoom[z], 0, mem_size);
        glb_dt1[i].bz_size[z] = mem_size;
    }

    // make the bitmaps
    for (b=0; b < glb_dt1[i].block_num; b++){
        // for each blocks of a dt1
        //从这里来看..貌似一个block就是一个tile
        // get infos
        orientation = b_ptr->orientation;

        // prepare tmp bitmap size
        w = b_ptr->size_x;
        if ((orientation == 10) || (orientation == 11)){
            // 10或者11是special layer
            // set it to 160 because we'll draw infos over it later
            w = 160;
        }
        //这个地方...图片的高直接是个负值...
        h = - b_ptr->size_y;

        // adjustment (which y line in the bitmap is the zero line ?)

        // by default, when orientation > 15 : lower wall
        y_add = 96;
        if ((orientation == 0) || (orientation == 15)){
            // floor or roof
            if (b_ptr->size_y) {
                b_ptr->size_y = - 80;
                h = 80;
                y_add = 0;
            }
        } else if (orientation < 15){
            // upper wall, shadow, special
            if (b_ptr->size_y) {
                b_ptr->size_y += 32;
                h -= 32;
                y_add = h;
            }
        }

        // anti-bug (for empty block)
        if ((w == 0) || (h == 0)) {
            fprintf(stderr, "0");
            fflush(stderr);
            b_ptr ++;
            continue;
        }

        // normal block (non-empty)
        tmp_bmp = create_bitmap(w, h);
        if (tmp_bmp == NULL) {
            FATAL_EXIT("dt1_all_zoom_make(%i), can't make a bitmap of %i * %i pixels\n", i, w, h);
        }
        clear(tmp_bmp);

        // draw sub-tiles in this bitmap
        // first we need to fill the block data offset here

        ///{
        ///    char *p;

        ///    // be careful!!!!!!!!!!!!
        ///    // 60 may vary
        ///    p = 61*b + 60 + (char*)glb_dt1[i].buffer2;

        ///    fprintf(stdout, "to wirte the offset: glb_dt1[i].buff_len2=%lu\n", glb_dt1[i].buff_len2 );
        ///    memcpy(p, &(glb_dt1[i].buff_len2), 4);
        ///}
        for (s=0; s < b_ptr->tiles_number; s++){
            // for each sub-tiles
            //一个tile由多个subtile构成的
            // get the sub-tile info
            //dt1_fill_subt(& st_ptr, i, b_ptr->tiles_ptr, s);
            //dt1_fill_subt2(& st_ptr, i, b_ptr->tiles_ptr, s);
            dt1_fill_subt3(& st_ptr, i, b_ptr->tiles_ptr, s, p);
            p+=12;

            // get infos
            x0     = st_ptr.x_pos;
            y0     = y_add + st_ptr.y_pos;
            //data   = (UBYTE *) ((UBYTE *)glb_dt1[i].buffer + b_ptr->tiles_ptr + st_ptr.data_offset);
            data   = (UBYTE *)( p );
            length = st_ptr.length;
            format = st_ptr.format;

            p += length;

            //
            //{
            //    char *ptr2 = glb_dt1[i].buffer2;
            //    ptr2 += glb_dt1[i].buff_len2;
            //    memcpy(ptr2, data, length);
            //    glb_dt1[i].buff_len2 += length;

            //}
            // draw the sub-tile
            if (format == 0x0001){
                //这个画法我已经知道了
                //参见dt1 tool的解析
                draw_sub_tile_isometric(tmp_bmp, x0, y0, data, length);
            }else{
                draw_sub_tile_normal(tmp_bmp, x0, y0, data, length);
            }
        }

        // if a game's special tile, draw my own info over it
        if ( (glb_ds1edit.cmd_line.no_vis_debug == FALSE) && (i != 0) && ((orientation == 10) || (orientation == 11))) {
            // get info
            t_mi     = b_ptr->main_idx;
            t_si     = b_ptr->sub_idx;
            my_b_ptr = (BLOCK_S *) glb_dt1[0].bh_buffer;

            // search same info in my dt1
            for (my_idx=0; my_idx < glb_dt1[0].block_num; my_idx++) {
                if ( (my_b_ptr[my_idx].orientation == orientation) && (my_b_ptr[my_idx].main_idx  == t_mi) && (my_b_ptr[my_idx].sub_idx   == t_si)) {
                    // found it, draw that tile over the game's gfx
                    sprite = * (glb_dt1[0].block_zoom[ZM_11] + my_idx);
                    if (sprite != NULL){
                        draw_sprite(tmp_bmp, sprite, 0, tmp_bmp->h - sprite->h);
                    }
                    // stop the search
                    my_idx = glb_dt1[0].block_num;
                }
            }
        }



        // make zoom from the bitmap, for each zoom
        dt1_zoom(tmp_bmp, i, b, ZM_11);
        //* (glb_dt1[i].block_zoom[z] + b) = tmp_bmp;//dst;
#if 0
        for (z=0; z<ZM_MAX; z++){
            dt1_zoom(tmp_bmp, i, b, z);
        }
#endif

        // destroy tmp bitmap
        destroy_bitmap(tmp_bmp);

        // next block header
        b_ptr ++;
        fprintf(stderr, ".");
        fflush(stderr);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}





void dt1_bh_update3(int i, int *offset)
{

    BLOCK_S * bh_ptr = glb_dt1[i].bh_buffer;
    char    * ptr;
    int     b, t, idxtable[25] = {20, 21, 22, 23, 24,
        15, 16, 17, 18, 19,
        10, 11, 12, 13, 14,
        5,  6,  7,  8,  9,
        0,  1,  2,  3,  4};


    char    * p;
    char    * ptr2 = glb_dt1[i].buffer2;


    if(i!=2){
        return;
    }


    // for the "D2T" and # of block
    ptr2 += (3+4);
    p = ptr2;
    ptr = (char *) glb_dt1[i].buffer + glb_dt1[i].bh_start;
    for (b=0; b < glb_dt1[i].block_num; b++) {
        //bh_ptr->direction    = * (long *)  ptr;
        //do not copy it cause it's useless.
        //memcpy(ptr2, ptr, 4);
        //ptr2+=4;
        //
        bh_ptr->roof_y       = * (WORD *) p; p += 2;
        //memcpy(ptr2, ptr+4, 2);
        //ptr2+=2;
        bh_ptr->sound        = * (UBYTE *) p; p += 1;
        //memcpy(ptr2, ptr+6, 1);
        //ptr2+=1;
        bh_ptr->animated     = * (UBYTE *) p; p += 1;
        //memcpy(ptr2, ptr+7, 1);
        //ptr2+=1;
        bh_ptr->size_y       = * (long *) p; p += 4;
        //memcpy(ptr2, ptr+8, 4);
        //ptr2+=4;
        bh_ptr->size_x       = * (long *) p; p += 4;
        //memcpy(ptr2, ptr+12, 4);
        //ptr2+=4;
        //// skip 4 bytes : zeros1
        bh_ptr->orientation  = * (long *) p; p += 4;
        //memcpy(ptr2, ptr+20, 4);
        //ptr2+=4;
        bh_ptr->main_idx   = * (long *) p; p+= 4;
        //memcpy(ptr2, ptr+24, 4);
        //ptr2+=4;
        bh_ptr->sub_idx    = * (long *) p; p+= 4;
        //memcpy(ptr2, ptr+28, 4);
        //ptr2+=4;
        bh_ptr->rarity       = * (long *) p; p+= 4;
        //memcpy(ptr2, ptr+32, 4);
        //ptr2+=4;
        // skip 4 bytes : unknown_a thru unknown_d
        for (t=0; t<25; t++){
            bh_ptr->sub_tiles_flags[idxtable[t]] = * (UBYTE *) p; p+=1;
            // skip 4 bytes : unknown_a thru unknown_d
            //memcpy(ptr2, ptr+40+t, 1);
            //ptr2+=1;
        }
        // skip 7 bytes : zeros2
        bh_ptr->tiles_ptr    = * (long *) p; p+=4;
        //fprintf(stdout, "current pos:%d\n", (char*)ptr2 - (char*)(glb_dt1[i].buffer2));
        //ptr2+=4;
        //bh_ptr->tiles_length = * (long *)  (ptr + 76);
        //ptr2+=4;
        bh_ptr->tiles_number = * (long *) p; p+=4;
        //memcpy(ptr2, ptr+80, 4);
        //ptr2+=4;
        // skip 12 bytes : zeros3
        // next block header
        bh_ptr++;
        //ptr += 96;
    }


    * offset = p - (char *)glb_dt1[i].buffer2;

}



/*
 * ===  FUNCTION  ======================================================================
 *         Name:  dt1_test_gfx
 *  Description:  
 * =====================================================================================
 */
int dt1_test_gfx ( int i )
{

    char *ptr;
    if(i!=2){
        return 0;
    }

    fprintf(stdout, "in dt1_test_gfx\n" );
    /* {{{ open file for reading */
    {
        FILE    *p;
        char    p_path[256];
        int tlen;

        sprintf( p_path, "%s", "./test.d2t" );

        p = fopen( p_path, "r" );
        if ( p == NULL ) {
            fprintf ( stderr, "couldn't open file '%s'; %s\n", p_path, strerror(errno) );
            exit (0);
        }
        {
            fseek(p, 0, SEEK_END);
            tlen = ftell(p);
            fseek(p, 0, SEEK_SET);
            glb_dt1[i].buffer2  = malloc (tlen);
            if ( glb_dt1[i].buffer2==NULL ) {
                fprintf ( stderr, "\ndynamic memory allocation failed\n" );
                exit (0);
            }

        }
        fread(glb_dt1[i].buffer2, tlen, 1, p);

        if( fclose(p) == EOF ) {
            fprintf ( stderr, "couldn't close file '%s'; %s\n", p_path, strerror(errno) );
            exit (0);
        }
    }
    /* }}} */



    ptr = (char *) glb_dt1[i].buffer2;

    fprintf(stdout, "we try to test test.d2t now:\n" );
    ptr += 3; // "D2T"

    glb_dt1[i].block_num = * (long *) ptr;
    fprintf(stdout, "block num = %ld\n", glb_dt1[i].block_num );
    ptr += 4;




    // blocks
    {
        int size;
        size = sizeof(BLOCK_S) * glb_dt1[i].block_num;
        glb_dt1[i].bh_buffer = (void *) malloc(size);
        if (glb_dt1[i].bh_buffer == NULL) {
            FATAL_EXIT("dt1_struct_update(%i), not enough memory for %i bytes\n", i, size);
        }
        glb_dt1[i].bh_buff_len = size;
    }



    {
        int mark;

        dt1_bh_update3(i, &mark);
        dt1_all_zoom_make3(i, mark);

    }


    fprintf(stdout, "end of test to test.d2t now:\n" );
    return 0;
}


// ==========================================================================
// check if this dt1 is already loaded (multiple ds1 can use the same dt1)
int dt1_already_loaded(char * dt1name, int * idx)
{
    int i;



    for (i=0; i<DT1_MAX; i++) {
        if (stricmp(glb_dt1[i].name, dt1name) == 0) {
            * idx = i;
            return TRUE;
        }
    }
    * idx = -1;
    return FALSE;
}


// ==========================================================================
// memory free of a dt1
int dt1_free(int i)
{
    BITMAP  * bmp_ptr;
    int     size = glb_dt1[i].buff_len + glb_dt1[i].bh_buff_len;
    int     z, b;


    if (strlen(glb_dt1[i].name) == 0)
        return 0;

    fprintf(stderr, "      . %s\n", glb_dt1[i].name);
    fflush(stderr);

    // bitmaps in all zoom format
    for (z=0; z<ZM_MAX; z++) {
        if (glb_dt1[i].block_zoom[z] != NULL) {
            for (b=0; b < glb_dt1[i].block_num; b++) {
                bmp_ptr = * (glb_dt1[i].block_zoom[z] + b);
                if (bmp_ptr != NULL) {
                    size += bmp_ptr->w * bmp_ptr->h + sizeof(BITMAP);
                    destroy_bitmap(bmp_ptr);
                }
            }
            size += glb_dt1[i].bz_size[z];
            free(glb_dt1[i].block_zoom[z]);
        }
    }

    // block headers of dt1
    if (glb_dt1[i].bh_buffer != NULL) {
        free(glb_dt1[i].bh_buffer);
    }

    // dt1 buffer
    if (glb_dt1[i].buffer != NULL) {
        free(glb_dt1[i].buffer);
    }

    // end
    memset(& glb_dt1[i], 0, sizeof(DT1_S));
    return size;
}


// ==========================================================================
// a ds1 don't need to use a dt1 anymore
// if the dt1 is still use by another ds1, no change (except the usage count)
// else, free it
int dt1_del(int i)
{
    if ( (i < 0) || (i >= DT1_MAX)){
        return 2;
    }

    if (glb_dt1[i].ds1_usage <= 0){
        return 1;
    }

    glb_dt1[i].ds1_usage--;
    if (glb_dt1[i].ds1_usage == 0){
        dt1_free(i);
    }
    return 0;
}



void dt1_bh_update2(int i)
{


    //BLOCK_S * bh_ptr = glb_dt1[i].bh_buffer;
    char    * ptr;
    char    * ptr2 = glb_dt1[i].buffer2;
    int     b, t;

    // for the "D2T" and # of block
    ptr2 += (3+4);
    ptr = (char *) glb_dt1[i].buffer + glb_dt1[i].bh_start;
    for (b=0; b < glb_dt1[i].block_num; b++) {
        //bh_ptr->direction    = * (long *)  ptr;
        //do not copy it cause it's useless.
        //memcpy(ptr2, ptr, 4);
        //ptr2+=4;
        //
        //bh_ptr->roof_y       = * (WORD *)  (ptr +  4);
        memcpy(ptr2, ptr+4, 2);
        ptr2+=2;
        //bh_ptr->sound        = * (UBYTE *) (ptr +  6);
        memcpy(ptr2, ptr+6, 1);
        ptr2+=1;
        //bh_ptr->animated     = * (UBYTE *) (ptr +  7);
        memcpy(ptr2, ptr+7, 1);
        ptr2+=1;
        //bh_ptr->size_y       = * (long *)  (ptr +  8);
        memcpy(ptr2, ptr+8, 4);
        ptr2+=4;
        //bh_ptr->size_x       = * (long *)  (ptr + 12);
        memcpy(ptr2, ptr+12, 4);
        ptr2+=4;
        // skip 4 bytes : zeros1
        //bh_ptr->orientation  = * (long *)  (ptr + 20);
        memcpy(ptr2, ptr+20, 4);
        ptr2+=4;
        //bh_ptr->main_idx   = * (long *)  (ptr + 24);
        memcpy(ptr2, ptr+24, 4);
        ptr2+=4;
        //bh_ptr->sub_idx    = * (long *)  (ptr + 28);
        memcpy(ptr2, ptr+28, 4);
        ptr2+=4;
        //bh_ptr->rarity       = * (long *)  (ptr + 32);
        memcpy(ptr2, ptr+32, 4);
        ptr2+=4;
        // skip 4 bytes : unknown_a thru unknown_d
        for (t=0; t<25; t++){
            //bh_ptr->sub_tiles_flags[idxtable[t]] = * (UBYTE *) (ptr + 40 + t);
            // skip 4 bytes : unknown_a thru unknown_d
            memcpy(ptr2, ptr+40+t, 1);
            ptr2+=1;

        }
        // skip 7 bytes : zeros2
        //bh_ptr->tiles_ptr    = * (long *)  (ptr + 72);
        //fprintf(stdout, "current pos:%d\n", (char*)ptr2 - (char*)(glb_dt1[i].buffer2));
        ptr2+=4;
        //bh_ptr->tiles_length = * (long *)  (ptr + 76);
        //ptr2+=4;
        //bh_ptr->tiles_number = * (long *)  (ptr + 80);
        memcpy(ptr2, ptr+80, 4);
        ptr2+=4;
        // skip 12 bytes : zeros3
        {
            // for debug
#if 0
            memcpy(ptr2, "I AM HERE", 9);
            ptr2+=9;
#endif 
        }

        // next block header
        //bh_ptr++;
        ptr += 96;
    }


    glb_dt1[i].buff_len2 = ptr2-(char*)(glb_dt1[i].buffer2);
#if 0
    fprintf(stdout, "%lu\n", ptr2);
    fprintf(stdout, "%lu\n", glb_dt1[i].buffer2);
    fprintf(stdout, "glb_dt1[i].buff_len2 = %ld\n", glb_dt1[i].buff_len2);
#endif
}






// ==========================================================================
// fill the block header data of a dt1 with the data from the file
void dt1_bh_update(int i)
{
    BLOCK_S * bh_ptr = glb_dt1[i].bh_buffer;
    UBYTE   * ptr;
    int     b, t, idxtable[25] = {20, 21, 22, 23, 24,
        15, 16, 17, 18, 19,
        10, 11, 12, 13, 14,
        5,  6,  7,  8,  9,
        0,  1,  2,  3,  4};

    ptr = (UBYTE *) glb_dt1[i].buffer + glb_dt1[i].bh_start;
    for (b=0; b < glb_dt1[i].block_num; b++) {
        bh_ptr->direction    = * (long *)  ptr;
        bh_ptr->roof_y       = * (WORD *)  (ptr +  4);
        bh_ptr->sound        = * (UBYTE *) (ptr +  6);
        bh_ptr->animated     = * (UBYTE *) (ptr +  7);
        bh_ptr->size_y       = * (long *)  (ptr +  8);
        bh_ptr->size_x       = * (long *)  (ptr + 12);
        // skip 4 bytes : zeros1
        bh_ptr->orientation  = * (long *)  (ptr + 20);
        bh_ptr->main_idx   = * (long *)  (ptr + 24);
        bh_ptr->sub_idx    = * (long *)  (ptr + 28);
        bh_ptr->rarity       = * (long *)  (ptr + 32);
        // skip 4 bytes : unknown_a thru unknown_d
        for (t=0; t<25; t++){
            bh_ptr->sub_tiles_flags[idxtable[t]] = * (UBYTE *) (ptr + 40 + t);
        }
        // skip 7 bytes : zeros2
        bh_ptr->tiles_ptr    = * (long *)  (ptr + 72);
        bh_ptr->tiles_length = * (long *)  (ptr + 76);
        bh_ptr->tiles_number = * (long *)  (ptr + 80);
        // skip 12 bytes : zeros3

        // next block header
        bh_ptr++;
        ptr += 96;
    }
}

// ==========================================================================
// fill the sub-tile structure with the right sub-header data from the file
void dt1_fill_subt2(SUB_TILE_S * ptr, int i, long tiles_ptr, int s)
{
    char * st_ptr;

    char *ptr2 = glb_dt1[i].buffer2;
    ptr2 += glb_dt1[i].buff_len2;


    st_ptr = (char *) glb_dt1[i].buffer + tiles_ptr + (20 * s);
    //ptr->x_pos       = * (WORD *)  st_ptr;
    memcpy(ptr2, st_ptr, 2);
    ptr2+=2;
    //ptr->y_pos       = * (WORD *)  (st_ptr +  2);
    memcpy(ptr2, st_ptr+2, 2);
    ptr2+=2;
    // skip 2 bytes : unknown1
    //ptr->x_grid      = * (UBYTE *) (st_ptr +  6);
    memcpy(ptr2, st_ptr+6, 1);
    ptr2+=1;
    //ptr->y_grid      = * (UBYTE *) (st_ptr +  7);
    memcpy(ptr2, st_ptr+7, 1);
    ptr2+=1;
    //ptr->format      = * (WORD *)  (st_ptr +  8);
    memcpy(ptr2, st_ptr+8, 2);
    ptr2+=2;
    //ptr->length      = * (long *)  (st_ptr + 10);
    memcpy(ptr2, st_ptr+10, 4);
    ptr2+=4;
    // skip 2 bytes : unknown2
    //ptr->data_offset = * (long *)  ((UBYTE *)st_ptr + 16);

    //glb_dt1[i].buff_len2 += ptr2 - (char*)glb_dt1[i].buffer2;
    glb_dt1[i].buff_len2 += 12;
}

// ==========================================================================
// fill the sub-tile structure with the right sub-header data from the file
void dt1_fill_subt(SUB_TILE_S * ptr, int i, long tiles_ptr, int s)
{
    UBYTE * st_ptr;

    st_ptr = (UBYTE *) glb_dt1[i].buffer + tiles_ptr + (20 * s);
    ptr->x_pos       = * (WORD *)  st_ptr;
    ptr->y_pos       = * (WORD *)  (st_ptr +  2);
    // skip 2 bytes : unknown1
    ptr->x_grid      = * (UBYTE *) (st_ptr +  6);
    ptr->y_grid      = * (UBYTE *) (st_ptr +  7);
    ptr->format      = * (WORD *)  (st_ptr +  8);
    ptr->length      = * (long *)  (st_ptr + 10);
    // skip 2 bytes : unknown2
    ptr->data_offset = * (long *)  ((UBYTE *)st_ptr + 16);
}


// ==========================================================================
// make the bitmap of 1 tile, for 1 zoom
void dt1_zoom(BITMAP * src, int i, int b, int z)
{
    BITMAP * dst;
    int    w = src->w, h = src->h, d=1;
    char   tmp_str[100];

    z = ZM_11;

#if 0
    switch(z) {
        case ZM_11  : break;
        case ZM_12  : d =  2; break;
        case ZM_14  : d =  4; break;
        case ZM_18  : d =  8; break;
        case ZM_116 : d = 16; break;
    }
#endif

    w /= d;
    h /= d;
    dst = create_bitmap(w, h);
    if (dst == NULL) {
        FATAL_EXIT( "dt1_zoom(%i, %i, %i), can't make a bitmap of %i * %i pixels\n", i, b, z, w, h);
    }
    stretch_blit(src, dst, 0, 0, src->w, src->h, 0, 0, w, h);

    * (glb_dt1[i].block_zoom[z] + b) = dst;
}

void dt1_all_zoom_make2(int i)
{
    BLOCK_S       * b_ptr, * my_b_ptr; // pointers to current block header
    SUB_TILE_S    st_ptr;  // current sub-tile header
    BITMAP        * tmp_bmp, * sprite;
    int           b, w, h, s, x0, y0, length, y_add, z, mem_size;
    UBYTE         * data;
    WORD          format;
    long          orientation;
    char          tmp_str[100];
    int           t_mi, t_si, my_idx;

    b_ptr    = (BLOCK_S *) glb_dt1[i].bh_buffer;

    fprintf(stderr, "making zoom for glb_dt1[%i] (%s) ", i, glb_dt1[i].name);
    fflush(stderr);
    // get mem for table of pointers
    for (z=0; z<ZM_MAX; z++) {
        //一个block就是一个tile???
        mem_size = sizeof(BITMAP *) * glb_dt1[i].block_num;
        glb_dt1[i].block_zoom[z] = (BITMAP **) malloc(mem_size);
        if (glb_dt1[i].block_zoom[z] == NULL) {
            FATAL_EXIT("dt1_all_zoom_make(%i), zoom %i, not enough mem for %i bytes\n", i, z, mem_size);
        }
        memset(glb_dt1[i].block_zoom[z], 0, mem_size);
        glb_dt1[i].bz_size[z] = mem_size;
    }

    // make the bitmaps
    for (b=0; b < glb_dt1[i].block_num; b++){
        // for each blocks of a dt1
        //从这里来看..貌似一个block就是一个tile
        // get infos
        orientation = b_ptr->orientation;

        // prepare tmp bitmap size
        w = b_ptr->size_x;
        if ((orientation == 10) || (orientation == 11)){
            // 10或者11是special layer
            // set it to 160 because we'll draw infos over it later
            w = 160;
        }
        //这个地方...图片的高直接是个负值...
        h = - b_ptr->size_y;

        // adjustment (which y line in the bitmap is the zero line ?)

        // by default, when orientation > 15 : lower wall
        y_add = 96;
        if ((orientation == 0) || (orientation == 15)){
            // floor or roof
            if (b_ptr->size_y) {
                b_ptr->size_y = - 80;
                h = 80;
                y_add = 0;
            }
        } else if (orientation < 15){
            // upper wall, shadow, special
            if (b_ptr->size_y) {
                b_ptr->size_y += 32;
                h -= 32;
                y_add = h;
            }
        }

        // anti-bug (for empty block)
        if ((w == 0) || (h == 0)) {
            fprintf(stderr, "0");
            fflush(stderr);
            b_ptr ++;
            continue;
        }

        // normal block (non-empty)
        tmp_bmp = create_bitmap(w, h);
        if (tmp_bmp == NULL) {
            FATAL_EXIT("dt1_all_zoom_make(%i), can't make a bitmap of %i * %i pixels\n", i, w, h);
        }
        clear(tmp_bmp);

        // draw sub-tiles in this bitmap
        // first we need to fill the block data offset here

        {
            char *p;

            // be careful!!!!!!!!!!!!
            // 60 may vary
            p = 61*b + 60 + (char*)glb_dt1[i].buffer2;

            fprintf(stdout, "to wirte the offset: glb_dt1[i].buff_len2=%lu\n", glb_dt1[i].buff_len2 );
            memcpy(p, &(glb_dt1[i].buff_len2), 4);
        }
        for (s=0; s < b_ptr->tiles_number; s++){
            // for each sub-tiles
            //一个tile由多个subtile构成的
            // get the sub-tile info
            dt1_fill_subt(& st_ptr, i, b_ptr->tiles_ptr, s);
            dt1_fill_subt2(& st_ptr, i, b_ptr->tiles_ptr, s);

            // get infos
            x0     = st_ptr.x_pos;
            y0     = y_add + st_ptr.y_pos;
            data   = (UBYTE *) ((UBYTE *)glb_dt1[i].buffer + b_ptr->tiles_ptr + st_ptr.data_offset);
            length = st_ptr.length;
            format = st_ptr.format;

            //
            {
                char *ptr2 = glb_dt1[i].buffer2;
                ptr2 += glb_dt1[i].buff_len2;
                memcpy(ptr2, data, length);
                glb_dt1[i].buff_len2 += length;

            }
            // draw the sub-tile
            if (format == 0x0001){
                //这个画法我已经知道了
                //参见dt1 tool的解析
                draw_sub_tile_isometric(tmp_bmp, x0, y0, data, length);
            }else{
                draw_sub_tile_normal(tmp_bmp, x0, y0, data, length);
            }
        }

        // if a game's special tile, draw my own info over it
        if ( (glb_ds1edit.cmd_line.no_vis_debug == FALSE) && (i != 0) && ((orientation == 10) || (orientation == 11))) {
            // get info
            t_mi     = b_ptr->main_idx;
            t_si     = b_ptr->sub_idx;
            my_b_ptr = (BLOCK_S *) glb_dt1[0].bh_buffer;

            // search same info in my dt1
            for (my_idx=0; my_idx < glb_dt1[0].block_num; my_idx++) {
                if ( (my_b_ptr[my_idx].orientation == orientation) && (my_b_ptr[my_idx].main_idx  == t_mi) && (my_b_ptr[my_idx].sub_idx   == t_si)) {
                    // found it, draw that tile over the game's gfx
                    sprite = * (glb_dt1[0].block_zoom[ZM_11] + my_idx);
                    if (sprite != NULL){
                        draw_sprite(tmp_bmp, sprite, 0, tmp_bmp->h - sprite->h);
                    }
                    // stop the search
                    my_idx = glb_dt1[0].block_num;
                }
            }
        }



        // make zoom from the bitmap, for each zoom
        dt1_zoom(tmp_bmp, i, b, ZM_11);
        //* (glb_dt1[i].block_zoom[z] + b) = tmp_bmp;//dst;
#if 0
        for (z=0; z<ZM_MAX; z++){
            dt1_zoom(tmp_bmp, i, b, z);
        }
#endif

        // destroy tmp bitmap
        destroy_bitmap(tmp_bmp);

        // next block header
        b_ptr ++;
        fprintf(stderr, ".");
        fflush(stderr);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}

// ==========================================================================
// make all bitmaps in all zoom of all tiles of 1 dt1
//从这个函数来看:
//一个.dt1文件中有个多个block,每个block描述了一个tile或者一个立着的墙体单元
//可能就因为如此才导致不把block直接称为tile...(坑爹的墙体...)
//然后每个block中有多个subtile,也就是那种小方格
//如果该block是tile, 则每个sub-tile会菱形绘制,查表填入
//如果是墙体..则每个sub-tile会跳跃绘制~(程序里面有jump code~)
void dt1_all_zoom_make(int i)
{
    BLOCK_S       * b_ptr, * my_b_ptr; // pointers to current block header
    SUB_TILE_S    st_ptr;  // current sub-tile header
    BITMAP        * tmp_bmp, * sprite;
    int           b, w, h, s, x0, y0, length, y_add, z, mem_size;
    UBYTE         * data;
    WORD          format;
    long          orientation;
    char          tmp_str[100];
    int           t_mi, t_si, my_idx;

    b_ptr    = (BLOCK_S *) glb_dt1[i].bh_buffer;

    fprintf(stderr, "making zoom for glb_dt1[%i] (%s) ", i, glb_dt1[i].name);
    fflush(stderr);
    // get mem for table of pointers
    for (z=0; z<ZM_MAX; z++) {
        //一个block就是一个tile???
        mem_size = sizeof(BITMAP *) * glb_dt1[i].block_num;
        glb_dt1[i].block_zoom[z] = (BITMAP **) malloc(mem_size);
        if (glb_dt1[i].block_zoom[z] == NULL) {
            FATAL_EXIT("dt1_all_zoom_make(%i), zoom %i, not enough mem for %i bytes\n", i, z, mem_size);
        }
        memset(glb_dt1[i].block_zoom[z], 0, mem_size);
        glb_dt1[i].bz_size[z] = mem_size;
    }

    // make the bitmaps
    for (b=0; b < glb_dt1[i].block_num; b++){
        // for each blocks of a dt1
        //从这里来看..貌似一个block就是一个tile
        // get infos
        orientation = b_ptr->orientation;

        // prepare tmp bitmap size
        w = b_ptr->size_x;
        if ((orientation == 10) || (orientation == 11)){
            // 10或者11是special layer
            // set it to 160 because we'll draw infos over it later
            w = 160;
        }
        //这个地方...图片的高直接是个负值...
        h = - b_ptr->size_y;

        // adjustment (which y line in the bitmap is the zero line ?)

        // by default, when orientation > 15 : lower wall
        y_add = 96;
        if ((orientation == 0) || (orientation == 15)){
            // floor or roof
            if (b_ptr->size_y) {
                b_ptr->size_y = - 80;
                h = 80;
                y_add = 0;
            }
        } else if (orientation < 15){
            // upper wall, shadow, special
            if (b_ptr->size_y) {
                b_ptr->size_y += 32;
                h -= 32;
                y_add = h;
            }
        }

        // anti-bug (for empty block)
        if ((w == 0) || (h == 0)) {
            fprintf(stderr, "0");
            fflush(stderr);
            b_ptr ++;
            continue;
        }

        // normal block (non-empty)
        tmp_bmp = create_bitmap(w, h);
        if (tmp_bmp == NULL) {
            FATAL_EXIT("dt1_all_zoom_make(%i), can't make a bitmap of %i * %i pixels\n", i, w, h);
        }
        clear(tmp_bmp);

        // draw sub-tiles in this bitmap
        for (s=0; s < b_ptr->tiles_number; s++){
            // for each sub-tiles
            //一个tile由多个subtile构成的
            // get the sub-tile info
            dt1_fill_subt(& st_ptr, i, b_ptr->tiles_ptr, s);

            // get infos
            x0     = st_ptr.x_pos;
            y0     = y_add + st_ptr.y_pos;
            data   = (UBYTE *) ((UBYTE *)glb_dt1[i].buffer + b_ptr->tiles_ptr + st_ptr.data_offset);
            length = st_ptr.length;
            format = st_ptr.format;

            // draw the sub-tile
            if (format == 0x0001){
                //这个画法我已经知道了
                //参见dt1 tool的解析
                draw_sub_tile_isometric(tmp_bmp, x0, y0, data, length);
            }else{
                draw_sub_tile_normal(tmp_bmp, x0, y0, data, length);
            }
        }

        // if a game's special tile, draw my own info over it
        if ( (glb_ds1edit.cmd_line.no_vis_debug == FALSE) && (i != 0) && ((orientation == 10) || (orientation == 11))) {
            // get info
            t_mi     = b_ptr->main_idx;
            t_si     = b_ptr->sub_idx;
            my_b_ptr = (BLOCK_S *) glb_dt1[0].bh_buffer;

            // search same info in my dt1
            for (my_idx=0; my_idx < glb_dt1[0].block_num; my_idx++) {
                if ( (my_b_ptr[my_idx].orientation == orientation) && (my_b_ptr[my_idx].main_idx  == t_mi) && (my_b_ptr[my_idx].sub_idx   == t_si)) {
                    // found it, draw that tile over the game's gfx
                    sprite = * (glb_dt1[0].block_zoom[ZM_11] + my_idx);
                    if (sprite != NULL){
                        draw_sprite(tmp_bmp, sprite, 0, tmp_bmp->h - sprite->h);
                    }
                    // stop the search
                    my_idx = glb_dt1[0].block_num;
                }
            }
        }

        // make zoom from the bitmap, for each zoom
        dt1_zoom(tmp_bmp, i, b, ZM_11);
        //* (glb_dt1[i].block_zoom[z] + b) = tmp_bmp;//dst;
#if 0
        for (z=0; z<ZM_MAX; z++){
            dt1_zoom(tmp_bmp, i, b, z);
        }
#endif

        // destroy tmp bitmap
        destroy_bitmap(tmp_bmp);

        // next block header
        b_ptr ++;
        fprintf(stderr, ".");
        fflush(stderr);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}


// ==========================================================================
// fill / make all the datas of 1 dt1
void dt1_struct_update2(int i)
{
    char * ptr  = glb_dt1[i].buffer;
    char * ptr2;

    int  size;
    char tmp[100];
    uint32_t  info;

    if (ptr == NULL){
        return;
    }
    if ( i!=2 ){
        return;
    }

    {
        glb_dt1[i].buffer2  = malloc (glb_dt1[i].buff_len);
        if ( glb_dt1[i].buffer2==NULL ) {
            fprintf ( stderr, "\ndynamic memory allocation failed\n" );
            exit (0);
        }

    }

    ptr2 = glb_dt1[i].buffer2;
    //glb_dt1[i].x1        = * (long *) ptr;
    //glb_dt1[i].x2        = * (long *) ((UBYTE *)ptr + 4);
    //glb_dt1[i].block_num = * (long *) ((UBYTE *)ptr + 268);
    memcpy(ptr2, "D2T", 3);
    ptr2 += 3;
    memcpy(ptr2, ptr+268, 4);
    //glb_dt1[i].bh_start  = * (long *) ((UBYTE *)ptr + 272);

    // debug
    //printf("struct data of glb_dt1[%i] :\n", i);
    //printf("name of glb_dt1[%i] : %s\n", i, glb_dt1[i].name);
    //printf("   x1        = %li\n",    glb_dt1[i].x1);
    //printf("   x2        = %li\n",    glb_dt1[i].x2);
    //printf("   block_num = %li\n",    glb_dt1[i].block_num);
    //printf("   bh_start  = 0x%0lX\n", glb_dt1[i].bh_start);

    // blocks
    //size = sizeof(BLOCK_S) * glb_dt1[i].block_num;
    //glb_dt1[i].bh_buffer = (void *) malloc(size);
    //if (glb_dt1[i].bh_buffer == NULL) {
    //    FATAL_EXIT("dt1_struct_update(%i), not enough memory for %i bytes\n", i, size);
    //}
    //glb_dt1[i].bh_buff_len = size;
    //nice~和dt1 tool的源码可以对应
    dt1_bh_update2(i);
    dt1_all_zoom_make2(i);

    {

        {
            FILE    *p;
            char    *p_path = "./test.d2t";

            p = fopen( p_path, "wb" );
            if ( p == NULL ) {
                fprintf ( stderr, "couldn't open file '%s'; %s\n", p_path, strerror(errno) );
                exit (0);
            }

            fprintf(stdout, "glb_dt1[i].buff_len2 = %d\n", glb_dt1[i].buff_len2 );
            fwrite(glb_dt1[i].buffer2, glb_dt1[i].buff_len2, 1, p);
            if( fclose(p) == EOF ) {
                fprintf ( stderr, "couldn't close file '%s'; %s\n", p_path, strerror(errno) );
                exit (0);
            }
            //free(glb_dt1[i].buffer2);
            //fprintf (stdout, "my_dt1:%s\n", glb_dt1[i].buffer2 );
        }
    }
}
// ==========================================================================
// fill / make all the datas of 1 dt1
void dt1_struct_update(int i)
{
    void * ptr  = glb_dt1[i].buffer;
    int  size;
    char tmp[100];
    uint32_t  info;

    if (ptr == NULL){
        return;
    }

    glb_dt1[i].x1        = * (long *) ptr;
    glb_dt1[i].x2        = * (long *) ((UBYTE *)ptr + 4);
    glb_dt1[i].block_num = * (long *) ((UBYTE *)ptr + 268);
    glb_dt1[i].bh_start  = * (long *) ((UBYTE *)ptr + 272);

    // debug
    printf("struct data of glb_dt1[%i] :\n", i);
    printf("name of glb_dt1[%i] : %s\n", i, glb_dt1[i].name);
    printf("   x1        = %li\n",    glb_dt1[i].x1);
    printf("   x2        = %li\n",    glb_dt1[i].x2);
    printf("   block_num = %li\n",    glb_dt1[i].block_num);
    printf("   bh_start  = 0x%0lX\n", glb_dt1[i].bh_start);

    // blocks
    size = sizeof(BLOCK_S) * glb_dt1[i].block_num;
    glb_dt1[i].bh_buffer = (void *) malloc(size);
    if (glb_dt1[i].bh_buffer == NULL) {
        FATAL_EXIT("dt1_struct_update(%i), not enough memory for %i bytes\n", i, size);
    }
    glb_dt1[i].bh_buff_len = size;
    //nice~和dt1 tool的源码可以对应
    dt1_bh_update(i);
    dt1_all_zoom_make(i);
}


// ==========================================================================
// ask to load a dt1 from a mpq
// if the dt1 was already loaded by another ds1, just use the same dt1
//    (and update the usage counter)
// else load it
int dt1_add(char * dt1name)
{
    int  i, idx, entry;
    char tmp[256];


    if (dt1_already_loaded(dt1name, & idx) == TRUE) {
        glb_dt1[idx].ds1_usage++;
        return idx;
    } else {
        // search the first available dt1
        for (i=0; i<DT1_MAX; i++) {
            if (glb_dt1[i].ds1_usage == 0) {
                // this one is emtpy, now load it
                entry = misc_load_mpq_file(
                        dt1name,
                        (char **) & glb_dt1[i].buffer,
                        & glb_dt1[i].buff_len,
                        TRUE
                        );
                if (entry == -1) {
                    FATAL_EXIT("dt1_add() : file %s not found", dt1name);
                }



                // dt1 update
                strcpy(glb_dt1[i].name, dt1name);
                glb_dt1[i].ds1_usage = 1;
                //把第i个.dt1填入数据结构中~


                if( i != 2 ){
                    dt1_struct_update(i);
                }else{
                    dt1_test_gfx(i);
                }
                //dt1_struct_update(i);  // show all
                //dt1_struct_update2(i);// make test.d2t
                //dt1_test_gfx(i);

                // free the copy of the dt1 in mem
                free(glb_dt1[i].buffer);
                glb_dt1[i].buffer = NULL;
                glb_dt1[i].buff_len = 0;

                // tell to the ds1 which idx in the glb_dt1[] have been choosen
                return i;
            }
        }
        FATAL_EXIT("can not load %s from mpq,\ntoo much dt1 already loaded (%i)", dt1name, DT1_MAX);
        return -1; // useless, just for not having a vc6 warning
    }
}


// ==========================================================================
// to load ds1edit.dt1 from the data\ directory (not the mod_dir directory)
int dt1_add_special(char * dt1name)
{
    //这里读取的是ds1edit.dt1 中间全是那种特殊块..一点意思都没有
    //不是一般的dt1...通过dt1 tool 就可以看到

    int  i, idx, entry;
    char tmp[256];

    if (dt1_already_loaded(dt1name, & idx) == TRUE) {
        glb_dt1[idx].ds1_usage++;
        return idx;
    } else {
        // search the first available dt1
        for (i=0; i<DT1_MAX; i++) {
            if (glb_dt1[i].ds1_usage == 0) {
                // load from ds1edit directory
                entry = mod_load_in_mem(".", dt1name, & glb_dt1[i].buffer, & glb_dt1[i].buff_len);
                if (entry == -1) {
                    FATAL_EXIT("(dt1_add_special) file %s not found", dt1name);
                } else {
                    printf(", found\n");
                }

                // dt1 update
                strcpy(glb_dt1[i].name, dt1name);
                glb_dt1[i].ds1_usage = 1;
                dt1_struct_update(i);

                // free the copy of the dt1 in mem
                free(glb_dt1[i].buffer);
                glb_dt1[i].buffer = NULL;
                glb_dt1[i].buff_len = 0;

                // tell to the ds1 which idx in the glb_dt1[] have been choosen
                return i;
            }
        }
        FATAL_EXIT("can not load %s from mpq,\ntoo much dt1 already loaded (%i)", dt1name, DT1_MAX);
        return -1; // useless, just for not having a vc6 warning
    }
}
