/*	$OpenBSD: scores.c,v 1.11 2006/04/20 03:25:36 ray Exp $	*/
/*	$NetBSD: scores.c,v 1.2 1995/04/22 07:42:38 cgd Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek and Darren F. Provine.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)scores.c	8.1 (Berkeley) 5/31/93
 */

/** @addtogroup tetris
 * @{
 */
/** @file
 */

/*
 * Score code for Tetris, by Darren Provine (kilroy@gboro.glassboro.edu)
 * modified 22 January 1992, to limit the number of entries any one
 * person has.
 *
 * Major whacks since then.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <io/console.h>
#include <io/keycode.h>
#include <vfs/vfs.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>
#include <time.h>

#include "screen.h"
#include "tetris.h"
#include "scores.h"

/*
 * Within this code, we can hang onto one extra "high score", leaving
 * room for our current score (whether or not it is high).
 *
 * We also sometimes keep tabs on the "highest" score on each level.
 * As long as the scores are kept sorted, this is simply the first one at
 * that level.
 */

#define NUMSPOTS  (MAXHISCORES + 1)
#define NLEVELS   (MAXLEVEL + 1)

static struct highscore scores[NUMSPOTS];

/** Copy from hiscore table score with index src to dest
 *
 */
static void copyhiscore(int dest, int src)
{
	str_cpy(scores[dest].hs_name, STR_BOUNDS(MAXLOGNAME) + 1,
	    scores[src].hs_name);
	scores[dest].hs_score = scores[src].hs_score;
	scores[dest].hs_level = scores[src].hs_level;
}

void showscores(int firstgame)
{
	int i;
	
	clear_screen();
	moveto(10, 0);
	printf("\tRank \tLevel \tName\t                     points\n");
	printf("\t========================================================\n");
	
	for (i = 0; i < NUMSPOTS - 1; i++)
		printf("\t%6d %6d %-16s %20d\n",
		    i + 1, scores[i].hs_level, scores[i].hs_name, scores[i].hs_score);
	
	if (!firstgame) {
		printf("\t========================================================\n");
		printf("\t  Last %6d %-16s %20d\n",
		    scores[NUMSPOTS - 1].hs_level, scores[NUMSPOTS - 1].hs_name, scores[NUMSPOTS - 1].hs_score);
	}
	
	printf("\n\n\n\n\tPress any key to return to main menu.");
	getchar();
}

void insertscore(int score, int level)
{
	int i;
	int j;
	size_t off;
	console_event_t ev;
	
	clear_screen();
	moveto(10, 10);
	puts("Insert your name: ");
	str_cpy(scores[NUMSPOTS - 1].hs_name, STR_BOUNDS(MAXLOGNAME) + 1,
	    "Player");
	i = 6;
	off = 6;
	
	moveto(10 , 28);
	printf("%s%.*s", scores[NUMSPOTS - 1].hs_name, MAXLOGNAME-i,
	    "........................................");
	
	while (1) {
		fflush(stdout);
		if (!console_get_event(fphone(stdin), &ev))
			exit(1);
		
		if (ev.type == KEY_RELEASE)
			continue;
		
		if (ev.key == KC_ENTER || ev.key == KC_NENTER)
			break;
		
		if (ev.key == KC_BACKSPACE) {
			if (i > 0) {
				wchar_t uc;
				
				--i;
				while (off > 0) {
					--off;
					size_t otmp = off;
					uc = str_decode(scores[NUMSPOTS - 1].hs_name,
					    &otmp, STR_BOUNDS(MAXLOGNAME) + 1);
					if (uc != U_SPECIAL)
						break;
				}
				
				scores[NUMSPOTS - 1].hs_name[off] = '\0';
			}
		} else if (ev.c != '\0') {
			if (i < (MAXLOGNAME - 1)) {
				if (chr_encode(ev.c, scores[NUMSPOTS - 1].hs_name,
				    &off, STR_BOUNDS(MAXLOGNAME) + 1) == EOK) {
					++i;
				}
				scores[NUMSPOTS - 1].hs_name[off] = '\0';
			}
		}
		
		moveto(10, 28);
		printf("%s%.*s", scores[NUMSPOTS - 1].hs_name, MAXLOGNAME - i,
		    "........................................");
	}
	
	scores[NUMSPOTS - 1].hs_score = score;
	scores[NUMSPOTS - 1].hs_level = level;
	
	i = NUMSPOTS - 1;
	while ((i > 0) && (scores[i - 1].hs_score < score))
		i--;
	
	for (j = NUMSPOTS - 2; j > i; j--)
		copyhiscore(j, j-1);
	
	copyhiscore(i, NUMSPOTS - 1);
}

void initscores(void)
{
	int i;
	for (i = 0; i < NUMSPOTS; i++) {
		str_cpy(scores[i].hs_name, STR_BOUNDS(MAXLOGNAME) + 1, "HelenOS Team");
		scores[i].hs_score = (NUMSPOTS - i) * 200;
		scores[i].hs_level = (i + 1 > MAXLEVEL ? MAXLEVEL : i + 1);
	}
}

/** @}
 */
