#pragma once

#include "Mesh.hpp"

// 基础几何体生成
namespace prim {

Mesh makeCube(float size = 1.0f);   // 立方体
Mesh makeDisc(float radius, int slices = 32); // 圆盘

} // namespace prim
