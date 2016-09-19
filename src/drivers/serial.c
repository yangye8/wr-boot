/*
 * (C) Copyright 2000
 * Rob Taylor, Flying Pig Systems. robt@flyingpig.com.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "ns16550.h"

static NS16550_t console = 0;

int serial_init ( NS16550_t base, unsigned int clk)
{

	int clock_divisor = clk / 16 / 115200;

	console = base;

	NS16550_init(console, clock_divisor);

	return (0);
}

void
serial_putc(const char c)
{
	if (c == '\n')
		NS16550_putc(console, '\r');

	NS16550_putc(console, c);
}

void
serial_puts (const char *s)
{
	while (*s) {
		serial_putc (*s++);
	}
}


int
serial_getc(void)
{
	return NS16550_getc(console);
}

int
serial_tstc(void)
{
	return NS16550_tstc(console);
}

void
serial_setbrg (unsigned int clk)
{
	int clock_divisor = clk / 16 / 115200;

	NS16550_reinit(console, clock_divisor);
}

int getc (void)
{
    /* Send directly to the handler */
    return serial_getc (); 
}

int tstc (void)
{
    /* Send directly to the handler */
    return serial_tstc (); 
}

void putc (const char c)
{
        /* Send directly to the handler */
        serial_putc (c);
}

void puts (const char *s) 
{
        /* Send directly to the handler */
        serial_puts (s);
}


