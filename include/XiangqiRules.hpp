#pragma once

#include "Types.hpp"

#include <array>
#include <optional>
#include <vector>

// 走法
struct Move {
    Pos from;
    Pos to;
};

// 棋盘状态
struct BoardState {
    // cells[y][x]
    std::array<std::array<std::optional<Piece>, 9>, 10> cells{};

    std::optional<Piece>& at(const Pos& p) { return cells[p.y][p.x]; }
    const std::optional<Piece>& at(const Pos& p) const { return cells[p.y][p.x]; }
};

// 象棋规则相关函数
namespace xiangqi {

BoardState initialBoard();

bool inBounds(const Pos& p);

// 从指定位置生成该方合法走法
std::vector<Move> legalMovesFrom(const BoardState& b, const Pos& from, Side side);

// 获取该方所有合法走法
std::vector<Move> allLegalMoves(const BoardState& b, Side side);

bool isInCheck(const BoardState& b, Side side);

// 应用走法，返回被吃的棋子（若有）
std::optional<Piece> applyMove(BoardState& b, const Move& m);

} // namespace xiangqi
