#include "XiangqiGame.hpp"

#include "Util.hpp"

#include <algorithm>

// 获取对手阵营
static Side other(Side s) {
    return (s == Side::Red) ? Side::Black : Side::Red;
}

XiangqiGame::XiangqiGame() {
    reset();
}

void XiangqiGame::reset() {
    m_board = xiangqi::initialBoard();
    m_sideToMove = Side::Red;
    m_status = GameStatus::Ongoing;
    m_selected.reset();
    m_legalTargets.clear();
    m_captures.clear();
    m_moves.clear();

    m_eventText.clear();
    m_eventTimer = 0.0f;
    m_time = 0.0f;
    m_helpTimer = 0.0f;
    m_checkFlashTimer = 0.0f;
    m_resultTimer = 0.0f;
}

bool XiangqiGame::inCheck(Side s) const {
    return xiangqi::isInCheck(m_board, s);
}

// 计算选中棋子的合法落点
void XiangqiGame::computeLegalTargets() {
    m_legalTargets.clear();
    if (!m_selected) return;
    auto ms = xiangqi::legalMovesFrom(m_board, *m_selected, m_sideToMove);
    m_legalTargets.reserve(ms.size());
    for (const auto& m : ms) m_legalTargets.push_back(m.to);
}

// 处理棋盘点击交互
bool XiangqiGame::clickAt(const Pos& p) {
    if (m_status != GameStatus::Ongoing) {
        return false;
    }
    if (!xiangqi::inBounds(p)) return false;

    const auto& cell = m_board.at(p);

    // 选择阶段
    if (!m_selected) {
        if (cell && cell->side == m_sideToMove) {
            m_selected = p;
            computeLegalTargets();
            return true;
        }
        return false;
    }

    // 点击同一格：取消选中
    if (*m_selected == p) {
        m_selected.reset();
        m_legalTargets.clear();
        return true;
    }

    // 点击己方棋子：切换选中
    if (cell && cell->side == m_sideToMove) {
        m_selected = p;
        computeLegalTargets();
        return true;
    }

    // 目标合法则尝试走子
    bool isLegalTarget = std::any_of(m_legalTargets.begin(), m_legalTargets.end(), [&](const Pos& t) {
        return t == p;
    });
    if (!isLegalTarget) {
        // 保持选中
        return false;
    }

    Piece moving = *m_board.at(*m_selected);
    Move m{*m_selected, p};
    // 记录吃子动画（如有）
    if (m_board.at(p).has_value()) {
        CaptureVisual cv{*m_board.at(p), p, 0.0f, cfg::CAPTURE_ANIM_SECONDS};
        m_captures.push_back(cv);
    }

    xiangqi::applyMove(m_board, m);
    m_moves.push_back(MoveVisual{moving, m.from, m.to, 0.0f, cfg::MOVE_ANIM_SECONDS});

    // 结束选中并切换回合
    m_selected.reset();
    m_legalTargets.clear();

    m_sideToMove = other(m_sideToMove);
    afterMove();
    return true;
}

// 走子后更新胜负与提示
void XiangqiGame::afterMove() {
    // 切换走子方后进行判定：
    // 1) 将死/困毙（走子方无合法走法即失败）
    // 2) 将军
    const bool stmInCheck = xiangqi::isInCheck(m_board, m_sideToMove);
    auto ms = xiangqi::allLegalMoves(m_board, m_sideToMove);

    auto setEvent = [&](std::string text, float seconds) {
        // 避免相同提示重复刷屏
        if (text != m_eventText) {
            if (!text.empty()) {
                util::logInfo(text);
            }
            m_eventText = std::move(text);
        }
        m_eventTimer = seconds;
    };

    if (ms.empty()) {
        // 象棋中，“无合法走法”即判负（无论是否被将军）
        Side winner = (m_sideToMove == Side::Red) ? Side::Black : Side::Red;
        m_status = (winner == Side::Red) ? GameStatus::RedWin : GameStatus::BlackWin;
        m_resultTimer = 1.5f;

        if (stmInCheck) {
            setEvent(std::string("Checkmate. ") + sideNameCN(winner) + " wins. (Press R to restart)", -1.0f);
        } else {
            setEvent(std::string("Stalemate. ") + sideNameCN(winner) + " wins. (Press R to restart)", -1.0f);
        }
        return;
    }

    // 对局中：走子方被将军时短暂提示
    if (stmInCheck) {
        setEvent(std::string(sideNameCN(other(m_sideToMove))) + " gives check.", 2.0f);
        m_checkFlashTimer = 1.5f;
    } else {
        setEvent("", 0.0f);
    }
}

// 更新动画与计时器
void XiangqiGame::update(float dt) {
    // 更新吃子动画
    for (auto& c : m_captures) {
        c.t += dt;
    }
    m_captures.erase(
        std::remove_if(m_captures.begin(), m_captures.end(), [](const CaptureVisual& c) {
            return c.t >= c.duration;
        }),
        m_captures.end()
    );

    // 更新走子动画
    for (auto& mv : m_moves) {
        mv.t += dt;
    }
    m_moves.erase(
        std::remove_if(m_moves.begin(), m_moves.end(), [](const MoveVisual& mv) {
            return mv.t >= mv.duration;
        }),
        m_moves.end()
    );

    m_time += dt;
    if (m_helpTimer > 0.0f) {
        m_helpTimer -= dt;
        if (m_helpTimer < 0.0f) {
            m_helpTimer = 0.0f;
        }
    }
    if (m_checkFlashTimer > 0.0f) {
        m_checkFlashTimer -= dt;
        if (m_checkFlashTimer < 0.0f) {
            m_checkFlashTimer = 0.0f;
        }
    }

    // 淡出临时事件提示
    if (m_status == GameStatus::Ongoing && m_eventTimer > 0.0f) {
        m_eventTimer -= dt;
        if (m_eventTimer <= 0.0f) {
            m_eventTimer = 0.0f;
            m_eventText.clear();
        }
    }

    if (m_resultTimer > 0.0f) {
        m_resultTimer -= dt;
        if (m_resultTimer < 0.0f) {
            m_resultTimer = 0.0f;
        }
    }
}

void XiangqiGame::startHelp(float seconds) {
    if (seconds <= 0.0f) return;
    m_helpTimer = seconds;
}

// 生成当前回合提示
std::string XiangqiGame::statusTextCN() const {
    if (m_status == GameStatus::RedWin) return u8"\u7ea2\u65b9\u80dc";
    if (m_status == GameStatus::BlackWin) return u8"\u9ed1\u65b9\u80dc";

    std::string s = std::string(sideNameCN(m_sideToMove)) + u8"\u8d70\u68cb";
    if (xiangqi::isInCheck(m_board, m_sideToMove)) {
        s += u8" (被将军)";
    }
    return s;
}

// 生成事件提示文本
std::string XiangqiGame::eventTextCN() const {
    if (m_status != GameStatus::Ongoing) {
        return m_eventText; // 永久
    }
    if (m_eventTimer > 0.0f && !m_eventText.empty()) {
        return m_eventText;
    }
    return {};
}

// 生成窗口标题信息
std::string XiangqiGame::windowTitleCN() const {
    std::string s;

    auto evt = eventTextCN();
    if (!evt.empty()) {
        s += evt;
        s += "  ";
    }
    s += statusTextCN();
    if (m_status != GameStatus::Ongoing) {
        s += "  (Press R to restart)";
    }
    return s;
}
