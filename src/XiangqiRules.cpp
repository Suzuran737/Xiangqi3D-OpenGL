#include "XiangqiRules.hpp"

#include <algorithm>

namespace {

constexpr int WIDTH = 9;
constexpr int HEIGHT = 10;

bool inPalace(Side side, const Pos& p) {
    if (p.x < 3 || p.x > 5) return false;
    if (side == Side::Red) return (p.y >= 0 && p.y <= 2);
    return (p.y >= 7 && p.y <= 9);
}

bool onOwnSideForElephant(Side side, const Pos& p) {
    // River is between y=4 and y=5
    if (side == Side::Red) return p.y <= 4;
    return p.y >= 5;
}

bool hasPiece(const BoardState& b, const Pos& p) {
    return b.at(p).has_value();
}

Side other(Side s) {
    return (s == Side::Red) ? Side::Black : Side::Red;
}

int forwardDir(Side s) {
    return (s == Side::Red) ? +1 : -1;
}

std::optional<Pos> findKing(const BoardState& b, Side side) {
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            const auto& cell = b.cells[y][x];
            if (cell && cell->side == side && cell->type == PieceType::King) {
                return Pos{x, y};
            }
        }
    }
    return std::nullopt;
}

bool clearPathRookLike(const BoardState& b, const Pos& a, const Pos& c) {
    // a and c in same row or col. Checks there are no pieces between them.
    if (a.x == c.x) {
        int x = a.x;
        int y0 = std::min(a.y, c.y) + 1;
        int y1 = std::max(a.y, c.y) - 1;
        for (int y = y0; y <= y1; ++y) {
            if (b.cells[y][x]) return false;
        }
        return true;
    }
    if (a.y == c.y) {
        int y = a.y;
        int x0 = std::min(a.x, c.x) + 1;
        int x1 = std::max(a.x, c.x) - 1;
        for (int x = x0; x <= x1; ++x) {
            if (b.cells[y][x]) return false;
        }
        return true;
    }
    return false;
}

int countPiecesBetweenLine(const BoardState& b, const Pos& a, const Pos& c) {
    // assumes same row or col
    int count = 0;
    if (a.x == c.x) {
        int x = a.x;
        int y0 = std::min(a.y, c.y) + 1;
        int y1 = std::max(a.y, c.y) - 1;
        for (int y = y0; y <= y1; ++y) {
            if (b.cells[y][x]) count++;
        }
    } else if (a.y == c.y) {
        int y = a.y;
        int x0 = std::min(a.x, c.x) + 1;
        int x1 = std::max(a.x, c.x) - 1;
        for (int x = x0; x <= x1; ++x) {
            if (b.cells[y][x]) count++;
        }
    }
    return count;
}

bool attacksSquare(const BoardState& b, const Pos& from, Piece p, const Pos& target) {
    // Whether piece at `from` attacks `target` (ignoring whether `from` has the piece).
    int dx = target.x - from.x;
    int dy = target.y - from.y;

    switch (p.type) {
        case PieceType::King: {
            // king attacks adjacent within palace; plus "facing kings" handled elsewhere
            if (!inPalace(p.side, target)) return false;
            return (std::abs(dx) + std::abs(dy) == 1);
        }
        case PieceType::Advisor: {
            if (!inPalace(p.side, target)) return false;
            return (std::abs(dx) == 1 && std::abs(dy) == 1);
        }
        case PieceType::Elephant: {
            if (!onOwnSideForElephant(p.side, target)) return false;
            if (std::abs(dx) == 2 && std::abs(dy) == 2) {
                Pos eye{from.x + dx / 2, from.y + dy / 2};
                return !hasPiece(b, eye);
            }
            return false;
        }
        case PieceType::Horse: {
            if (!((std::abs(dx) == 2 && std::abs(dy) == 1) || (std::abs(dx) == 1 && std::abs(dy) == 2))) return false;
            // "leg" block
            if (std::abs(dx) == 2) {
                Pos leg{from.x + dx / 2, from.y};
                return !hasPiece(b, leg);
            } else {
                Pos leg{from.x, from.y + dy / 2};
                return !hasPiece(b, leg);
            }
        }
        case PieceType::Rook: {
            if (!(dx == 0 || dy == 0)) return false;
            return clearPathRookLike(b, from, target);
        }
        case PieceType::Cannon: {
            if (!(dx == 0 || dy == 0)) return false;
            // cannon attacks only with exactly one screen piece between
            return countPiecesBetweenLine(b, from, target) == 1;
        }
        case PieceType::Pawn: {
            int f = forwardDir(p.side);
            // Pawn attacks forward (always) and sideways only after crossing.
            if (dx == 0 && dy == f) return true;
            bool crossed = (p.side == Side::Red) ? (from.y >= 5) : (from.y <= 4);
            if (crossed && std::abs(dx) == 1 && dy == 0) return true;
            return false;
        }
    }
    return false;
}

std::vector<Move> pseudoMovesFrom(const BoardState& b, const Pos& from, Side side) {
    std::vector<Move> out;
    if (from.x < 0 || from.x >= WIDTH || from.y < 0 || from.y >= HEIGHT) return out;
    const auto& cell = b.at(from);
    if (!cell) return out;
    Piece p = *cell;
    if (p.side != side) return out;

    auto addIfOk = [&](const Pos& to) {
        if (to.x < 0 || to.x >= WIDTH || to.y < 0 || to.y >= HEIGHT) return;
        const auto& dst = b.at(to);
        if (dst && dst->side == side) return;
        out.push_back(Move{from, to});
    };

    switch (p.type) {
        case PieceType::King: {
            static const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
            for (auto& d : dirs) {
                Pos to{from.x + d[0], from.y + d[1]};
                if (!inPalace(side, to)) continue;
                addIfOk(to);
            }
            break;
        }
        case PieceType::Advisor: {
            static const int dirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
            for (auto& d : dirs) {
                Pos to{from.x + d[0], from.y + d[1]};
                if (!inPalace(side, to)) continue;
                addIfOk(to);
            }
            break;
        }
        case PieceType::Elephant: {
            static const int dirs[4][2] = {{2,2},{2,-2},{-2,2},{-2,-2}};
            for (auto& d : dirs) {
                Pos to{from.x + d[0], from.y + d[1]};
                if (!xiangqi::inBounds(to)) continue;
                if (!onOwnSideForElephant(side, to)) continue;
                Pos eye{from.x + d[0]/2, from.y + d[1]/2};
                if (hasPiece(b, eye)) continue;
                addIfOk(to);
            }
            break;
        }
        case PieceType::Horse: {
            static const int moves[8][2] = {{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};
            for (auto& m : moves) {
                int dx = m[0], dy = m[1];
                Pos to{from.x + dx, from.y + dy};
                if (!xiangqi::inBounds(to)) continue;
                // leg
                if (std::abs(dx) == 2) {
                    Pos leg{from.x + dx/2, from.y};
                    if (hasPiece(b, leg)) continue;
                } else {
                    Pos leg{from.x, from.y + dy/2};
                    if (hasPiece(b, leg)) continue;
                }
                addIfOk(to);
            }
            break;
        }
        case PieceType::Rook: {
            static const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
            for (auto& d : dirs) {
                Pos to{from.x, from.y};
                while (true) {
                    to.x += d[0];
                    to.y += d[1];
                    if (!xiangqi::inBounds(to)) break;
                    const auto& dst = b.at(to);
                    if (!dst) {
                        out.push_back(Move{from, to});
                    } else {
                        if (dst->side != side) out.push_back(Move{from, to});
                        break;
                    }
                }
            }
            break;
        }
        case PieceType::Cannon: {
            static const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
            for (auto& d : dirs) {
                Pos to{from.x, from.y};
                bool seenScreen = false;
                while (true) {
                    to.x += d[0];
                    to.y += d[1];
                    if (!xiangqi::inBounds(to)) break;
                    const auto& dst = b.at(to);
                    if (!seenScreen) {
                        if (!dst) {
                            out.push_back(Move{from, to});
                        } else {
                            seenScreen = true;
                        }
                    } else {
                        if (dst) {
                            if (dst->side != side) out.push_back(Move{from, to});
                            break;
                        }
                    }
                }
            }
            break;
        }
        case PieceType::Pawn: {
            int f = forwardDir(side);
            addIfOk(Pos{from.x, from.y + f});
            bool crossed = (side == Side::Red) ? (from.y >= 5) : (from.y <= 4);
            if (crossed) {
                addIfOk(Pos{from.x - 1, from.y});
                addIfOk(Pos{from.x + 1, from.y});
            }
            break;
        }
    }

    return out;
}

struct Undo {
    std::optional<Piece> captured;
};

void doMove(BoardState& b, const Move& m, Undo& u) {
    u.captured = b.at(m.to);
    b.at(m.to) = b.at(m.from);
    b.at(m.from) = std::nullopt;
}

void undoMove(BoardState& b, const Move& m, const Undo& u) {
    b.at(m.from) = b.at(m.to);
    b.at(m.to) = u.captured;
}

bool kingsFacing(const BoardState& b) {
    auto rK = findKing(b, Side::Red);
    auto bK = findKing(b, Side::Black);
    if (!rK || !bK) return false;
    if (rK->x != bK->x) return false;
    int x = rK->x;
    int y0 = std::min(rK->y, bK->y) + 1;
    int y1 = std::max(rK->y, bK->y) - 1;
    for (int y = y0; y <= y1; ++y) {
        if (b.cells[y][x]) return false;
    }
    return true;
}

} // namespace

namespace xiangqi {

BoardState initialBoard() {
    BoardState b;

    auto put = [&](int x, int y, Side side, PieceType type) {
        b.cells[y][x] = Piece{side, type};
    };

    // Red (bottom)
    put(0, 0, Side::Red, PieceType::Rook);
    put(1, 0, Side::Red, PieceType::Horse);
    put(2, 0, Side::Red, PieceType::Elephant);
    put(3, 0, Side::Red, PieceType::Advisor);
    put(4, 0, Side::Red, PieceType::King);
    put(5, 0, Side::Red, PieceType::Advisor);
    put(6, 0, Side::Red, PieceType::Elephant);
    put(7, 0, Side::Red, PieceType::Horse);
    put(8, 0, Side::Red, PieceType::Rook);
    put(1, 2, Side::Red, PieceType::Cannon);
    put(7, 2, Side::Red, PieceType::Cannon);
    put(0, 3, Side::Red, PieceType::Pawn);
    put(2, 3, Side::Red, PieceType::Pawn);
    put(4, 3, Side::Red, PieceType::Pawn);
    put(6, 3, Side::Red, PieceType::Pawn);
    put(8, 3, Side::Red, PieceType::Pawn);

    // Black (top)
    put(0, 9, Side::Black, PieceType::Rook);
    put(1, 9, Side::Black, PieceType::Horse);
    put(2, 9, Side::Black, PieceType::Elephant);
    put(3, 9, Side::Black, PieceType::Advisor);
    put(4, 9, Side::Black, PieceType::King);
    put(5, 9, Side::Black, PieceType::Advisor);
    put(6, 9, Side::Black, PieceType::Elephant);
    put(7, 9, Side::Black, PieceType::Horse);
    put(8, 9, Side::Black, PieceType::Rook);
    put(1, 7, Side::Black, PieceType::Cannon);
    put(7, 7, Side::Black, PieceType::Cannon);
    put(0, 6, Side::Black, PieceType::Pawn);
    put(2, 6, Side::Black, PieceType::Pawn);
    put(4, 6, Side::Black, PieceType::Pawn);
    put(6, 6, Side::Black, PieceType::Pawn);
    put(8, 6, Side::Black, PieceType::Pawn);

    return b;
}

bool inBounds(const Pos& p) {
    return (p.x >= 0 && p.x < WIDTH && p.y >= 0 && p.y < HEIGHT);
}

bool isInCheck(const BoardState& b, Side side) {
    auto k = findKing(b, side);
    if (!k) return false;

    // Facing kings is effectively mutual check
    if (kingsFacing(b)) {
        // If kings face, both are "in check"
        return true;
    }

    Side enemy = other(side);
    // Iterate all enemy pieces and see if they attack our king
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            const auto& cell = b.cells[y][x];
            if (!cell || cell->side != enemy) continue;
            Pos from{x, y};
            // For rook/cannon we need line checks; attacksSquare covers that.
            if (attacksSquare(b, from, *cell, *k)) {
                // Special case: cannon only attacks if king is on a piece (capture)
                // In check detection, cannon attacks the king as capture, so screen count==1 is enough.
                // For rook-like it's OK.
                // But for cannon, if there is exactly one piece between, it's check regardless of target occupancy.
                // attacksSquare already checks screen==1.
                // Great.
                return true;
            }
        }
    }

    return false;
}

std::vector<Move> legalMovesFrom(const BoardState& b, const Pos& from, Side side) {
    std::vector<Move> out;
    auto pseudo = pseudoMovesFrom(b, from, side);

    BoardState temp = b;
    for (const auto& m : pseudo) {
        Undo u;
        doMove(temp, m, u);
        bool ok = !isInCheck(temp, side);
        undoMove(temp, m, u);

        if (ok) out.push_back(m);
    }

    return out;
}

std::vector<Move> allLegalMoves(const BoardState& b, Side side) {
    std::vector<Move> out;
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            Pos p{x, y};
            const auto& cell = b.cells[y][x];
            if (!cell || cell->side != side) continue;
            auto ms = legalMovesFrom(b, p, side);
            out.insert(out.end(), ms.begin(), ms.end());
        }
    }
    return out;
}

std::optional<Piece> applyMove(BoardState& b, const Move& m) {
    std::optional<Piece> cap = b.at(m.to);
    b.at(m.to) = b.at(m.from);
    b.at(m.from) = std::nullopt;
    return cap;
}

} // namespace xiangqi
