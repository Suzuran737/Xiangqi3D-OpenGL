#pragma once

#include <string>

namespace cfg {

// Board coordinate system
// - X: files 0..8 (left->right from red perspective)
// - Y: ranks 0..9 (bottom->top; red at y=0)
// World uses: X (horizontal), Y (up), Z (forward toward black)

inline constexpr float CELL = 1.0f;          // distance between two adjacent intersections
inline constexpr float BOARD_GRID_OFFSET_X = 0.0f;
inline constexpr float BOARD_GRID_OFFSET_Z = 0.0f;
inline constexpr float BOARD_PLANE_Y = 0.0f; // chessboard surface height

// Board model overall size (includes borders).
inline constexpr float BOARD_MODEL_WIDTH = 8.0f * CELL + 2.0f;
inline constexpr float BOARD_MODEL_DEPTH = 9.0f * CELL + 2.0f;
inline constexpr bool BOARD_DRAW_GRID = false;

inline constexpr float PIECE_Y = 0.08f;      // piece base offset above board
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

// Models you provide:
// Board model expected at: assets/models/board/board.glb
// Piece models expected at: assets/models/pieces/<color>_<type>.glb
//  - color: red | black
//  - type: king | advisor | elephant | horse | rook | cannon | pawn

inline const std::string BOARD_MODEL_GLB = "assets/models/board/board.glb";
inline const std::string BOARD_MODEL_GLTF = "assets/models/board/board.gltf";
inline const std::string BOARD_MODEL_OBJ = "assets/models/board/board.obj";

inline const std::string PIECES_DIR = "assets/models/pieces";

// Font for UI text rendering.
inline const std::string FONT_PATH = "assets/fonts/NotoSansSC-Regular.otf";
// Menu background image.
inline const std::string MENU_BG_TEXTURE = "assets/textures/bgc.png";
// "Check" overlay image (transparent background).
inline const std::string CHECK_OVERLAY_TEXTURE = "assets/textures/check.png";
// Win overlays (transparent background).
inline const std::string RED_WIN_OVERLAY_TEXTURE = "assets/textures/win_red.png";
inline const std::string BLACK_WIN_OVERLAY_TEXTURE = "assets/textures/win_black.png";
// Gameplay background (subtle texture).
inline const std::string GAME_BG_TEXTURE = "assets/textures/game_bg.png";
// Board normal map (optional).
inline const std::string BOARD_NORMAL_MAP = "assets/textures/board_normal.png";

} // namespace cfg
