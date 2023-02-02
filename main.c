
#define EXTERN
#include "xchomp.h"

/* 25 frames per seconds */
#define TARGET_LOOP_DURATION_SECONDS 0.0333

main(argc, argv)
int argc;
char *argv[];
{
   XEvent		event;
   int			dummy;
   XCharStruct		chars;
   unsigned long	event_mask;

   struct timespec start, end, loop_delay;
   long loop_duration;

   loop_delay.tv_sec = 0;
   
   /* open the display */
   display 	= XOpenDisplay(NULL);
   screen 	= DefaultScreen(display);
   root		= DefaultRootWindow(display);
   depth 	= DefaultDepth(display, screen);
   black 	= BlackPixel(display, screen);
   white 	= WhitePixel(display, screen);

   /* get a font */
   font = XLoadFont(display, "fixed");
   XQueryTextExtents(display, font, "000000", 6, &dummy,
      &ascent, &descent, &chars);

   /*
    * We want to suspend the game in case the window is iconified.
    * This is more difficult than it sounds.  On the Sun, iconification
    * seems to produce an UnmapNotify event -- very nice.  DECwindows,
    * however, informs the application by generating a PropertyNotify
    * event on a DEC-specific property -- very nasty.  The atom is
    * not defined in any of the DECwindows headers, so we will try
    * to get its value from the server, and use it later.  We are
    * hoping here that all non-DECwindows servers will return None
    * for this atom.
    */
   DEC_icon_atom = XInternAtom(display, "DEC_WM_ICON_STATE", True);

   /* assemble resources */
   create_ghost();
   create_pac();
   create_fruit();
   create_maze_symbols();
   create_demo_images();
   create_window(argc, argv);
   create_GCs();
   create_maps();

   /* select the event mask for the window */
   event_mask = ExposureMask | KeyPressMask;
   if (DEC_icon_atom == None)
      event_mask |= StructureNotifyMask;
   else event_mask |= PropertyChangeMask;
   XSelectInput(display, window, event_mask);
   
   /* display the window */
   XMapWindow(display, window);
   while (True) {
      XNextEvent(display, &event);
      if (event.xany.window != window) continue;
      if (event.type == Expose) break;
   }

   /*-- The Game Starts Here -----------------------------------------*/
   {
	register int	i, num_clips;
	char		c_buf;
	XComposeStatus  status;

	static int	flash_ticks[] = {
			   13, 8, 4, 1, 13, 8, 4, 1,
			   8, 4, 1, 4, 1, 8, 4, 1,
			   0, 0, 8, 4, 0, 0, 1, 0 };
	static int	off_ticks[] = {
			   19, 14, 10, 7, 19, 14, 10, 7,
			   14, 10, 7, 10, 7, 14, 10, 7,
			   1, 1, 14, 10, 1, 1, 7, 1 };
	static int	screens[] = {
			   1, 1, 1, 1, 2, 2, 2, 2,
			   3, 3, 3, 4, 4, 5, 5, 5,
			   1, 2, 6, 6, 3, 4, 6, 5 };

	high_score = 0L;

     demo:

	/* run the demo screen */
	demo_seq();

	/* initialize the game */
	lives = 3;
	level = (-1);
	score = 0L;
	print_score(0L);

     new_screen:

	/* advance the level */
	plevel = (++level > 13) ? 13 : level;
	flash_tick = flash_ticks[level % 24];
	off_tick = off_ticks[level % 24];
	display_level(True);

	/* initialize dynamic parameters */
	completed = False;
	fruit_times = 0;

	/* build the maze */
	clear_maps();
        read_maze(screens[level % 24] - 1);

     new_life:

	/* initialize more dynamic parameters */
	last_key = XK_Left;
	dead = False;
	eat_mode = False;
	count = (-1);
	fruit_count = (-1);
	fruit_shown = False;
	position_players();

	/* display the number of lives */
	(void)set_lives(lives);

	/* copy the maze to the map and the screen */
	XCopyArea(display, save, map, fullcopyGC, 0, 0,
	   WIN_WIDTH, WIN_HEIGHT, 0, 0);
	XCopyArea(display, map, window, fullcopyGC, 0, 0,
	   WIN_WIDTH, WIN_HEIGHT, 0, 0);

	/* display the ready message */
	get_ready();


	/*-- The Animation Loop ----------------------------------------*/

	while (True) {

	  clock_gettime(CLOCK_MONOTONIC, &start);
	  
	   /*-- Xlib Event Section -------------------------------------*/

	   while (QLength(display) > 0) {
	      XNextEvent(display, &event);
	      if (event.xany.window != window) continue;
	      switch (event.type) {
		 case KeyPress:
		    XLookupString(&event, &c_buf, 1, &last_key, &status);
		    if (last_key == XK_space)
		       if (!pause_seq())
			  goto demo;
		    break;
		 case UnmapNotify:
		    while (True) {
		       XNextEvent(display, &event);
		       if (event.xany.window != window) continue;
		       if (event.type == MapNotify) break;
		    }
		    XCopyArea(display, map, window, fullcopyGC, 0, 0,
		       WIN_WIDTH, WIN_HEIGHT, 0, 0);
		    restore_status();
		    if (!pause_seq())
		       goto demo;
		    break;
		 case PropertyNotify:
		    if (event.xproperty.atom != DEC_icon_atom) break;
		    while (True) {
		       XNextEvent(display, &event);
		       if (event.xany.window != window) continue;
		       if (event.type != PropertyNotify) continue;
		       if (event.xproperty.atom == DEC_icon_atom) break;
		    }
		    XCopyArea(display, map, window, fullcopyGC, 0, 0,
		       WIN_WIDTH, WIN_HEIGHT, 0, 0);
		    restore_status();
		    if (!pause_seq())
		       goto demo;
		    break;
		 case Expose:
		    XCopyArea(display, map, window, fullcopyGC, 0, 0,
		       WIN_WIDTH, WIN_HEIGHT, 0, 0);
		    restore_status();
		    break;
		 default: break;
	      }
	   }

	   /*-- Adjust Frame Counter -----------------------------------*/

	   count = (count + 1) & 0x0f;

	   /*-- Flashing Power-Dot And Fruit Section -------------------*/

	   num_clips = NUM_FIGURES;

	   if (count == 0) {

	      /* it's time to flash the power-dots */
	      XCopyArea(display, powermap, save, powerGC, 0, 0,
	         WIN_WIDTH, WIN_HEIGHT, 0, 0);
	      num_clips = NUM_FIGURES + MAX_POWER_DOTS;

	      /* see if it's time to display or erase the fruit */
	      if (fruit_times < 2) {
	         if (++fruit_count == 30) {
		    XCopyPlane(display, fruit_pix[plevel], save, fullcopyGC,
		       0, 0, GHOST_SIZE, GHOST_SIZE, fruit_x, fruit_y, 1);
                    num_clips = NUM_FIGURES + MAX_POWER_DOTS + 1;
		    dd[fruit_y >> 4][fruit_x >> 4] = 'F';
		    fruit_shown = True;
	         }
	         else if (fruit_count == 50) {
		    XFillRectangle(display, save, clearGC, fruit_x - 2,
		       fruit_y, FRUIT_WIDTH, FRUIT_HEIGHT);
		    dd[fruit_y >> 4][fruit_x >> 4] = '\0';
	  	    fruit_count = 0;
		    ++fruit_times;
                    num_clips = NUM_FIGURES + MAX_POWER_DOTS + 1;
		    fruit_shown = False;
	         }
	      }
	   }

	   /*-- Set Clipping Information -------------------------------*/

	   XSetClipRectangles(display, copyGC, 0, 0, rectangle,
	      num_clips, Unsorted);

	   /*-- Restore Background Image -------------------------------*/

	   XCopyArea(display, save, map, copyGC, 0, 0, WIN_WIDTH,
	      WIN_HEIGHT, 0, 0);

	   /*-- Motion Control Section ---------------------------------*/

	   control_pac();
	   if (dead || completed) break;
	   for (i = 0; i < PAC_SLOT; i++)
	      if (!(x[i] & 0x0f) && !(y[i] & 0x0f))
		 (*drive[i])(i);
	   for (i = 0; i < PAC_SLOT; i++) {
	      rectangle[i].x = (x[i] += ix[i]) - 2;
	      rectangle[i].y = (y[i] += iy[i]) - 2;
	   }

	   /*-- Flashing Ghost Section ---------------------------------*/

	   /*
	    * If we're in the middle of a ghost-eating period, this section
	    * handles the timing and changes ghost states when necessary
	    */
	   if (eat_mode)
	      if (count == count_sync) {
	         ++grey_tick;
	         if (grey_tick == flash_tick) {
		    for (i = 0; i < PAC_SLOT; i++)
		       if (ghost[i] == gghost)
		          ghost[i] = fghost;
		 }
	         else if (grey_tick == off_tick) {
		    eat_mode = False;
		    for (i = 0; i < PAC_SLOT; i++)
		       if (drive[i] == run) {
		          ghost[i] = bghost;
		          contact[i] = die;
		          drive[i] = follow;
		          x[i] &= ~0x1;  y[i] &= ~0x01;
		          ix[i] *= 2;    iy[i] *= 2;
		       }
		       else if (drive[i] == hover2) {
		          ghost[i] = bghost;
		          contact[i] = die;
			  drive[i] = hover;
		          x[i] &= ~0x1;  y[i] &= ~0x01;
		          ix[i] *= 2;    iy[i] *= 2;
		       }
	         }
	      }

	   /*-- Offscreen Figure Overlay -------------------------------*/

	   for (i = 0; i < PAC_SLOT; i++)
	      XCopyPlane(display, ghost[i][count], map, orGC, 0, 0, 
		 GHOST_SIZE, GHOST_SIZE, x[i], y[i], 1);
	   XCopyPlane(display, pac[count], map, orGC, 0, 0,
	      GHOST_SIZE, GHOST_SIZE, x[PAC_SLOT], y[PAC_SLOT], 1);

	   /*-- Screen Update ------------------------------------------*/

	   XCopyArea(display, map, window, copyGC, 0, 0, WIN_WIDTH,
	      WIN_HEIGHT, 0, 0);

	   /*-- Synchronization And Delay ------------------------------*/
	   
	   XSync(display, False);

	   clock_gettime(CLOCK_MONOTONIC,&end);

	   loop_duration =
	     (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);

	   if (loop_duration < TARGET_LOOP_DURATION_SECONDS * 1e9) {
	     loop_delay.tv_nsec =
	       (long)(TARGET_LOOP_DURATION_SECONDS * 1e9 - loop_duration);
	     nanosleep(&loop_delay, NULL);
	   }
	   
	} /* while */

	/*-- End of Animation Loop -------------------------------------*/

	if (dead) {
	   if (set_lives(lives - 1)) {
	      sleep(2);
	      goto new_life;
	   }
	   game_over();
	   goto demo;
	}

	if (completed) {
	   finish();
	   goto new_screen;
	}
   }
   /*-- The Game Ends Here -------------------------------------------*/

   do_exit();
}


do_exit()
{
   XUnmapWindow(display, window);
   XUnloadFont(display, font);
   XFlush(display);
   XCloseDisplay(display);
   exit(1);
}
