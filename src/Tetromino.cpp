#include "Tetromino.hpp"
#include "Game.hpp"

#include <iostream>
#include <cmath>
#include <cassert>

Tetromino::Tetromino(Game* game) : game_(game), rotation_degrees_(0)
{
}

void Tetromino::Initialize(const std::vector<Cell*>& board_cells, TetrominoType type)
{
	type_ = type;
	rotation_degrees_ = 0;
	bounding_box_ = board_cells;

	if (type == TetrominoType::I_BLOCK)
	{
		assert(board_cells.size() == 16);
		rotation_indices_ = { 4, 5, 6, 7 };
		render_color_ = { 0x00, 0xff, 0xff, 0xff };
	}
	else if (type == TetrominoType::J_BLOCK)
	{
		assert(board_cells.size() == 9);
		rotation_indices_ = { 0, 3, 4, 5 };
		render_color_ = { 0x00, 0x00, 0xff, 0xff };
	}
	else if (type == TetrominoType::L_BLOCK)
	{
		assert(board_cells.size() == 9);
		rotation_indices_ = { 2, 3, 4, 5 };
		render_color_ = { 0xff, 0xaa, 0x00, 0xff };
	}
	else if (type == TetrominoType::O_BLOCK)
	{
		assert(board_cells.size() == 16);
		rotation_indices_ = { 1, 2, 5, 6 };
		render_color_ = { 0xff, 0xff, 0x00, 0xff };
	}
	else if (type == TetrominoType::S_BLOCK)
	{
		assert(board_cells.size() == 9);
		rotation_indices_ = { 1, 2, 3, 4 };
		render_color_ = { 0x00, 0xff, 0x00, 0xff };
	}
	else if (type == TetrominoType::T_BLOCK)
	{
		assert(board_cells.size() == 9);
		rotation_indices_ = { 1, 3, 4, 5 };
		render_color_ = { 0x99, 0x00, 0xff, 0xff };
	}
	else if (type == TetrominoType::Z_BLOCK)
	{
		assert(board_cells.size() == 9);
		rotation_indices_ = { 0, 1, 4, 5 };
		render_color_ = { 0xff, 0x00, 0x00, 0xff };
	}

	const std::size_t matrix_dimension = static_cast<std::size_t>(std::sqrt(board_cells.size()));

	for (std::size_t i = 0; i < blocks_.size(); ++i)
	{
		if (type_ == TetrominoType::I_BLOCK)
		{
			blocks_[i] = bounding_box_[rotation_indices_[i] - matrix_dimension];
		}
		else
		{
			blocks_[i] = bounding_box_[rotation_indices_[i]];
		}

		blocks_[i]->color_ = render_color_;
	}

	std::vector<std::size_t> rotation_indices_copy = rotation_indices_;

	for (std::size_t i = 1; i < 4; ++i)
	{
		const std::vector<std::size_t> rotated_indices = GetRotatedIndices(rotation_indices_copy, i * 90, matrix_dimension);
		rotation_indices_.insert(rotation_indices_.end(), rotated_indices.begin(), rotated_indices.end());
	}
}

void Tetromino::Render()
{
	SDL_SetRenderDrawColor(game_->renderer_, render_color_.r, render_color_.g, render_color_.b, render_color_.a);

	for (Cell* cell : blocks_)
	{
		SDL_RenderFillRect(game_->renderer_, &cell->rect_);
	}

	std::array<Cell*, 4> blocks_tmp = blocks_;

	while (!BlocksAtSettlePosition(blocks_tmp))
	{
		for (std::size_t i = 0; i < blocks_tmp.size(); ++i)
		{
			blocks_tmp[i] += game_->cells_width_;
		}
	}
	
	for (Cell* cell : blocks_tmp)
	{
		SDL_Rect rect_cpy = cell->rect_;

		++rect_cpy.x;
		++rect_cpy.y;
		rect_cpy.w -= 2;
		rect_cpy.h -= 2;

		SDL_RenderDrawRect(game_->renderer_, &rect_cpy);
	}
}

TetrominoType Tetromino::GetType()
{
	return type_;
}

std::size_t Tetromino::GetBBoxSize()
{
	return bounding_box_.size();
}
	
std::size_t Tetromino::GetBBoxDimension()
{
	return (type_ == TetrominoType::I_BLOCK || type_ == TetrominoType::O_BLOCK) ? 4 : 3;
}

std::vector<std::size_t> Tetromino::GetRotatedIndices(const std::vector<std::size_t>& indices, int degrees, std::size_t matrix_dimension)
{
	assert(matrix_dimension >= 0 && degrees % 90 == 0);

	std::vector<std::size_t> rotated_indices;
	std::vector<std::size_t> matrix;

	matrix.resize(matrix_dimension * matrix_dimension, 0);

	for (std::size_t index : indices)
	{
		matrix[index] = 1;
	}

	for (int i = 0; i < (degrees / 90); ++i)
	{		
		RotateMatrix(&matrix, matrix_dimension);
	}

	for (std::size_t i = 0; i < matrix.size(); ++i)
	{
		if (matrix[i] == 1)
		{
			rotated_indices.emplace_back(i);
		}
	}

	return rotated_indices;
}

void Tetromino::RotateMatrix(std::vector<std::size_t>* matrix, std::size_t matrix_dimension)
{
	assert(matrix_dimension >= 0 && matrix != nullptr);

	const std::size_t matrix_dimension_index = matrix_dimension - 1;

	std::size_t low = 0;

	for (std::size_t i = 0; i < matrix_dimension_index - low; ++i)
	{
		for (std::size_t j = low; j < matrix_dimension_index - low; ++j)
		{
			const std::size_t source_index = j * matrix_dimension + i;

			std::swap((*matrix)[source_index], (*matrix)[(i * matrix_dimension) + (matrix_dimension_index - j)]);
			std::swap((*matrix)[source_index], (*matrix)[((matrix_dimension_index - j) * matrix_dimension) + matrix_dimension_index - i]);
			std::swap((*matrix)[source_index], (*matrix)[((matrix_dimension_index - i) * matrix_dimension) + j]);

			// std::cout << j << "," << i << " <-> " << i << "," << matrix_dimension_index - j << std::endl;
			// std::cout << j << "," << i << " <-> " << matrix_dimension_index - j << "," << matrix_dimension_index - i << std::endl;
			// std::cout << j << "," << i << " <-> " << matrix_dimension_index - i << "," << j << std::endl;

			// std::cout << std::endl;
		}

		++low;

		// std::cout << std::endl;
		// std::cout << "----------------";
		// std::cout << std::endl;
	}
}

bool Tetromino::DescendTetromino(int* score_)
{
	if (BlocksAtSettlePosition(blocks_))
	{
		SettleTetromino();
		return false;
	}

	if (score_ != nullptr)
	{
		++(*score_);
	}

	for (std::size_t i = 0; i < blocks_.size(); ++i)
	{
		blocks_[i] += game_->cells_width_;
	}

	if ((bounding_box_[bounding_box_.size() - 1] - &game_->board_[0]) >= (game_->cells_width_ * game_->cells_height_ - game_->cells_width_))
	{
		return true;
	}

	for (std::size_t i = 0; i < bounding_box_.size(); ++i)
	{
		bounding_box_[i] += game_->cells_width_;
	}

	return true;
}

void Tetromino::MoveTetromino(bool right)
{
	for (std::size_t i = 0; i < blocks_.size(); ++i)
	{
		const std::size_t block_board_index = blocks_[i] - &game_->board_[0];

		if (right && ((blocks_[i] + 1)->occupied_ || (block_board_index + 1) % game_->cells_width_ == 0))
		{
			return;
		}
		else if (!right && ((blocks_[i] - 1)->occupied_ || block_board_index % game_->cells_width_ == 0))
		{
			return;
		}
	}

	for (std::size_t i = 0; i < blocks_.size(); ++i)
	{
		blocks_[i]->color_ = { 0x00, 0x00, 0x00, 0xff };
		right ? ++blocks_[i] : --blocks_[i];
	}

	for (std::size_t i = 0; i < blocks_.size(); ++i)
	{
		blocks_[i]->color_ = render_color_;
	}

	if (right && ((bounding_box_[bounding_box_.size() - 1] - &game_->board_[0]) + 1) % game_->cells_width_ == 0)
	{
		return;
	}
	else if (!right && (bounding_box_[0] - &game_->board_[0]) % game_->cells_width_ == 0)
	{
		return;
	}

	for (std::size_t i = 0; i < bounding_box_.size(); ++i)
	{
		right ? ++bounding_box_[i] : --bounding_box_[i];
	}
}

void Tetromino::SettleTetromino(int* score_)
{
	while (!BlocksAtSettlePosition(blocks_))
	{
		for (std::size_t i = 0; i < blocks_.size(); ++i)
		{
			blocks_[i] += game_->cells_width_;
		}

		if (score_ != nullptr)
		{
			++(*score_);
		}
	}

	for (Cell* cell : blocks_)
	{
		cell->occupied_ = true;
		cell->color_ = render_color_;
	}
}

bool Tetromino::BlocksAtSettlePosition(const std::array<Cell*, 4>& blocks)
{
	for (std::size_t i = 0; i < blocks.size(); ++i)
	{
		const std::size_t block_board_index = blocks[i] - &game_->board_[0];

		if (block_board_index >= static_cast<std::size_t>(game_->cells_width_ * game_->cells_height_ - game_->cells_width_) 
			|| (blocks[i] + game_->cells_width_)->occupied_)
		{
			return true;
		}
	}

	return false;
}

void Tetromino::RotateTetromino(int degrees)
{
	if (type_ == TetrominoType::O_BLOCK)
	{
		return;
	}

	std::size_t start_index = (((rotation_degrees_ / 90) * 4) + ((degrees / 90) * 4)) % 16;
	rotation_degrees_ = (rotation_degrees_ + degrees) % 360;

	bool found_space = true;
	int rotation_attempts = 0;

	do
	{
		++rotation_attempts;
		found_space = true;

		for (std::size_t i = 0; i < 4; ++i)
		{
			if (bounding_box_[rotation_indices_[start_index]]->occupied_)
			{
				found_space = false;
			}
			
			start_index = (start_index + 1) % 16;
		}

		if (!found_space && rotation_attempts == 4)
		{
			break;
		}
	}
	while (!found_space);

	if (!found_space)
	{
		return;
	}

	start_index = (start_index + 12) % 16;

	for (std::size_t i = 0; i < 4; ++i)
	{
		blocks_[i] = bounding_box_[rotation_indices_[start_index++]];
	}
}

