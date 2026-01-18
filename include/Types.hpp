#pragma once

#include <cstdint>
#include <string>

enum class Side : uint8_t {
    Red = 0,
    Black = 1,
};

inline const char* sideNameCN(Side s) {
    return (s == Side::Red) ? u8"\u7ea2\u65b9" : u8"\u9ed1\u65b9";
}

enum class PieceType : uint8_t {
    King,
    Advisor,
    Elephant,
    Horse,
    Rook,
    Cannon,
    Pawn,
};

struct Piece {
    Side side;
    PieceType type;
};

struct Pos {
    int x = 0; // 0..8
    int y = 0; // 0..9
};

inline bool operator==(const Pos& a, const Pos& b) {
    return a.x == b.x && a.y == b.y;
}

inline bool operator!=(const Pos& a, const Pos& b) {
    return !(a == b);
}

inline std::string pieceKey(Piece p) {
    // Used for model file lookups
    std::string color = (p.side == Side::Red) ? "black" : "red";
    std::string type;
    switch (p.type) {
        case PieceType::King: type = "king"; break;
        case PieceType::Advisor: type = "advisor"; break;
        case PieceType::Elephant: type = "elephant"; break;
        case PieceType::Horse: type = "horse"; break;
        case PieceType::Rook: type = "rook"; break;
        case PieceType::Cannon: type = "cannon"; break;
        case PieceType::Pawn: type = "pawn"; break;
    }
    return color + "_" + type;
}
