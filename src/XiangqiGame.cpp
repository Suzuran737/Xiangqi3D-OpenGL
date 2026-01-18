#include "XiangqiGame.hpp"

#include "Util.hpp"

#include <algorithm>

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

    m_eventText.clear();
    m_eventTimer = 0.0f;
}

bool XiangqiGame::inCheck(Side s) const {
    return xiangqi::isInCheck(m_board, s);
}

void XiangqiGame::computeLegalTargets() {
    m_legalTargets.clear();
    if (!m_selected) return;
    auto ms = xiangqi::legalMovesFrom(m_board, *m_selected, m_sideToMove);
    m_legalTargets.reserve(ms.size());
    for (const auto& m : ms) m_legalTargets.push_back(m.to);
}

bool XiangqiGame::clickAt(const Pos& p) {
    if (m_status != GameStatus::Ongoing) {
        return false;
    }
    if (!xiangqi::inBounds(p)) return false;

    const auto& cell = m_board.at(p);

    // Select phase
    if (!m_selected) {
        if (cell && cell->side == m_sideToMove) {
            m_selected = p;
            computeLegalTargets();
            return true;
        }
        return false;
    }

    // If click same cell: deselect
    if (*m_selected == p) {
        m_selected.reset();
        m_legalTargets.clear();
        return true;
    }

    // If click own piece: change selection
    if (cell && cell->side == m_sideToMove) {
        m_selected = p;
        computeLegalTargets();
        return true;
    }

    // Attempt move if target is legal
    bool isLegalTarget = std::any_of(m_legalTargets.begin(), m_legalTargets.end(), [&](const Pos& t) {
        return t == p;
    });
    if (!isLegalTarget) {
        // Keep selection
        return false;
    }

    Move m{*m_selected, p};
    // record capture visual (if any)
    if (m_board.at(p).has_value()) {
        CaptureVisual cv{*m_board.at(p), p, 0.0f, cfg::CAPTURE_ANIM_SECONDS};
        m_captures.push_back(cv);
    }

    xiangqi::applyMove(m_board, m);

    // End selection and switch turn
    m_selected.reset();
    m_legalTargets.clear();

    m_sideToMove = other(m_sideToMove);
    afterMove();
    return true;
}

void XiangqiGame::afterMove() {
    // After switching side-to-move, evaluate:
    // 1) Checkmate/stalemate (side-to-move has no legal moves -> loses)
    // 2) Check
    const bool stmInCheck = xiangqi::isInCheck(m_board, m_sideToMove);
    auto ms = xiangqi::allLegalMoves(m_board, m_sideToMove);

    auto setEvent = [&](std::string text, float seconds) {
        // Avoid spamming logs if the same message repeats.
        if (text != m_eventText) {
            if (!text.empty()) {
                util::logInfo(text);
            }
            m_eventText = std::move(text);
        }
        m_eventTimer = seconds;
    };

    if (ms.empty()) {
        // In Xiangqi, "no legal moves" is a loss (whether checked or not).
        Side winner = (m_sideToMove == Side::Red) ? Side::Black : Side::Red;
        m_status = (winner == Side::Red) ? GameStatus::RedWin : GameStatus::BlackWin;

        if (stmInCheck) {
            setEvent(std::string("Checkmate. ") + sideNameCN(winner) + " wins. (Press R to restart)", -1.0f);
        } else {
            setEvent(std::string("Stalemate. ") + sideNameCN(winner) + " wins. (Press R to restart)", -1.0f);
        }
        return;
    }

    // Ongoing: show a short prompt when side-to-move is in check.
    if (stmInCheck) {
        setEvent(std::string(sideNameCN(other(m_sideToMove))) + " gives check.", 2.0f);
    } else {
        setEvent("", 0.0f);
    }
}

void XiangqiGame::update(float dt) {
    // Update capture visuals
    for (auto& c : m_captures) {
        c.t += dt;
    }
    m_captures.erase(
        std::remove_if(m_captures.begin(), m_captures.end(), [](const CaptureVisual& c) {
            return c.t >= c.duration;
        }),
        m_captures.end()
    );

    // Fade out transient event prompt.
    if (m_status == GameStatus::Ongoing && m_eventTimer > 0.0f) {
        m_eventTimer -= dt;
        if (m_eventTimer <= 0.0f) {
            m_eventTimer = 0.0f;
            m_eventText.clear();
        }
    }
}

std::string XiangqiGame::statusTextCN() const {
    if (m_status == GameStatus::RedWin) return "Red wins";
    if (m_status == GameStatus::BlackWin) return "Black wins";

    std::string s = std::string(sideNameCN(m_sideToMove)) + " to move";
    if (xiangqi::isInCheck(m_board, m_sideToMove)) {
        s += " (in check)";
    }
    return s;
}

std::string XiangqiGame::eventTextCN() const {
    if (m_status != GameStatus::Ongoing) {
        return m_eventText; // permanent
    }
    if (m_eventTimer > 0.0f && !m_eventText.empty()) {
        return m_eventText;
    }
    return {};
}

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