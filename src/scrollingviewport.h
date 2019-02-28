/*
 * Copyright (c) Sebastian Krzyszkowiak <dos@dosowisko.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef ANIMATCH_SCROLLINGVIEWPORT_H
#define ANIMATCH_SCROLLINGVIEWPORT_H

#include "common.h"

struct ScrollingViewport {
	double pos;
	double speed;
	double offset;
	bool pressed, triggered;
	double lasttime;
	double lasty;

	int x, y, w, h;
	int content;
};

void SetScrollingViewportPosition(struct Game* game, struct ScrollingViewport* viewport, int x, int y, int w, int h, int content);
void SetScrollingViewportAsTarget(struct Game* game, struct ScrollingViewport* viewport);
void UpdateScrollingViewport(struct Game* game, struct ScrollingViewport* viewport);
void ProcessScrollingViewportEvent(struct Game* game, ALLEGRO_EVENT* ev, struct ScrollingViewport* viewport);

#endif
