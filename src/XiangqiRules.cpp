#include "XiangqiRules.hpp"

#include <algorithm>

namespace {

// 棋盘尺寸
constexpr int WIDTH = 9;
constexpr int HEIGHT = 10;

// 判断是否在九宫内
bool inPalace(Side side, const Pos& p) {
    if (p.x < 3 || p.x > 5) return false;
    if (side == Side::Red) return (p.y >= 0 && p.y <= 2);
    return (p.y >= 7 && p.y <= 9);
}

// 判断相象是否过河
bool onOwnSideForElephant(Side side, const Pos& p) {
    // 河界位于第 5 行与第 6 行之间（从 0 开始算）
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

// 车或炮直线路径是否被阻挡
bool clearPathRookLike(const BoardState& b, const Pos& a, const Pos& c) {
    // 起点与终点同行或同列，且中间无子
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

// 统计两点之间的棋子数
int countPiecesBetweenLine(const BoardState& b, const Pos& a, const Pos& c) {
    // 假定同行或同列
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

// 判断棋子对目标格的攻击性
bool attacksSquare(const BoardState& b, const Pos& from, Piece p, const Pos& target) {
    // 判断起点棋子是否攻击目标格（不考虑起点是否真有子）
    int dx = target.x - from.x;
    int dy = target.y - from.y;

    switch (p.type) {
        case PieceType::King: {
            // 将在九宫内走一步；“将帅照面”另行处理
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
            // 蹩马腿
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
            // 炮吃子需隔一个“炮架”
            return countPiecesBetweenLine(b, from, target) == 1;
        }
        case PieceType::Pawn: {
            int f = forwardDir(p.side);
            // 兵/卒总是向前攻击，过河后可横向
            if (dx == 0 && dy == f) return true;
            bool crossed = (p.side == Side::Red) ? (from.y >= 5) : (from.y <= 4);
            if (crossed && std::abs(dx) == 1 && dy == 0) return true;
            return false;
        }
    }
    return false;
}

// 生成伪合法走法（不考虑被将军）
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
                // 蹩马腿
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

// 执行一步并记录可撤销信息
void doMove(BoardState& b, const Move& m, Undo& u) {
    u.captured = b.at(m.to);
    b.at(m.to) = b.at(m.from);
    b.at(m.from) = std::nullopt;
}

// 撤销一步走子
void undoMove(BoardState& b, const Move& m, const Undo& u) {
    b.at(m.from) = b.at(m.to);
    b.at(m.to) = u.captured;
}

// 判断将帅是否照面
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

} // 匿名命名空间

namespace xiangqi {

// 初始化棋盘布局
BoardState initialBoard() {
    BoardState b;

    auto put = [&](int x, int y, Side side, PieceType type) {
        b.cells[y][x] = Piece{side, type};
    };

    // 红方（下方）
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

    // 黑方（上方）
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

// 判断坐标是否在棋盘内
bool inBounds(const Pos& p) {
    return (p.x >= 0 && p.x < WIDTH && p.y >= 0 && p.y < HEIGHT);
}

// 判断是否被将军
bool isInCheck(const BoardState& b, Side side) {
    auto k = findKing(b, side);
    if (!k) return false;

    // 将帅照面视为互相将军
    if (kingsFacing(b)) {
        // 将帅照面时双方均处于将军
        return true;
    }

    Side enemy = other(side);
    // 遍历敌方棋子，判断是否攻击己方将
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            const auto& cell = b.cells[y][x];
            if (!cell || cell->side != enemy) continue;
            Pos from{x, y};
            // 车/炮需要直线检查，攻击判定已处理
            if (attacksSquare(b, from, *cell, *k)) {
                // 特殊情况：炮的攻击依赖“隔子”规则
                // 将军判定中，炮按吃子规则判定，隔一子即可
                // 对车来说同理
                // 对炮而言，只要隔一子就可将军（不需目标占位）
                // 攻击判断已校验隔子数==1
                // 正常
                return true;
            }
        }
    }

    return false;
}

// 生成合法走法
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

// 获取全部合法走法
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

// 应用走法并返回被吃棋子
std::optional<Piece> applyMove(BoardState& b, const Move& m) {
    std::optional<Piece> cap = b.at(m.to);
    b.at(m.to) = b.at(m.from);
    b.at(m.from) = std::nullopt;
    return cap;
}

} // 象棋命名空间
