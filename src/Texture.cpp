#include "Texture.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

Texture::Texture() : texture_(nullptr), width_(0), height_(0)
{
}

Texture::~Texture()
{
	FreeTexture();
}

void Texture::FreeTexture()
{
	if (texture_ != nullptr)
	{
		SDL_DestroyTexture(texture_);
		texture_ = nullptr;
		width_ = 0;
		height_ = 0;
	}
}

bool Texture::LoadFromText(SDL_Renderer* renderer, TTF_Font* font, const char* text, const SDL_Color& color, int text_length)
{
	FreeTexture();
	SDL_Surface* text_surface = text_length == -1 ? TTF_RenderText_Blended(font, text, color) : TTF_RenderText_Blended_Wrapped(font, text, color, text_length);

	if (text_surface == nullptr)
	{
		printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
		return false;
	}

	texture_ = SDL_CreateTextureFromSurface(renderer, text_surface);

	if (texture_ == nullptr)
	{
		printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
		SDL_FreeSurface(text_surface);
		return false;
	}

	width_ = text_surface->w;
	height_ = text_surface->h;
	SDL_FreeSurface(text_surface);
	return true;
}

void Texture::Render(SDL_Renderer* renderer, int x, int y, float scale, SDL_Rect* clip)
{
	SDL_Rect render_rect = { x, y, static_cast<int>(width_ * scale), static_cast<int>(height_ * scale) };

	if (clip != nullptr)
	{
		render_rect.w = clip->w * scale;
		render_rect.h = clip->h * scale;
	}

	SDL_RenderCopy(renderer, texture_, clip, &render_rect);
}