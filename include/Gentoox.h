/**
 * Gentoox loader includes for LoadLinux.c
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

void errorLED(void) {
   I2cSetFrontpanelLed(I2C_LED_RED0);
}

void busyLED(void) {
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_RED2 | I2C_LED_GREEN3);	
}

void inputLED(void) {
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_GREEN0 | I2C_LED_GREEN2 | I2C_LED_RED2);	
}

void goodLED(void) {
	I2cSetFrontpanelLed(I2C_LED_GREEN0 | I2C_LED_GREEN2);	
}

void importantLED(void) {
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_GREEN1 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_GREEN3 | I2C_LED_RED3);	
}

void downloadingLED(void) {
	I2cSetFrontpanelLed(I2C_LED_GREEN0 | I2C_LED_RED0 | I2C_LED_GREEN1 | I2C_LED_RED2 | I2C_LED_RED3);	
}


void uberLED(void) {
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED2);	
}

void highLED(void) {
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3);	
}

void midLED(void) {
	I2cSetFrontpanelLed(I2C_LED_GREEN0 | I2C_LED_RED0 | I2C_LED_GREEN1 | I2C_LED_RED1 | I2C_LED_GREEN2 | I2C_LED_RED2);	
}

void lowLED(void) {
	I2cSetFrontpanelLed(I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3);	
}

void dots(void) {
   wait_ms(333);
   printk(".");
   wait_ms(333);
   printk(".");
   wait_ms(333);
   printk(".");
   wait_ms(333);
}

void cromwellError(void) {
   VIDEO_ATTR=0xffd8d8d8;
   printk("\t[ ");
   VIDEO_ATTR=0xffff0000;
   printk("!!");
   VIDEO_ATTR=0xffd8d8d8;
   printk(" ]");
	errorLED();
}

void cromwellWarning(void) {
   VIDEO_ATTR=0xffd8d8d8;
   printk("\t[ ");
   VIDEO_ATTR=0xffffae01;
   printk("!!");
   VIDEO_ATTR=0xffd8d8d8;
   printk(" ]\n");
	errorLED();
}

void cromwellSuccess(void) {
   VIDEO_ATTR=0xffd8d8d8;
   printk("\t[ ");
   VIDEO_ATTR=0xff00ff00;
   printk("ok");
   VIDEO_ATTR=0xffd8d8d8;
   printk(" ]\n");
}
