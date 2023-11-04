#ifndef CELL_HPP
#define CELL_HPP

#include <SDL2/SDL.h>

class Game;

class Cell
{
private:
	Game* game_;
public:
	bool occupied_;
	SDL_Rect rect_;
	SDL_Color color_;

	Cell(Game* game);

	void Render() const;
};

#endif
