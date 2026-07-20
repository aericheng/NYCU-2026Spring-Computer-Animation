#include "terrain.h"

#include <stdexcept>

#include "../util/helper.h"
#include <iostream>

namespace simulation {
// Factory
std::unique_ptr<Terrain> TerrainFactory::CreateTerrain(TerrainType type) {
    switch (type) {
        case simulation::TerrainType::Plane:
            return std::make_unique<PlaneTerrain>();
        case simulation::TerrainType::Elevator:
            return std::make_unique<ElevatorTerrain>();

        default:
            throw std::invalid_argument("TerrainFactory::CreateTerrain : invalid TerrainType");
            break;
    }
    return nullptr;
}
// Terrain

Eigen::Matrix4f Terrain::getModelMatrix() { return modelMatrix; }

float Terrain::getMass() const { return mass; }

Eigen::Vector3f Terrain::getPosition() const { return position; }

Eigen::Vector3f Terrain::getVelocity() const { return velocity; }

Eigen::Vector3f Terrain::getAcceleration() const { return force / mass; }

Eigen::Vector3f Terrain::getForce() const { return force; }

void Terrain::setMass(const float _mass) { mass = _mass; }

void Terrain::setPosition(const Eigen::Vector3f& _position) {
    modelMatrix = util::translate(_position - position) * modelMatrix;
    position = _position;
}

void Terrain::setVelocity(const Eigen::Vector3f& _velocity) { velocity = _velocity; }

void Terrain::setAcceleration(const Eigen::Vector3f& _acceleration) { force = _acceleration * mass; }

void Terrain::setForce(const Eigen::Vector3f& _force) { force = _force; }

void Terrain::addPosition(const Eigen::Vector3f& _position) {
    position += _position;
    modelMatrix = util::translate(_position) * modelMatrix;
}

void Terrain::addVelocity(const Eigen::Vector3f& _velocity) { velocity += _velocity; }

void Terrain::addAcceleration(const Eigen::Vector3f& _acceleration) { force += _acceleration * mass; }

void Terrain::addForce(const Eigen::Vector3f& _force) { force += _force; }

// Note:
// You should update each particles' velocity (base on the equation in
// slide) and force (contact force : resist + friction) in handleCollision function

// PlaneTerrain //

PlaneTerrain::PlaneTerrain() { reset(); }

void PlaneTerrain::reset() {
    modelMatrix = util::translate(0.0f, position[1], 0.0f) * util::rotateDegree(0, 0, -20) * util::scale(30, 1, 30);
}


TerrainType PlaneTerrain::getType() { return TerrainType::Plane; }

void PlaneTerrain::handleCollision(const float delta_T, Jelly& jelly) {
    constexpr float eEPSILON = 0.01f;
    constexpr float coefResist = 0.4f; //0.8f
    constexpr float coefFriction = 0.75f;//0.5f
    // TODO#3-1: Handle collision when a particle collide with the plane terrain.
    //   If collision happens:
    //      1. Directly update particles' velocity
    //      2. Apply contact force to particles when needed
    // Note:
    //   1. There are `jelly.getParticleNum()` particles.
    //   2. See TODOs in `Jelly::computeInternalForce` and functions in `particle.h` if you don't know how to access
    //   data.
    //   3. The plane spans 30x30 units in the XZ plane and is rotated -20 degrees around the Z-axis.
    // Hint:
    //   1. Review "particles.pptx" from p.14 - p.19
    //   2. Use a.norm() to get length of a.
    //   3. Use a.normalize() to normalize a inplace.
    //          a.normalized() will create a new vector.
    //   4. Use a.dot(b) to get dot product of a and b.
    // normal 已定義在 header: (sin(20°), cos(20°), 0)
    // 平面過原點，所以 p = (0,0,0)
    Eigen::Vector3f N = normal.normalized();
    Eigen::Vector3f planePoint = Eigen::Vector3f::Zero();
    Eigen::Vector3f planeX(std::cos(util::PI<float>() / 9.0f), std::sin(util::PI<float>() / 9.0f), 0.0f);
    Eigen::Vector3f planeZ(0.0f, 0.0f, 1.0f);

    for (int i = 0; i < jelly.getParticleNum(); ++i) {
        Particle& p = jelly.getParticle(i);
        Eigen::Vector3f x = p.getPosition();
        Eigen::Vector3f v = p.getVelocity();
        Eigen::Vector3f f = p.getForce();

        float localX = planeX.dot(x - planePoint);
        float localZ = planeZ.dot(x - planePoint);
        if (std::abs(localX) > 15.0f || std::abs(localZ) > 15.0f) continue;

        float dist = N.dot(x - planePoint);
        float vn = N.dot(v);
        float fn = N.dot(f);

        if (dist < eEPSILON && vn < 0) {
            Eigen::Vector3f vN = vn * N;
            Eigen::Vector3f vT = v - vN;
            p.setVelocity(-coefResist * vN + vT);
            p.setPosition(x + (eEPSILON - dist) * N);
        }

        if (std::abs(dist) < eEPSILON && std::abs(vn) < eEPSILON && fn < 0) {
            Eigen::Vector3f contactForce = -fn * N;
            p.addForce(contactForce);

            Eigen::Vector3f vT = v - N.dot(v) * N;
            Eigen::Vector3f frictionForce = -coefFriction * (-fn) * vT;
            p.addForce(frictionForce);
        }
    }
}

ElevatorTerrain::ElevatorTerrain() {
    reset();
}

void ElevatorTerrain::reset() {
    modelMatrix = util::translate(0.0f, 1.0f, 0.0f) * util::rotateDegree(0, 0, 0) * util::scale(5, 1, 5);
    position = Eigen::Vector3f(0.0f, 1.0f, 0.0f);
    velocity = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
}

TerrainType ElevatorTerrain::getType() { return TerrainType::Elevator; }

void ElevatorTerrain::handleCollision(const float delta_T, Jelly& jelly) {
    constexpr float eEPSILON = 0.01f;
    constexpr float coefResist = 0.8f;
    constexpr float coefFriction = 0.5f;
}

}  // namespace simulation
