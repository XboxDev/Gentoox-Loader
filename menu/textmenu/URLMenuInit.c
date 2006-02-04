/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "include/config.h"
#include "TextMenu.h"

#include "VideoInitialization.h"
#include "URLMenuActions.h"

TEXTMENU *URLMenuInit(void) {
	
	TEXTMENUITEM *itemPtr;
	TEXTMENU *menuPtr;

	extern char url[200];
	memset(url, 0, 200);

	//Create the root menu - MANDATORY
	menuPtr = (TEXTMENU*)malloc(sizeof(TEXTMENU));
   memset(menuPtr,0x00,sizeof(TEXTMENU));
	menuPtr->timeout = 1;
	menuPtr->longTitle = 1;
	menuPtr->visibleCount = 3;
	strcpy(menuPtr->szCaption, "/sourceforge/xbox-linux/resctoox.t00x");
	menuPtr->firstMenuItem=NULL;

	strcpy(url, menuPtr->szCaption);

	// Connect
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	strcpy(itemPtr->szCaption, "Connect");
	itemPtr->functionPtr= enableHttpc;
	itemPtr->functionDataPtr = NULL ;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionLeftPtr=decrementAlphabet;
	itemPtr->functionLeftDataPtr = menuPtr->szCaption;
	itemPtr->functionRightPtr=incrementAlphabet;
	itemPtr->functionRightDataPtr = menuPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);
	
	// New letter
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	sprintf(itemPtr->szCaption, "->");
	itemPtr->functionPtr=nextLetter;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionLeftPtr=decrementAlphabet;
	itemPtr->functionLeftDataPtr = menuPtr->szCaption;
	itemPtr->functionRightPtr=incrementAlphabet;
	itemPtr->functionRightDataPtr = menuPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);

	// Delete
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	sprintf(itemPtr->szCaption, "<-");
	itemPtr->functionPtr=deleteLetter;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionLeftPtr=decrementAlphabet;
	itemPtr->functionLeftDataPtr = menuPtr->szCaption;
	itemPtr->functionRightPtr=incrementAlphabet;
	itemPtr->functionRightDataPtr = menuPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);

	// Uppercase
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	sprintf(itemPtr->szCaption, "Uppercase");
	itemPtr->functionPtr=setUC;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionLeftPtr=decrementAlphabet;
	itemPtr->functionLeftDataPtr = menuPtr->szCaption;
	itemPtr->functionRightPtr=incrementAlphabet;
	itemPtr->functionRightDataPtr = menuPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);

	// Lowercase
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	sprintf(itemPtr->szCaption, "Lowercase");
	itemPtr->functionPtr=setLC;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionLeftPtr=decrementAlphabet;
	itemPtr->functionLeftDataPtr = menuPtr->szCaption;
	itemPtr->functionRightPtr=incrementAlphabet;
	itemPtr->functionRightDataPtr = menuPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);

	// Number
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	sprintf(itemPtr->szCaption, "Numbers");
	itemPtr->functionPtr=setNum;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionLeftPtr=decrementAlphabet;
	itemPtr->functionLeftDataPtr = menuPtr->szCaption;
	itemPtr->functionRightPtr=incrementAlphabet;
	itemPtr->functionRightDataPtr = menuPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);

	// Fullstop (.)
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	sprintf(itemPtr->szCaption, "Fullstop (.)");
	itemPtr->functionPtr=setFullStop;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionLeftPtr=decrementAlphabet;
	itemPtr->functionLeftDataPtr = menuPtr->szCaption;
	itemPtr->functionRightPtr=incrementAlphabet;
	itemPtr->functionRightDataPtr = menuPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);

	// Forward slash (/)
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	sprintf(itemPtr->szCaption, "Forward slash (/)");
	itemPtr->functionPtr=setFSlash;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionLeftPtr=decrementAlphabet;
	itemPtr->functionLeftDataPtr = menuPtr->szCaption;
	itemPtr->functionRightPtr=incrementAlphabet;
	itemPtr->functionRightDataPtr = menuPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);

	// Dash (-)
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	sprintf(itemPtr->szCaption, "Dash (-)");
	itemPtr->functionPtr=setDash;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionLeftPtr=decrementAlphabet;
	itemPtr->functionLeftDataPtr = menuPtr->szCaption;
	itemPtr->functionRightPtr=incrementAlphabet;
	itemPtr->functionRightDataPtr = menuPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);

	// Underscore (_)
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	sprintf(itemPtr->szCaption, "Underscore (_)");
	itemPtr->functionPtr=setUScore;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionDataPtr = menuPtr->szCaption;
	itemPtr->functionLeftPtr=decrementAlphabet;
	itemPtr->functionLeftDataPtr = menuPtr->szCaption;
	itemPtr->functionRightPtr=incrementAlphabet;
	itemPtr->functionRightDataPtr = menuPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);

	return menuPtr;
}
