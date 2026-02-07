#pragma once

#include <memory>
#include "Mesh.h"

namespace GeometryFactory {

std::unique_ptr<Mesh> createCube();
std::unique_ptr<Mesh> createSphere(int segments = 32, int rings = 16);
std::unique_ptr<Mesh> createTorus(float majorRadius = 1.0f, float minorRadius = 0.3f,
                                  int segments = 32, int rings = 16);
std::unique_ptr<Mesh> createPlane(float size = 10.0f);
std::unique_ptr<Mesh> createQuad();
std::unique_ptr<Mesh> createWeaponMesh();

}
