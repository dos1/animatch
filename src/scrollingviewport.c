/*! \file common.c
 *  \brief Common stuff that can be used by all gamestates.
 */
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

#include "scrollingviewport.h"

void SetScrollingViewportPosition(struct Game* game, struct ScrollingViewport* viewport, int x, int y, int w, int h, int content) {
	viewport->x = x;
	viewport->y = y;
	viewport->w = w;
	viewport->h = h;
	viewport->content = content;
}

void SetScrollingViewportAsTarget(struct Game* game, struct ScrollingViewport* viewport) {
	if (viewport) {
		SetClippingRectangle(viewport->x, viewport->y, viewport->w, viewport->h);
		ALLEGRO_TRANSFORM transform;
		al_identity_transform(&transform);
		al_translate_transform(&transform, viewport->x, viewport->y - viewport->pos);
		PushTransform(game, &transform);
	} else {
		PopTransform(game);
		ResetClippingRectangle();
	}
}

void UpdateScrollingViewport(struct Game* game, struct ScrollingViewport* viewport) {
	if (!viewport->pressed) {
		viewport->speed *= 0.95;
		viewport->pos += viewport->speed * game->viewport.height;

		if (viewport->pos < 0) {
			viewport->pos *= 0.75;
			viewport->speed *= 0.2;
		}
		if (viewport->pos > viewport->content - viewport->h) {
			double diff = viewport->pos - (viewport->content - viewport->h);
			diff *= 0.25;
			viewport->pos -= diff;
			viewport->speed *= 0.2;
		}
	}
}

void ProcessScrollingViewportEvent(struct Game* game, ALLEGRO_EVENT* ev, struct ScrollingViewport* viewport) {
	if ((ev->type == ALLEGRO_EVENT_TOUCH_BEGIN) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)) {
		if ((game->data->mouseX * game->viewport.width >= viewport->x) && (game->data->mouseX * game->viewport.width <= viewport->x + viewport->w) && (game->data->mouseY * game->viewport.height >= viewport->y) && (game->data->mouseY * game->viewport.height <= viewport->y + viewport->h)) {
			viewport->offset = 0;
			viewport->speed = 0;
			viewport->pressed = true;
		}
	}
	if ((ev->type == ALLEGRO_EVENT_TOUCH_END) || (ev->type == ALLEGRO_EVENT_TOUCH_CANCEL) || (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)) {
		viewport->pressed = false;
		viewport->triggered = false;
		if (fabs(viewport->speed) < 0.0015) {
			viewport->speed = 0;
		}
	}
	if ((ev->type == ALLEGRO_EVENT_TOUCH_MOVE) || (ev->type == ALLEGRO_EVENT_MOUSE_AXES)) {
		if (ev->type == ALLEGRO_EVENT_TOUCH_MOVE) {
			viewport->offset += ev->touch.dy / (float)game->viewport.height;
		} else {
			viewport->offset += ev->mouse.dy / (float)game->viewport.height;
		}
		if (viewport->pressed && !viewport->triggered && fabs(viewport->offset) > 0.01) {
			double timestamp;
			if (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN) {
				timestamp = ev->touch.timestamp;
			} else {
				timestamp = ev->mouse.timestamp;
			}
			viewport->triggered = true;
			viewport->lasty = game->data->mouseY;
			viewport->lasttime = timestamp;
		}

		if (viewport->triggered) {
			double timestamp;
			if (ev->type == ALLEGRO_EVENT_TOUCH_MOVE) {
				timestamp = ev->touch.timestamp;
			} else {
				timestamp = ev->mouse.timestamp;
			}

			if (timestamp - viewport->lasttime) {
				viewport->speed = (viewport->lasty - game->data->mouseY) / ((timestamp - viewport->lasttime) * 1000) * 16.666;
			}

			viewport->pos += (viewport->lasty - game->data->mouseY) * game->viewport.height;

			viewport->lasty = game->data->mouseY;
			viewport->lasttime = timestamp;
		}
	}
	if (ev->type == ALLEGRO_EVENT_MOUSE_AXES) {
		viewport->pos -= ev->mouse.dz * 16;
	}
}
