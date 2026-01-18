#pragma once

#include "Mesh.hpp"

namespace prim {

Mesh makeCube(float size = 1.0f);
Mesh makeDisc(float radius, int slices = 32);

} // namespace prim
