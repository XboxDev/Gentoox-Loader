/**
 * Xbox LED Pattern setting for Cromwell.
 * Copyright (C) Thomas "ShALLaX" Pedley (gentoox@shallax.com)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

int strlen(const char * s);

// Set the pattern of the LED.
// The pattern must be 4 characters long and must consist
// only of 'r', 'g', 'o' and 'x'.
//
// r = Red
// g = Green
// o = Orange
// x = Off
// 
// E.g. rgog will cycle through red, green, orange, green and then loop.
//
// If the pattern string isn't 4 characters in length, nothing is done.
void setLED(char *pattern) {
	char *x = pattern;
	int r, g;
	
	if(strlen(pattern) == 4) {
		r = g = 0;
		while (*x) {
			r *= 2;
			g *= 2;
			switch (*x) {
				case 'r':
					r++; 
					break;
				case 'g':
					g++;
					break;
				case 'o':
					r++;
					g++;
					break;
			}
			x++;
		}
		I2cSetFrontpanelLed(((r<<4) & 0xF0) + (g & 0xF));
	}
}
