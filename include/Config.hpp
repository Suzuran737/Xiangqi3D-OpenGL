#pragma once

#include <string>

namespace cfg {

// 棋盘坐标系
// - X: 列 0..8（红方视角从左到右）
// - Y: 行 0..9（下到上；红方在 y=0）
// 世界坐标：X 水平，Y 向上，Z 朝黑方

inline constexpr float CELL = 1.0f;          // 相邻交叉点的距离
inline constexpr float BOARD_GRID_OFFSET_X = 0.0f;
inline constexpr float BOARD_GRID_OFFSET_Z = 0.0f;
inline constexpr float BOARD_PLANE_Y = 0.0f; // 棋盘表面高度

// 棋盘模型整体尺寸（包含边框）
inline constexpr float BOARD_MODEL_WIDTH = 8.0f * CELL + 2.0f;
inline constexpr float BOARD_MODEL_DEPTH = 9.0f * CELL + 2.0f;
inline constexpr bool BOARD_DRAW_GRID = false;

inline constexpr float PIECE_Y = 0.08f;      // 棋子底面高出棋盘的偏移
inline constexpr float PIECE_Y_OFFSET = 0.0f;

inline constexpr float PIECE_MODEL_SCALE = 0.8f;
inline constexpr bool BOARD_USE_ALBEDO = true;

inline constexpr float CAPTURE_ANIM_SECONDS = 0.35f;
inline constexpr float MOVE_ANIM_SECONDS = 0.28f;
inline constexpr float MOVE_LIFT_HEIGHT = 0.18f;

inline constexpr float BOARD_ROUGHNESS = 0.65f;
inline constexpr float BOARD_METALNESS = 0.05f;
inline constexpr float PIECE_ROUGHNESS = 0.45f;
inline constexpr float PIECE_METALNESS = 0.05f;

// 你提供的模型：
// 棋盘模型路径：assets/models/board/board.glb
// 棋子模型路径：assets/models/pieces/<color>_<type>.glb
//  - color: red | black
//  - type: king | advisor | elephant | horse | rook | cannon | pawn

inline const std::string BOARD_MODEL_GLB = "assets/models/board/board.glb";
inline const std::string BOARD_MODEL_GLTF = "assets/models/board/board.gltf";
inline const std::string BOARD_MODEL_OBJ = "assets/models/board/board.obj";

inline const std::string PIECES_DIR = "assets/models/pieces";

// UI 文本字体
inline const std::string FONT_PATH = "assets/fonts/NotoSansSC-Regular.otf";
// 菜单背景图
inline const std::string MENU_BG_TEXTURE = "assets/textures/bgc.png";
// “将军”提示图（透明背景）
inline const std::string CHECK_OVERLAY_TEXTURE = "assets/textures/check.png";
// 胜利提示图（透明背景）
inline const std::string RED_WIN_OVERLAY_TEXTURE = "assets/textures/win_red.png";
inline const std::string BLACK_WIN_OVERLAY_TEXTURE = "assets/textures/win_black.png";
// 游戏内背景（淡纹理）
inline const std::string GAME_BG_TEXTURE = "assets/textures/game_bg.png";
// 棋盘法线贴图（可选）
inline const std::string BOARD_NORMAL_MAP = "assets/textures/board_normal.png";

} // namespace cfg
