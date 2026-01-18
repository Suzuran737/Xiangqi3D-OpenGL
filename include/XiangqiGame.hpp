#pragma once

#include "Config.hpp"
#include "Types.hpp"
#include "XiangqiRules.hpp"

#include <optional>
#include <string>
#include <vector>

struct CaptureVisual {
    Piece piece;
    // captured at this board position (before removal)
    Pos pos;
    float t = 0.0f;
    float duration = cfg::CAPTURE_ANIM_SECONDS;
};

enum class GameStatus {
    Ongoing,
    RedWin,
    BlackWin,
};

class XiangqiGame {
public:
    XiangqiGame();

    void reset();

    Side sideToMove() const { return m_sideToMove; }
    GameStatus status() const { return m_status; }

    const BoardState& board() const { return m_board; }

    std::optional<Pos> selected() const { return m_selected; }
    const std::vector<Pos>& legalTargets() const { return m_legalTargets; }

    const std::vector<CaptureVisual>& captures() const { return m_captures; }

    bool inCheck(Side s) const;

    // User clicks a board intersection. Returns true if game state changed.
    bool clickAt(const Pos& p);

    // Animation updates
    void update(float dt);

    // UI message
    std::string statusTextCN() const;

    // Transient/important prompt (check / mate).
    // - Ongoing: shown for a short time after the last move.
    // - GameOver: shown permanently.
    std::string eventTextCN() const;

    // Suggested window title suffix (works even when font is missing)
    std::string windowTitleCN() const;

private:
    BoardState m_board;
    Side m_sideToMove = Side::Red;
    GameStatus m_status = GameStatus::Ongoing;

    std::optional<Pos> m_selected;
    std::vector<Pos> m_legalTargets;

    std::vector<CaptureVisual> m_captures;

    // last event prompt (check / mate). For ongoing games, this fades out.
    std::string m_eventText;
    float m_eventTimer = 0.0f; // seconds remaining; <0 means permanent

    void computeLegalTargets();
    void afterMove();
};
