/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "IPMenuActions.h"
#include "TextMenu.h"

static int A = NB_BLOCK_A, B = NB_BLOCK_B, C = NB_BLOCK_C, D = NB_BLOCK_D, P = NB_PORT;

void enableHttpc(void *whatever) {
	extern unsigned char *videosavepage;
	memcpy((void*)FB_START,videosavepage,FB_SIZE);
	VIDEO_ATTR=0xffef37;
	printk("\n\n\n\n\n\n");
	VIDEO_ATTR=0xffc8c8c8;
	initialiseNetwork();
	netBoot(A, B, C, D, P);
}

void incrementNumberA(void *num) {
	char *text = (char *)num;
	int n = simple_strtol(num+4, NULL, 10);
	n++;
	if(n > 255) {
		n = 0;
	}
	A = n;
	sprintf(text, "%s %i", "A: ", n);
}

void incrementNumberB(void *num) {
	char *text = (char *)num;
	int n = simple_strtol(num+4, NULL, 10);
	n++;
	if(n > 255) {
		n = 0;
	}
	B = n;
	sprintf(text, "%s %i", "B: ", n);
}

void incrementNumberC(void *num) {
	char *text = (char *)num;
	int n = simple_strtol(num+4, NULL, 10);
	n++;
	if(n > 255) {
		n = 0;
	}
	C = n;
	sprintf(text, "%s %i", "C: ", n);
}

void incrementNumberD(void *num) {
	char *text = (char *)num;
	int n = simple_strtol(num+4, NULL, 10);
	n++;
	if(n > 255) {
		n = 1;
	}
	D = n;
	sprintf(text, "%s %i", "D: ", n);
}

void incrementNumberP(void *num) {
	char *text = (char *)num;
	int n = simple_strtol(num+4, NULL, 10);
	n++;
	if(n > 65535) {
		n = 1;
	}
	P = n;
	sprintf(text, "%s %i", "P: ", n);
}
