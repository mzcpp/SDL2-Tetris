#include "Constants.hpp"
#include "Game.hpp"
#include "Tetromino.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <cstdint>
#include <string>
#include <deque>
#include <iostream>
#include <random>
#include <vector>
#include <cassert>

Game::Game() : 
	ticks_(0), 
	moving_ticks_(0), 
	initialized_(false), 
	running_(false), 
	score_(0), 
	lines_(0), 
	descend_speed_(60), 
	game_over_(false), 
	cell_size_(32), 
	moving_left_(false), 
	moving_right_(false), 
	moving_down_(false), 
	unstash_possible_(false), 
	stashed_tetromino_(nullptr), 
	falling_tetromino_(std::make_unique<Tetromino>(this)), 
	score_texture_(std::make_unique<Texture>()), 
	lines_texture_(std::make_unique<Texture>()), 
	game_over_texture_(std::make_unique<Texture>()), 
	stash_texture_(std::make_unique<Texture>()), 
	next_texture_(std::make_unique<Texture>()), 
	font_(nullptr), 
	window_(nullptr), 
	cells_width_(0), 
	cells_height_(0), 
	renderer_(nullptr)
{
	initialized_ = Initialize();

	if (!initialized_)
	{
		return;
	}

	info_viewport_.x = 0;
	info_viewport_.y = 0;
	info_viewport_.w = constants::screen_width / 3;
	info_viewport_.h = constants::screen_height;

	board_viewport_.x = constants::screen_width * 1 / 3;
	board_viewport_.y = 0;
	board_viewport_.w = constants::screen_width / 3;
	board_viewport_.h = constants::screen_height;

	queue_viewport_.x = constants::screen_width * 2 / 3;
	queue_viewport_.y = 0;
	queue_viewport_.w = constants::screen_width / 3;
	queue_viewport_.h = constants::screen_height;

	cells_width_ = (board_viewport_.w / cell_size_);
	cells_height_ = (board_viewport_.h / cell_size_);

	UpdateScoreText();
	UpdateLinesText();

	game_over_texture_->LoadFromText(renderer_, font_, "Game Over! Press 'r' to reset.", { 0xff, 0x00, 0x00, 0xff }, 200);
	stash_texture_->LoadFromText(renderer_, font_, "Stash", { 0xff, 0xff, 0xff, 0xff });
	next_texture_->LoadFromText(renderer_, font_, "Next", { 0xff, 0xff, 0xff, 0xff });

	InitBoard(&board_, cells_width_ * cells_height_, cells_width_, { 0, 0 });
	GenerateTetrominoes();
	SpawnTetromino(tetromino_queue_.front());
	InitBoard(&stash_board_, falling_tetromino_->GetBBoxSize(), falling_tetromino_->GetBBoxDimension(), { 96, 160 });
	InitQueue();	
	UpdateQueue();
}

Game::~Game()
{
	Finalize();
}

bool Game::Initialize()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not be initialized! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"))
	{
		printf("%s\n", "Warning: Texture filtering is not enabled!");
	}

	window_ = SDL_CreateWindow(constants::game_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, constants::screen_width, constants::screen_height, SDL_WINDOW_SHOWN);

	if (window_ == nullptr)
	{
		printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);

	if (renderer_ == nullptr)
	{
		printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	constexpr int img_flags = IMG_INIT_PNG;

	if (!(IMG_Init(img_flags) & img_flags))
	{
		printf("SDL_image could not be initialized! SDL_image Error: %s\n", IMG_GetError());
		return false;
	}

	if (TTF_Init() == -1)
	{
		printf("SDL_ttf could not be initialized! SDL_ttf Error: %s\n", TTF_GetError());
		return false;
	}
	
	font_ = TTF_OpenFont("res/font/font.ttf", 38);

	if (font_ == nullptr)
	{
		printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
		return false;
	}

	return true;
}

void Game::Finalize()
{
	SDL_DestroyWindow(window_);
	window_ = nullptr;

	SDL_DestroyRenderer(renderer_);
	renderer_ = nullptr;

	TTF_CloseFont(font_);
	font_ = nullptr;

	SDL_Quit();
	IMG_Quit();
	TTF_Quit();
}

void Game::Run()
{
	if (!initialized_)
	{
		return;
	}

	running_ = true;

	constexpr long double ms = 1.0 / 60.0;
	std::uint64_t last_time = SDL_GetPerformanceCounter();
	long double delta = 0.0;

	double timer = SDL_GetTicks();

	int frames = 0;
	int ticks = 0;

	while (running_)
	{
		const std::uint64_t now = SDL_GetPerformanceCounter();
		const long double elapsed = static_cast<long double>(now - last_time) / static_cast<long double>(SDL_GetPerformanceFrequency());

		last_time = now;
		delta += elapsed;

		HandleEvents();

		while (delta >= ms)
		{
			Tick();
			delta -= ms;
			++ticks;
		}

		//printf("%Lf\n", delta / ms);
		Render();
		++frames;

		if (SDL_GetTicks() - timer > 1000)
		{
			timer += 1000;
			//printf("Frames: %d, Ticks: %d\n", frames, ticks);
			frames = 0;
			ticks = 0;
		}
	}
}

void Game::Stop()
{
	running_ = false;
}

void Game::HandleEvents()
{
	SDL_Event e;
	
	while (SDL_PollEvent(&e) != 0)
	{
		if (e.type == SDL_QUIT)
		{
			Stop();
			return;
		}
		
		if (game_over_ && e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_r)
		{
			Reset();
		}

		if (!game_over_ && e.type == SDL_KEYDOWN)
		{
			if (e.key.keysym.sym == SDLK_UP)
			{
				falling_tetromino_->RotateTetromino(90);
			}

			if (e.key.keysym.sym == SDLK_LEFT)
			{
				moving_left_ = true;
			}
			else if (e.key.keysym.sym == SDLK_RIGHT)
			{
				moving_right_ = true;
			}
			
			if (e.key.keysym.sym == SDLK_DOWN)
			{
				moving_down_ = true;
			}
			
			if (e.key.keysym.sym == SDLK_SPACE)
			{
				SettleTetromino(&score_);
			}
			else if (e.key.keysym.sym == SDLK_c)
			{
				TriggerStashTetromino();
			}
		}
		else if (!game_over_ && e.type == SDL_KEYUP)
		{
			if (e.key.keysym.sym == SDLK_LEFT)
			{
				moving_left_ = false;
				moving_ticks_ = 0;
			}
			else if (e.key.keysym.sym == SDLK_RIGHT)
			{
				moving_right_ = false;
				moving_ticks_ = 0;
			}
			
			if (e.key.keysym.sym == SDLK_DOWN)
			{
				moving_down_ = false;
				moving_ticks_ = 0;
			}
		}
	}
}

void Game::Tick()
{
	if (game_over_)
	{
		return;
	}

	++ticks_;

	if (ticks_ % descend_speed_ == 0)
	{
		if (!moving_down_ && !falling_tetromino_->DescendTetromino())
		{
			SettleTetromino();
		}
	}

	if (moving_left_ || moving_right_ || moving_down_)
	{
		if (moving_ticks_ % 5 == 0)
		{
			if (moving_left_)
			{
				falling_tetromino_->MoveTetromino(false);
			}
			else if (moving_right_)
			{
				falling_tetromino_->MoveTetromino(true);
			}
			
			if (moving_down_)
			{
				if (!falling_tetromino_->DescendTetromino(&score_))
				{
					SettleTetromino();
				}
			}
		}

		++moving_ticks_;
	}

	UpdateScoreText();
}

void Game::Render()
{
	SDL_RenderSetViewport(renderer_, NULL);
	SDL_SetRenderDrawColor(renderer_, 0x00, 0x00, 0x00, 0xff);
	SDL_RenderClear(renderer_);

	RenderFalingTetromino();
	RenderStashedTetromino();
	RenderQueuedTetrominoes();

	RenderBoards();
	RenderInfo();

	SDL_RenderPresent(renderer_);
}

void Game::InitBoard(std::vector<Cell>* board, std::size_t size, std::size_t width, const SDL_Point& top_left)
{
	assert(board != nullptr);

	board->clear();
	board->resize(size, this);

	int rect_x = top_left.x;
	int rect_y = top_left.y;

	for (std::size_t i = 0; i < board->size(); ++i)
	{
		(*board)[i].rect_.x = rect_x;
		(*board)[i].rect_.y = rect_y;
		(*board)[i].rect_.w = cell_size_;
		(*board)[i].rect_.h = (*board)[i].rect_.w;

		if ((i + 1) % width == 0)
		{
			rect_x = top_left.x;
			rect_y += cell_size_;
		}
		else
		{
			rect_x += cell_size_;
		}
	}
}

void Game::InitQueue()
{
	InitBoard(&queue_board_, 48, 4, { 96, 160 });
	queued_tetrominoes_.resize(3);
	queued_tetrominoes_[0] = std::make_unique<Tetromino>(this);
	queued_tetrominoes_[1] = std::make_unique<Tetromino>(this);
	queued_tetrominoes_[2] = std::make_unique<Tetromino>(this);
}

void Game::UpdateQueue()
{
	assert(!tetromino_queue_.empty());
	std::vector<Cell*> bbox;

	for (std::size_t ti = 0; ti < 3; ++ti)
	{
		std::size_t dimension = (tetromino_queue_[ti] == TetrominoType::I_BLOCK || tetromino_queue_[ti] == TetrominoType::O_BLOCK) ? 4 : 3;
		std::size_t start_index = ti * 16;
		std::size_t index = ti * 16;

		for (std::size_t i = 0; i < dimension * dimension; ++i)
		{
			bbox.emplace_back(&queue_board_[index++]);

			if (dimension == 3 && ((index - start_index) == 3 || (index - start_index) == 7 || (index - start_index) == 11))
			{
				index++;
			}
		}

		queued_tetrominoes_[ti]->Initialize(bbox, tetromino_queue_[ti]);
		bbox.clear();
	}
}

void Game::Reset()
{
	stash_board_.clear();
	stashed_tetromino_.reset();
	stashed_tetromino_ = nullptr;

	for (Cell& cell : board_)
	{
		cell.color_ = { 0x00, 0x00, 0x00, 0xff };
		cell.occupied_ = false;
	}

	for (Cell& cell : queue_board_)
	{
		cell.color_ = { 0x00, 0x00, 0x00, 0xff };
		cell.occupied_ = false;
	}

	tetromino_queue_.clear();
	GenerateTetrominoes();
	UpdateQueue();

	score_ = 0;
	lines_ = 0;
	descend_speed_ = 60;
	moving_left_ = false;
	moving_right_ = false;
	moving_down_ = false;
	game_over_ = false;
}

void Game::TriggerStashTetromino()
{
	if (stashed_tetromino_ == nullptr || unstash_possible_)
	{
		InitBoard(&stash_board_, falling_tetromino_->GetBBoxSize(), falling_tetromino_->GetBBoxDimension(), { 96, 160 });

		std::vector<Cell*> bbox;

		for (std::size_t i = 0; i < stash_board_.size(); ++i)
		{
			bbox.emplace_back(&stash_board_[i]);
		}

		if (unstash_possible_)
		{
			TetrominoType stashed_type = stashed_tetromino_->GetType();
			stashed_tetromino_->Initialize(bbox, falling_tetromino_->GetType());
			unstash_possible_ = false;
			SpawnTetromino(stashed_type, true);
		}

		if (stashed_tetromino_ == nullptr)
		{
			stashed_tetromino_ = std::make_unique<Tetromino>(this);
			stashed_tetromino_->Initialize(bbox, falling_tetromino_->GetType());
			unstash_possible_ = false;
			SpawnTetromino(tetromino_queue_.front());
			UpdateQueue();
		}
	}
}

void Game::RenderFalingTetromino()
{
	SDL_RenderSetViewport(renderer_, &board_viewport_);
	falling_tetromino_->Render();
	SDL_RenderSetViewport(renderer_, NULL);
}

void Game::RenderStashedTetromino()
{
	if (stashed_tetromino_ != nullptr)
	{
		SDL_RenderSetViewport(renderer_, &info_viewport_);
		stashed_tetromino_->Render();
		SDL_RenderSetViewport(renderer_, NULL);
	}
}

void Game::RenderQueuedTetrominoes()
{
	SDL_RenderSetViewport(renderer_, &queue_viewport_);

	for (const std::unique_ptr<Tetromino>& tetromino : queued_tetrominoes_)
	{
		tetromino->Render();
	}
	
	SDL_RenderSetViewport(renderer_, NULL);
}

void Game::RenderBoards()
{
	if (stashed_tetromino_ != nullptr)
	{
		RenderBoardCells(stash_board_, info_viewport_);
	}

	RenderBoardCells(board_, board_viewport_);
	RenderBoardCells(queue_board_, queue_viewport_);

	if (stashed_tetromino_ != nullptr)
	{
		RenderBoardGridLines(stash_board_, stashed_tetromino_->GetBBoxDimension(), stashed_tetromino_->GetBBoxDimension(), info_viewport_);
	}

	RenderBoardGridLines(board_, cells_width_, cells_height_, board_viewport_);
	RenderBoardGridLines(queue_board_, 4, 12, queue_viewport_);

	SDL_SetRenderDrawColor(renderer_, 0xff, 0xff, 0xff, 0xff);
	SDL_RenderSetViewport(renderer_, &info_viewport_);

	stash_texture_->Render(renderer_, (info_viewport_.w / 2) - (stash_texture_->width_ / 2), stash_board_[0].rect_.y - (2 * cell_size_));
	SDL_RenderDrawLine(renderer_, info_viewport_.w - 1, 0, info_viewport_.w - 1, info_viewport_.h);
	
	SDL_RenderSetViewport(renderer_, &queue_viewport_);
	
	next_texture_->Render(renderer_, (queue_viewport_.w / 2) - (next_texture_->width_ / 2), queue_board_[0].rect_.y - (2 * cell_size_));
	SDL_RenderDrawLine(renderer_, 0, 0, 0, queue_viewport_.h);
	
	SDL_RenderSetViewport(renderer_, NULL);
}

void Game::RenderBoardGridLines(const std::vector<Cell>& board, std::size_t board_cells_width, std::size_t board_cells_height, const SDL_Rect& viewport)
{
	SDL_SetRenderDrawColor(renderer_, 0x15, 0x16, 0x17, 0xff);
	SDL_RenderSetViewport(renderer_, &viewport);

	for (std::size_t i = 1; i < board_cells_width; ++i)
	{
		SDL_RenderDrawLine(renderer_, board[i].rect_.x, board.front().rect_.y, board[i].rect_.x, board.back().rect_.y + cell_size_);
	}

	for (std::size_t i = 1; i < board_cells_height; ++i)
	{
		SDL_RenderDrawLine(renderer_, board.front().rect_.x, board[i * board_cells_width].rect_.y, board.back().rect_.x + cell_size_, board[i * board_cells_width].rect_.y);
	}
	
	SDL_RenderSetViewport(renderer_, NULL);
}

void Game::RenderBoardCells(const std::vector<Cell>& board, const SDL_Rect& viewport)
{
	SDL_RenderSetViewport(renderer_, &viewport);

	std::for_each(board.begin(), board.end(), [](const Cell& cell)
		{
			cell.Render();
		});

	SDL_RenderSetViewport(renderer_, NULL);
}

void Game::RenderInfo()
{
	SDL_RenderSetViewport(renderer_, &info_viewport_);

	const int info_height = info_viewport_.h * 3 / 4;

	score_texture_->Render(renderer_, (info_viewport_.w / 2) - (score_texture_->width_ / 2), info_height);
	lines_texture_->Render(renderer_, (info_viewport_.w / 2) - (lines_texture_->width_ / 2), info_height + (lines_texture_->height_ * 2));

	if (game_over_)
	{
		game_over_texture_->Render(renderer_, (info_viewport_.w / 2) - (game_over_texture_->width_ / 2), (info_viewport_.h / 2) - (game_over_texture_->height_ / 2));
	}

	SDL_RenderSetViewport(renderer_, NULL);
}

void Game::GenerateTetrominoes()
{
	std::vector<int> types_indices = { 0, 1, 2, 3, 4, 5, 6 };
	std::random_device rd;
    std::mt19937 mt(rd());
    constexpr int permutations = 100;

    for (int i = 0; i < permutations; ++i)
    {
	    std::shuffle(types_indices.begin(), types_indices.end(), mt);

	    for (int index : types_indices)
	    {
	    	tetromino_queue_.emplace_back(static_cast<TetrominoType>(index));
	    }
	}
}

void Game::SpawnTetromino(TetrominoType type, bool unstashing)
{
	if (falling_tetromino_ == nullptr)
	{
		falling_tetromino_ = std::make_unique<Tetromino>(this);
	}

	std::vector<Cell*> bbox;
	
	const std::size_t bbox_side_size = (type == TetrominoType::I_BLOCK || type == TetrominoType::O_BLOCK) ? 4 : 3;

	std::size_t cell_index = (cells_width_ / 2) - (bbox_side_size / 2) - 1;

	for (std::size_t i = 0; i < bbox_side_size * bbox_side_size; ++i)
	{
		bbox.push_back(&board_[cell_index]);

		if ((i + 1) % bbox_side_size == 0)
		{
			cell_index += cells_width_ - (bbox_side_size - 1);
		}
		else
		{
			++cell_index;
		}
	}

	falling_tetromino_->Initialize(bbox, type);

	if (!unstashing)
	{
		tetromino_queue_.pop_front();

		if (tetromino_queue_.size() < 10)
		{
			GenerateTetrominoes();
		}
	}

	for (std::size_t i = 0; i < bbox_side_size; ++i)
	{
		if (bbox[i]->occupied_)
		{
			game_over_ = true;
		}
	}
}

void Game::SettleTetromino(int* score_)
{
	falling_tetromino_->SettleTetromino(score_);
	ClearFilledLines();
	DescendUnfilledLines();
	SpawnTetromino(tetromino_queue_.front());
	unstash_possible_ = stashed_tetromino_ != nullptr;
	UpdateQueue();
}

void Game::UpdateScoreText()
{
	SDL_Color text_color = { 0xff, 0xff, 0xff, 0xff };
	const std::string score_text = "Score: " + std::to_string(score_);
	score_texture_->LoadFromText(renderer_, font_, score_text.c_str(), text_color);
}

void Game::UpdateLinesText()
{
	SDL_Color text_color = { 0xff, 0xff, 0xff, 0xff };
	const std::string lines_text = "Lines: " + std::to_string(lines_);
	lines_texture_->LoadFromText(renderer_, font_, lines_text.c_str(), text_color);
}

void Game::ClearFilledLines()
{
	for (int i = 0; i < cells_height_; ++i)
	{
		bool filled_line = true;

		for (int j = 0; j < cells_width_; ++j)
		{
			if (!board_[i * cells_width_ + j].occupied_)
			{
				filled_line = false;
				break;
			}
		}

		if (filled_line)
		{
			for (int j = 0; j < cells_width_; ++j)
			{
				board_[i * cells_width_ + j].color_ = { 0x00, 0x00, 0x00, 0xff };
				board_[i * cells_width_ + j].occupied_ = false;
			}

			score_ += 100;
			++lines_;

			if (lines_ % 10 == 0 && descend_speed_ > 10)
			{
				descend_speed_ -= 10;
			}

			UpdateLinesText();
		}
	}
}

void Game::DescendUnfilledLines()
{
	for (int i = cells_height_ - 2; i >= 0; --i)
	{
		bool descend_line = false;

		for (int j = 0; j < cells_width_; ++j)
		{
			if (board_[i * cells_width_ + j].occupied_)
			{
				descend_line = true;
				break;
			}
		}

		if (!descend_line)
		{
			continue;
		}

		int fall_height = 0;
		bool row_below_free = true;

		for (int k = 0; k < cells_height_ - 1 - i; ++k)
		{
			for (int j = 0; j < cells_width_; ++j)
			{
				if (board_[(i + (fall_height + 1)) * cells_width_ + j].occupied_)
				{
					row_below_free = false;
					break;
				}
			}

			if (row_below_free)
			{
				++fall_height;
			}
		}


		if (fall_height == 0)
		{
			continue;
		}

		for (int j = 0; j < cells_width_; ++j)
		{
			board_[(i + fall_height) * cells_width_ + j].color_ = board_[i * cells_width_ + j].color_;
			board_[(i + fall_height) * cells_width_ + j].occupied_ = board_[i * cells_width_ + j].occupied_;

			board_[i * cells_width_ + j].color_ = { 0x00, 0x00, 0x00, 0xff };
			board_[i * cells_width_ + j].occupied_ = false;
		}
	}
}

