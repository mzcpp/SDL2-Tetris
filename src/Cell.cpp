#include "Cell.hpp"
#include "Game.hpp"

#include <SDL2/SDL.h>

Cell::Cell(Game* game) : game_(game), occupied_(false)
{
	rect_.x = 0;
	rect_.y = 0;
	rect_.w = 0;
	rect_.h = 0;

	color_.r = 0x00;
	color_.g = 0x00;
	color_.b = 0x00;
	color_.a = 0xff;
}

void Cell::Render() const
{
	if (occupied_)
	{
		SDL_SetRenderDrawColor(game_->renderer_, color_.r, color_.g, color_.b, color_.a);
		SDL_RenderFillRect(game_->renderer_, &rect_);
	}
}