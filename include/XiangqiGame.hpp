#pragma once

#include "Config.hpp"
#include "Types.hpp"
#include "XiangqiRules.hpp"

#include <optional>
#include <string>
#include <vector>

struct CaptureVisual {
    Piece piece;
    // 在该棋盘位置被吃（移除前）
    Pos pos;
    float t = 0.0f;
    float duration = cfg::CAPTURE_ANIM_SECONDS;
};

struct MoveVisual {
    Piece piece;
    Pos from;
    Pos to;
    float t = 0.0f;
    float duration = cfg::MOVE_ANIM_SECONDS;
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
    const std::vector<MoveVisual>& moves() const { return m_moves; }

    float timeSeconds() const { return m_time; }
    bool helpActive() const { return m_helpTimer > 0.0f; }
    void startHelp(float seconds);
    bool checkFlashActive() const { return m_checkFlashTimer > 0.0f; }
    bool resultOverlayActive() const { return m_resultTimer > 0.0f; }
    bool resultPromptActive() const { return m_status != GameStatus::Ongoing && m_resultTimer <= 0.0f; }
    Side winnerSide() const { return (m_status == GameStatus::RedWin) ? Side::Red : Side::Black; }

    bool inCheck(Side s) const;

    // 用户点击棋盘交点；如游戏状态改变则返回 true
    bool clickAt(const Pos& p);

    // 动画更新
    void update(float dt);

    // 界面文本
    std::string statusTextCN() const;

    // 临时/重要提示（将军/将死）
    // - 对局中：在最后一步后短暂显示
    // - 结束：永久显示
    std::string eventTextCN() const;

    // 窗口标题后缀（即使缺少字体也可用）
    std::string windowTitleCN() const;

private:
    BoardState m_board;
    Side m_sideToMove = Side::Red;
    GameStatus m_status = GameStatus::Ongoing;

    std::optional<Pos> m_selected;
    std::vector<Pos> m_legalTargets;

    std::vector<CaptureVisual> m_captures;
    std::vector<MoveVisual> m_moves;

    float m_time = 0.0f;
    float m_helpTimer = 0.0f;
    float m_checkFlashTimer = 0.0f;
    float m_resultTimer = 0.0f;

    // 上一次事件提示（将军/将死）；对局中会逐渐淡出
    std::string m_eventText;
    float m_eventTimer = 0.0f; // 剩余秒数；<0 表示永久

    void computeLegalTargets();
    void afterMove();
};
