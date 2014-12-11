/*
 * Copyright (C) 2011 Giorgio Vazzana
 *
 * This file is part of hwebcam.
 *
 * hwebcam is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * hwebcam is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include "common.h"

int  newline;
char msgbuf[MBS];

void need_newline()
{
	if (newline == 0) {
		fprintf(stderr, "\n");
		newline = 1;
	}
}

void print_msg_nl(char *msg)
{
	need_newline();
	if (msg)
		fprintf(stderr, "%s", msg);
}

void die(char *msg, int exit_code)
{
	print_msg_nl(msg);
	exit(exit_code);
}
