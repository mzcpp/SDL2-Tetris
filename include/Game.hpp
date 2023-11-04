#ifndef GAME_HPP
#define GAME_HPP

#include "Texture.hpp"
#include "Cell.hpp"
#include "Tetromino.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <deque>
#include <random>
#include <memory>
#include <vector>

class Game
{
private:
	int ticks_;
	int moving_ticks_;
	bool initialized_;
	bool running_;
	int score_;
	int lines_;
	int descend_speed_;
	bool game_over_;
	int cell_size_;
	bool moving_left_;
	bool moving_right_;
	bool moving_down_;
	bool unstash_possible_;

	SDL_Rect info_viewport_;
	SDL_Rect board_viewport_;
	SDL_Rect queue_viewport_;

	std::unique_ptr<Tetromino> stashed_tetromino_;
	std::unique_ptr<Tetromino> falling_tetromino_;
	std::vector<std::unique_ptr<Tetromino>> queued_tetrominoes_;
	std::deque<TetrominoType> tetromino_queue_;

	std::unique_ptr<Texture> score_texture_;
	std::unique_ptr<Texture> lines_texture_;
	std::unique_ptr<Texture> game_over_texture_;
	std::unique_ptr<Texture> stash_texture_;
	std::unique_ptr<Texture> next_texture_;

	std::vector<Cell> stash_board_;
	std::vector<Cell> queue_board_;

	TTF_Font* font_;
	SDL_Window* window_;

public:
	int cells_width_;
	int cells_height_;

	std::vector<Cell> board_;
	SDL_Renderer* renderer_;

	Game();

	~Game();

	bool Initialize();

	void Finalize();

	void Run();
	
	void Stop();

	void HandleEvents();

	void Tick();

	void Render();

	void InitBoard(std::vector<Cell>* board, std::size_t size, std::size_t width, const SDL_Point& top_left);
	
	void InitQueue();

	void UpdateQueue();

	void Reset();

	void TriggerStashTetromino();

	void RenderFalingTetromino();
	
	void RenderStashedTetromino();
	
	void RenderQueuedTetrominoes();

	void RenderBoards();

	void RenderBoardGridLines(const std::vector<Cell>& board, std::size_t board_cells_width, std::size_t board_cells_height, const SDL_Rect& viewport);

	void RenderBoardCells(const std::vector<Cell>& board, const SDL_Rect& viewport);
	
	void RenderInfo();

	void GenerateTetrominoes();

	void SpawnTetromino(TetrominoType type, bool unstashing = false);
	
	void SettleTetromino(int* score_ = nullptr);

	void UpdateScoreText();

	void UpdateLinesText();
	
	void ClearFilledLines();

	void DescendUnfilledLines();
};

#endif
