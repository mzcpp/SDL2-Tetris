#ifndef TETROMINO_HPP
#define TETROMINO_HPP

#include "Cell.hpp"

#include <SDL2/SDL.h>

#include <array>
#include <vector>

enum class TetrominoType
{
	I_BLOCK, J_BLOCK, L_BLOCK, O_BLOCK, S_BLOCK, T_BLOCK, Z_BLOCK
};

class Game;

class Tetromino
{
private:
	Game* game_;
	TetrominoType type_;
	int rotation_degrees_;
	std::vector<std::size_t> rotation_indices_;
	std::array<Cell*, 4> blocks_;
	std::vector<Cell*> bounding_box_;
	SDL_Color render_color_;

public:
	Tetromino(Game* game);

	void Initialize(const std::vector<Cell*>& board_cells, TetrominoType type);

	void Render();

	TetrominoType GetType();
	
	std::size_t GetBBoxSize();
	
	std::size_t GetBBoxDimension();

	std::vector<std::size_t> GetRotatedIndices(const std::vector<std::size_t>& indices, int degrees, std::size_t matrix_dimension);

	void RotateMatrix(std::vector<std::size_t>* matrix, std::size_t matrix_dimension);

	bool DescendTetromino(int* score_ = nullptr);

	void MoveTetromino(bool right);
	
	void SettleTetromino(int* score_ = nullptr);

	bool BlocksAtSettlePosition(const std::array<Cell*, 4>& blocks);

	void RotateTetromino(int degrees);
};

#endif
