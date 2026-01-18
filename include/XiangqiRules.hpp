#pragma once

#include "Types.hpp"

#include <array>
#include <optional>
#include <vector>

struct Move {
    Pos from;
    Pos to;
};

struct BoardState {
    // board[y][x]
    std::array<std::array<std::optional<Piece>, 9>, 10> cells{};

    std::optional<Piece>& at(const Pos& p) { return cells[p.y][p.x]; }
    const std::optional<Piece>& at(const Pos& p) const { return cells[p.y][p.x]; }
};

namespace xiangqi {

BoardState initialBoard();

bool inBounds(const Pos& p);

// Returns all legal moves for `side` from a given position.
std::vector<Move> legalMovesFrom(const BoardState& b, const Pos& from, Side side);

// Returns all legal moves for `side` across the whole board.
std::vector<Move> allLegalMoves(const BoardState& b, Side side);

bool isInCheck(const BoardState& b, Side side);

// Apply a move (assumes move is legal). Returns captured piece if any.
std::optional<Piece> applyMove(BoardState& b, const Move& m);

} // namespace xiangqi
