#include "integrator.h"
#include <iostream>
#include <cmath>
#include <vector>
namespace simulation {
// Factory
std::unique_ptr<Integrator> IntegratorFactory::CreateIntegrator(IntegratorType type) {
    switch (type) {
        case simulation::IntegratorType::ExplicitEuler:
            return std::make_unique<ExplicitEulerIntegrator>();
        case simulation::IntegratorType::ImplicitEuler:
            return std::make_unique<ImplicitEulerIntegrator>();
        case simulation::IntegratorType::MidpointEuler:
            return std::make_unique<MidpointEulerIntegrator>();
        case simulation::IntegratorType::RungeKuttaFourth:
            return std::make_unique<RungeKuttaFourthIntegrator>();
        case simulation::IntegratorType::PositionBasedDynamic:
            return std::make_unique<PositionBasedDynamicIntegrator>();
        default:
            throw std::invalid_argument("TerrainFactory::CreateTerrain : invalid TerrainType");
            break;
    }
    return nullptr;
}

//
// ExplicitEulerIntegrator
//

IntegratorType ExplicitEulerIntegrator::getType() { return IntegratorType::ExplicitEuler; }


void ExplicitEulerIntegrator::integrate(MassSpringSystem& particleSystem) {
    // TODO#4-1: Integrate position and velocity
    //   1. Integrate position using current velocity.
    //   2. Integrate velocity using current acceleration.
    //   3. Clear force
    // Note:
    //   1. You should do this first because it is very simple. Then you can check whether your collision is correct or
    //   not.
    //   2. See functions in `particle.h` if you don't know how to access data.

    //   3. Review "ODE_basics.pptx" from p.15 - p.16
    float dt = particleSystem.deltaTime;

    for (int j = 0; j < particleSystem.getJellyCount(); ++j) {
        Jelly* jelly = particleSystem.getJellyPointer(j);
        int numParticles = jelly->getParticleNum();
        
        for (int i = 0; i < numParticles; ++i) {
            Particle& p = jelly->getParticle(i);       
            p.setPosition(p.getPosition() + p.getVelocity() * dt);
            p.setVelocity(p.getVelocity() + p.getAcceleration() * dt);
            p.setForce(Eigen::Vector3f::Zero()); 
        }
    }
}

//
// ImplicitEulerIntegrator
//

IntegratorType ImplicitEulerIntegrator::getType() { return IntegratorType::ImplicitEuler; }

void ImplicitEulerIntegrator::integrate(MassSpringSystem& particleSystem) {
    // TODO#4-2: Integrate position and velocity
    //   1. Backup original particles' data.
    //   2. Integrate position and velocity using explicit euler to get Xn+1.
    //   3. Compute refined Xn+1 and Vn+1 using (1.) and (2.).
    // Note:
    //   1. Use `MassSpringSystem::computeJellyForce`
    //      with modified position and velocity to get Xn+1.
    //   2. See functions in `particle.h` if you don't know how to access data.
    //   3. Review "ODE_implicit.pptx" from p.18 - p.19

    float dt = particleSystem.deltaTime;

    for (int j = 0; j < particleSystem.getJellyCount(); ++j) {
        Jelly* jelly = particleSystem.getJellyPointer(j);
        int n = jelly->getParticleNum();

        for (int i = 0; i < n; ++i) {
            Particle& p = jelly->getParticle(i);
            p.setVelocity(p.getVelocity() + p.getAcceleration() * dt);
            p.setPosition(p.getPosition() + p.getVelocity() * dt);
            p.setForce(Eigen::Vector3f::Zero());
        }
    }
}

//
// MidpointEulerIntegrator
//

IntegratorType MidpointEulerIntegrator::getType() { return IntegratorType::MidpointEuler; }

void MidpointEulerIntegrator::integrate(MassSpringSystem& particleSystem) {}

//
// RungeKuttaFourthIntegrator
//

IntegratorType RungeKuttaFourthIntegrator::getType() { return IntegratorType::RungeKuttaFourth; }

void RungeKuttaFourthIntegrator::integrate(MassSpringSystem& particleSystem) {
    // TODO#Bonus: Integrate velocity and acceleration
    //   1. Backup original particles' data.
    //   2. Compute k1, k2, k3, k4
    //   3. Compute refined Xn+1 using (1.) and (2.).
    // Note:
    //   1. Use `MassSpringSystem::computeJellyForce`
    //      with modified position and velocity to get Xn+1.
    //   2. See functions in `particle.h` if you don't know how to access data.
    //   3. StateStep struct is just a hint, you can use whatever you want.
    //   4. Review "ODE_basics.pptx" from p.21

    const float dt = particleSystem.deltaTime;

    for (int j = 0; j < particleSystem.getJellyCount(); ++j) {
        Jelly* jelly = particleSystem.getJellyPointer(j);
        int n = jelly->getParticleNum();

        std::vector<Eigen::Vector3f> origPos(n), origVel(n);
        for (int i = 0; i < n; ++i) {
            Particle& p = jelly->getParticle(i);
            origPos[i] = p.getPosition();
            origVel[i] = p.getVelocity();
        }

        std::vector<Eigen::Vector3f> k1v(n), k1a(n);
        for (int i = 0; i < n; ++i) {
            Particle& p = jelly->getParticle(i);
            k1v[i] = p.getVelocity();
            k1a[i] = p.getAcceleration();
        }

        for (int i = 0; i < n; ++i) {
            Particle& p = jelly->getParticle(i);
            p.setPosition(origPos[i] + 0.5f * dt * k1v[i]);
            p.setVelocity(origVel[i] + 0.5f * dt * k1a[i]);
        }
        particleSystem.computeJellyForce(*jelly);
        std::vector<Eigen::Vector3f> k2v(n), k2a(n);
        for (int i = 0; i < n; ++i) {
            Particle& p = jelly->getParticle(i);
            k2v[i] = p.getVelocity();
            k2a[i] = p.getAcceleration();
        }

        for (int i = 0; i < n; ++i) {
            Particle& p = jelly->getParticle(i);
            p.setPosition(origPos[i] + 0.5f * dt * k2v[i]);
            p.setVelocity(origVel[i] + 0.5f * dt * k2a[i]);
        }
        particleSystem.computeJellyForce(*jelly);
        std::vector<Eigen::Vector3f> k3v(n), k3a(n);
        for (int i = 0; i < n; ++i) {
            Particle& p = jelly->getParticle(i);
            k3v[i] = p.getVelocity();
            k3a[i] = p.getAcceleration();
        }

        for (int i = 0; i < n; ++i) {
            Particle& p = jelly->getParticle(i);
            p.setPosition(origPos[i] + dt * k3v[i]);
            p.setVelocity(origVel[i] + dt * k3a[i]);
        }
        particleSystem.computeJellyForce(*jelly);
        std::vector<Eigen::Vector3f> k4v(n), k4a(n);
        for (int i = 0; i < n; ++i) {
            Particle& p = jelly->getParticle(i);
            k4v[i] = p.getVelocity();
            k4a[i] = p.getAcceleration();
        }

        for (int i = 0; i < n; ++i) {
            Particle& p = jelly->getParticle(i);
            Eigen::Vector3f newPos = origPos[i] + (dt / 6.0f) * (k1v[i] + 2.0f*k2v[i] + 2.0f*k3v[i] + k4v[i]);
            Eigen::Vector3f newVel = origVel[i] + (dt / 6.0f) * (k1a[i] + 2.0f*k2a[i] + 2.0f*k3a[i] + k4a[i]);
            p.setPosition(newPos);
            p.setVelocity(newVel);
            p.setForce(Eigen::Vector3f::Zero());
        }
    }
}

IntegratorType PositionBasedDynamicIntegrator::getType() { return IntegratorType::PositionBasedDynamic; }

void PositionBasedDynamicIntegrator::integrate(MassSpringSystem& particleSystem) {
    // TODO#4-3: Integrate position and velocity
    //   1. Prediction Step
    //   2. Constraint Projection
    //   3. Position and Velocity Update with XSPH Viscosity

    const float dt = particleSystem.deltaTime;
    const int solverIterations = 4; //4
    const float xsphFactor = 0.1f; //0.1

    for (int j = 0; j < particleSystem.getJellyCount(); ++j) {
        Jelly* jelly = particleSystem.getJellyPointer(j);
        int n = jelly->getParticleNum();
        int numSprings = jelly->getSpringNum();
        int numPerEdge = jelly->getNumAtEdge();
        float cellSize = jelly->getLength() / (numPerEdge - 1);
        float minDist = cellSize * 0.75f;
        float maxDist = cellSize * 1.25f;

        for (int i = 0; i < n; ++i) {
            Particle& p = jelly->getParticle(i);
            Eigen::Vector3f newVel = p.getVelocity() + p.getAcceleration() * dt;
            Eigen::Vector3f newPos = p.getPosition() + newVel * dt;
            p.setVelocity(newVel);
            p.setPredictedPosition(newPos);
        }

        int numPerFace = numPerEdge * numPerEdge;

        for (int iter = 0; iter < solverIterations; ++iter) {
            Eigen::Vector3f planeNormal = particleSystem.planeTerrain->getNormal();
            Eigen::Vector3f planePoint(0.0f, 0.0f, 0.0f);
            constexpr float epsilon = 0.01f;

            for (int i = 0; i < n; ++i) {
                Particle& p = jelly->getParticle(i);
                Eigen::Vector3f pred = p.getPredictedPosition();
                float dist = planeNormal.dot(pred - planePoint);
                if (dist < epsilon) {
                    p.setPredictedPosition(pred + (epsilon - dist) * planeNormal);
                }
            }

            for (int idx = 0; idx < n; ++idx) {
                int ii = idx / numPerFace;
                int jj = (idx / numPerEdge) % numPerEdge;
                int kk = idx % numPerEdge;

                for (int di = -1; di <= 1; ++di) {
                    for (int dj = -1; dj <= 1; ++dj) {
                        for (int dk = -1; dk <= 1; ++dk) {
                            if (di == 0 && dj == 0 && dk == 0) continue;
                            int ni = ii + di, nj = jj + dj, nk = kk + dk;
                            if (ni < 0 || ni >= numPerEdge ||
                                nj < 0 || nj >= numPerEdge ||
                                nk < 0 || nk >= numPerEdge) continue;

                            int nIdx = ni * numPerFace + nj * numPerEdge + nk;
                            if (nIdx <= idx) continue;

                            Particle& pA = jelly->getParticle(idx);
                            Particle& pB = jelly->getParticle(nIdx);
                            Eigen::Vector3f posA = pA.getPredictedPosition();
                            Eigen::Vector3f posB = pB.getPredictedPosition();
                            Eigen::Vector3f diff = posA - posB;
                            float dist = diff.norm();

                            if (dist < 1e-8f) continue;

                            float neighborDist = std::sqrt((float)(di*di + dj*dj + dk*dk));
                            float targetMin = minDist * neighborDist;
                            float targetMax = maxDist * neighborDist;

                            if (dist < targetMin) {
                                float correction = (targetMin - dist) * 0.5f;
                                Eigen::Vector3f dir = diff.normalized();
                                pA.setPredictedPosition(posA + correction * dir);
                                pB.setPredictedPosition(posB - correction * dir);
                            } else if (dist > targetMax) {
                                float correction = (dist - targetMax) * 0.5f;
                                Eigen::Vector3f dir = diff.normalized();
                                pA.setPredictedPosition(posA - correction * dir);
                                pB.setPredictedPosition(posB + correction * dir);
                            }
                        }
                    }
                }
            }
        }

        for (int i = 0; i < n; ++i) {
            Particle& p = jelly->getParticle(i);
            Eigen::Vector3f newVel = (p.getPredictedPosition() - p.getPosition()) / dt;
            p.setVelocity(newVel);
            p.setPosition(p.getPredictedPosition());
        }

        std::vector<Eigen::Vector3f> velCorrections(n, Eigen::Vector3f::Zero());

        for (int idx = 0; idx < n; ++idx) {
            Particle& pP = jelly->getParticle(idx);
            int ii = idx / numPerFace;
            int jj = (idx / numPerEdge) % numPerEdge;
            int kk = idx % numPerEdge;

            float weightSum = 0.0f;
            Eigen::Vector3f velCorr = Eigen::Vector3f::Zero();

            for (int di = -1; di <= 1; ++di) {
                for (int dj = -1; dj <= 1; ++dj) {
                    for (int dk = -1; dk <= 1; ++dk) {
                        if (di == 0 && dj == 0 && dk == 0) continue;
                        int ni = ii + di, nj = jj + dj, nk = kk + dk;
                        if (ni < 0 || ni >= numPerEdge ||
                            nj < 0 || nj >= numPerEdge ||
                            nk < 0 || nk >= numPerEdge) continue;

                        int nIdx = ni * numPerFace + nj * numPerEdge + nk;
                        Particle& pQ = jelly->getParticle(nIdx);

                        float d = (pP.getPosition() - pQ.getPosition()).norm();
                        if (d < 1e-8f) continue;
                        float w = 1.0f / d;
                        weightSum += w;
                        velCorr += w * (pQ.getVelocity() - pP.getVelocity());
                    }
                }
            }

            if (weightSum > 0.0f) {
                velCorrections[idx] = xsphFactor * velCorr / weightSum;
            }
        }

        for (int i = 0; i < n; ++i) {
            Particle& p = jelly->getParticle(i);
            p.setVelocity(p.getVelocity() + velCorrections[i]);
            p.setForce(Eigen::Vector3f::Zero());
        }
    }
}

}  // namespace simulation
