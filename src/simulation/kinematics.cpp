#include "simulation/kinematics.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stack>
#include "Eigen/Dense"
#include "acclaim/bone.h"
#include "util/helper.h"

namespace kinematics {
namespace {
constexpr double kConvergeEpsilon = 1e-3;
constexpr double kTiny = 1e-6;
constexpr int kMaxIteration = 1000;
constexpr double kJacobianIKStepSize = 0.3;
constexpr double kCCDIKStepSize = 0.3;
// distal=1.0, base=kJointWeightMin（linear interp）
constexpr double kJointWeightMin = 0.35;

// UI 顯示用
int gLastIterations = 0;
double gLastResidual = 0.0;

struct IKChainContext {
    Eigen::Vector4d target;                 // The position where `end_bone` (idx = 0 in the chain) will move to.
    acclaim::Bone* rootBone;                // Pointer to the root bone.
    acclaim::Posture& posture;              // The original AMC motion's reference, you need to modify this.
    JointChain jointChain;                  // 1D vector which holds pointers to Eigen::Vector4d constituting a chain.
    BoneChain boneChain;                    // 1D vector which holds pointers to acclaim::Bone constituting a chain.
    const Eigen::Vector4d& currentBasePos;  // The base of the current chain.
    int chainIdx = 0;                       // Chain index of the current chain.
    int boneNum = 0;                        // Number of bones in the chain.
};

bool IKConverged(const std::vector<Eigen::Vector4d>& targets, JointChains& jointChains) {
    const std::size_t chainCount = std::min(targets.size(), jointChains.size());
    for (std::size_t i = 0; i < chainCount; ++i) {
        if (jointChains[i].empty() || jointChains[i][0] == nullptr) continue;
        Eigen::Vector4d diff = targets[i] - *jointChains[i][0];
        if (diff.head<3>().norm() > kConvergeEpsilon) {
            return false;
        }
    }
    return true;
}

Eigen::Vector4d clampBoneRotation(Eigen::Vector4d rotation, const acclaim::Bone* bone) {
    Eigen::Vector4d clamped = rotation;
    if (!bone->dofrx) {
        clamped[0] = 0.0;
    } else if (bone->rxmin != bone->rxmax) {
        clamped[0] = std::clamp(clamped[0],
                                static_cast<double>(bone->rxmin),
                                static_cast<double>(bone->rxmax));
    }
    if (!bone->dofry) {
        clamped[1] = 0.0;
    } else if (bone->rymin != bone->rymax) {
        clamped[1] = std::clamp(clamped[1],
                                static_cast<double>(bone->rymin),
                                static_cast<double>(bone->rymax));
    }
    if (!bone->dofrz) {
        clamped[2] = 0.0;
    } else if (bone->rzmin != bone->rzmax) {
        clamped[2] = std::clamp(clamped[2],
                                static_cast<double>(bone->rzmin),
                                static_cast<double>(bone->rzmax));
    }
    clamped[3] = 0.0;
    return clamped;
}

Eigen::VectorXd pseudoInverseLinearSolver(const Eigen::Matrix4Xd& Jacobian, const Eigen::Vector4d& target) {
    // DLS: x = V * diag(s/(s^2+lambda^2)) * U^T * b
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(Jacobian, Eigen::ComputeThinU | Eigen::ComputeThinV);
    const Eigen::VectorXd& singularValues = svd.singularValues();
    constexpr double lambda2 = 1e-4;
    Eigen::VectorXd dampedInv(singularValues.size());
    for (Eigen::Index i = 0; i < singularValues.size(); ++i) {
        const double s = singularValues[i];
        dampedInv[i] = s / (s * s + lambda2);
    }
    return svd.matrixV() * dampedInv.asDiagonal() * svd.matrixU().transpose() * target;
}

inline Eigen::Affine3d jointFrameOf(const acclaim::Bone* bone) {
    if (bone->parent != nullptr) {
        return bone->parent->rotation * bone->rot_parent_current;
    }
    return bone->rot_parent_current;
}

inline void matrixToEulerZYX(const Eigen::Matrix3d& R, double& x_rad, double& y_rad, double& z_rad) {
    const double sy = std::clamp(-R(2, 0), -1.0, 1.0);
    y_rad = std::asin(sy);
    if (std::abs(std::cos(y_rad)) > kTiny) {
        x_rad = std::atan2(R(2, 1), R(2, 2));
        z_rad = std::atan2(R(1, 0), R(0, 0));
    } else {
        x_rad = std::atan2(-R(1, 2), R(1, 1));
        z_rad = 0.0;
    }
}

// distal=1.0, base=kJointWeightMin
inline double jointWeight(int boneIdxInChain, int boneNum) {
    if (boneNum <= 1) return 1.0;
    const double t = static_cast<double>(boneIdxInChain) / static_cast<double>(boneNum - 1);
    return 1.0 - (1.0 - kJointWeightMin) * t;
}

// 把 world rotation 投到 local、寫回 posture、clamp 後回傳實際 applied 的 world rotation
void applyWorldRotationToBone(acclaim::Bone* bone, const Eigen::Quaterniond& q_world,
                              acclaim::Posture& posture,
                              Eigen::Quaterniond& appliedWorldOut) {
    Eigen::Quaterniond jf_q(jointFrameOf(bone).linear());
    jf_q.normalize();
    const Eigen::Quaterniond old_local = util::rotateDegreeZYX(posture.bone_rotations[bone->idx]);
    const Eigen::Quaterniond q_local_delta = jf_q.conjugate() * q_world * jf_q;
    Eigen::Quaterniond new_local_q = q_local_delta * old_local;
    new_local_q.normalize();

    double x_rad, y_rad, z_rad;
    matrixToEulerZYX(new_local_q.toRotationMatrix(), x_rad, y_rad, z_rad);
    Eigen::Vector4d new_euler;
    new_euler << x_rad * 180.0 / util::PI,
                 y_rad * 180.0 / util::PI,
                 z_rad * 180.0 / util::PI,
                 0.0;
    const Eigen::Vector4d clamped_euler = clampBoneRotation(new_euler, bone);
    posture.bone_rotations[bone->idx] = clamped_euler;

    const Eigen::Quaterniond clamped_local = util::rotateDegreeZYX(clamped_euler);
    appliedWorldOut = (jf_q * clamped_local) * (jf_q * old_local).conjugate();
    appliedWorldOut.normalize();
}

int JacobianChainSolver(IKChainContext& chain) {
    int iter = 0;
    for (; iter < kMaxIteration; ++iter) {
        Eigen::Vector4d endEffectorPos = *chain.jointChain[0];
        Eigen::Vector4d delta = chain.target - endEffectorPos;
        delta[3] = 0.0;
        if (delta.head<3>().norm() < kConvergeEpsilon) break;

        int totalDof = 0;
        for (int i = 0; i < chain.boneNum; ++i) {
            const acclaim::Bone* bone = chain.boneChain[i];
            if (bone->dofrx) ++totalDof;
            if (bone->dofry) ++totalDof;
            if (bone->dofrz) ++totalDof;
        }
        if (totalDof == 0) break;

        // J' = J * W (weight 攤進 column)，解完再乘回去 dtheta = W * y
        Eigen::Matrix4Xd J(4, totalDof);
        J.setZero();
        Eigen::VectorXd colWeights(totalDof);
        int col = 0;
        for (int i = 0; i < chain.boneNum; ++i) {
            const acclaim::Bone* bone = chain.boneChain[i];
            const Eigen::Affine3d jointFrame = jointFrameOf(bone);
            const Eigen::Vector3d pivot = chain.jointChain[i + 1]->head<3>();
            const Eigen::Vector3d lever = endEffectorPos.head<3>() - pivot;
            const double w = jointWeight(i, chain.boneNum);

            if (bone->dofrx) {
                const Eigen::Vector3d axisW = jointFrame.linear() * Eigen::Vector3d::UnitX();
                J.col(col).head<3>() = w * axisW.cross(lever);
                colWeights[col] = w;
                ++col;
            }
            if (bone->dofry) {
                const Eigen::Vector3d axisW = jointFrame.linear() * Eigen::Vector3d::UnitY();
                J.col(col).head<3>() = w * axisW.cross(lever);
                colWeights[col] = w;
                ++col;
            }
            if (bone->dofrz) {
                const Eigen::Vector3d axisW = jointFrame.linear() * Eigen::Vector3d::UnitZ();
                J.col(col).head<3>() = w * axisW.cross(lever);
                colWeights[col] = w;
                ++col;
            }
        }

        Eigen::Vector4d step = kJacobianIKStepSize * delta;
        step[3] = 0.0;
        const Eigen::VectorXd y = pseudoInverseLinearSolver(J, step);
        Eigen::VectorXd dtheta = colWeights.cwiseProduct(y);

        int idx = 0;
        for (int i = 0; i < chain.boneNum; ++i) {
            acclaim::Bone* bone = chain.boneChain[i];
            Eigen::Vector4d delta_deg = Eigen::Vector4d::Zero();
            if (bone->dofrx) { delta_deg[0] = dtheta[idx] * 180.0 / util::PI; ++idx; }
            if (bone->dofry) { delta_deg[1] = dtheta[idx] * 180.0 / util::PI; ++idx; }
            if (bone->dofrz) { delta_deg[2] = dtheta[idx] * 180.0 / util::PI; ++idx; }
            const Eigen::Vector4d newRot = chain.posture.bone_rotations[bone->idx] + delta_deg;
            chain.posture.bone_rotations[bone->idx] = clampBoneRotation(newRot, bone);
        }

        if (chain.chainIdx == 0) {
            chain.posture.bone_translations[0] += (chain.currentBasePos - *chain.jointChain[chain.boneNum]);
        }
        forwardSolver(chain.posture, chain.rootBone);
    }
    return iter;
}

int CCDChainSolver(IKChainContext& chain) {
    int iter = 0;
    for (; iter < kMaxIteration; ++iter) {
        Eigen::Vector4d cachedEffectorPos = *chain.jointChain[0];
        if ((chain.target - cachedEffectorPos).head<3>().norm() < kConvergeEpsilon) break;

        for (int i = 0; i < chain.boneNum; ++i) {
            acclaim::Bone* bone = chain.boneChain[i];
            const Eigen::Vector3d pivot = chain.jointChain[i + 1]->head<3>();
            const Eigen::Vector3d e = cachedEffectorPos.head<3>() - pivot;
            const Eigen::Vector3d t = chain.target.head<3>() - pivot;
            if (e.norm() < kTiny || t.norm() < kTiny) continue;

            const double w = jointWeight(i, chain.boneNum);  // distal 轉多、base 轉少
            const Eigen::Quaterniond q_world_full = Eigen::Quaterniond::FromTwoVectors(e, t);
            const Eigen::Quaterniond q_world =
                Eigen::Quaterniond::Identity().slerp(kCCDIKStepSize * w, q_world_full);

            Eigen::Quaterniond appliedWorld;
            applyWorldRotationToBone(bone, q_world, chain.posture, appliedWorld);

            // 用 clamp 後實際 applied 的旋轉去更新 cached effector（PDF p.7）
            const Eigen::Vector3d new_e = appliedWorld * e;
            cachedEffectorPos.head<3>() = pivot + new_e;
            cachedEffectorPos[3] = 0.0;
        }

        if (chain.chainIdx == 0) {
            chain.posture.bone_translations[0] += (chain.currentBasePos - *chain.jointChain[chain.boneNum]);
        }
        forwardSolver(chain.posture, chain.rootBone);
    }
    return iter;
}

// FABRIK + CCD refinement
//   FABRIK 是 position-based 的解，每 iter 做 backward + forward pass 維持骨長，
//   再從每 joint 的目標位置反推 bone rotation。clamp 之後常達不到 1e-3，後面接 CCD 收尾。
int FABRIKChainSolver(IKChainContext& chain) {
    if (chain.boneNum <= 0) return 0;
    const int N = chain.boneNum;

    // 偵測 upper section：chain 順序與 hierarchy parent 對齊的前綴
    // 跨身體 chain 0 的下半身段 chain pivot/tip 與 hierarchy 反向，rotation extraction 公式不適用，跳過
    int upperEnd = N;
    for (int i = 0; i < N - 1; ++i) {
        if (chain.boneChain[i]->parent != chain.boneChain[i + 1]) {
            upperEnd = i + 1;
            break;
        }
    }

    std::vector<Eigen::Vector3d> p(N + 1);
    std::vector<double> d(N);
    const Eigen::Vector3d basePos = chain.jointChain[N]->head<3>();

    int fabrikIter = 0;
    for (; fabrikIter < kMaxIteration; ++fabrikIter) {
        for (int i = 0; i <= N; ++i) p[i] = chain.jointChain[i]->head<3>();
        for (int i = 0; i < N; ++i) d[i] = (p[i] - p[i + 1]).norm();

        const Eigen::Vector3d tgt = chain.target.head<3>();
        if ((tgt - p[0]).norm() < kConvergeEpsilon) break;

        // backward pass: 末端→base，保持骨長
        p[0] = tgt;
        for (int i = 1; i <= N; ++i) {
            const Eigen::Vector3d dir = (p[i] - p[i - 1]).normalized();
            p[i] = p[i - 1] + dir * d[i - 1];
        }
        // forward pass: base pin 回 anchor → 末端
        p[N] = basePos;
        for (int i = N - 1; i >= 0; --i) {
            const Eigen::Vector3d dir = (p[i] - p[i + 1]).normalized();
            p[i] = p[i + 1] + dir * d[i];
        }

        // 每根 bone 算 rotation、套上、馬上 FK 讓下一根用 live pivot 算 desiredDir
        for (int i = upperEnd - 1; i >= 0; --i) {
            acclaim::Bone* bone = chain.boneChain[i];
            const Eigen::Vector3d pivot = chain.jointChain[i + 1]->head<3>();
            const Eigen::Vector3d currentTip = chain.jointChain[i]->head<3>();
            const Eigen::Vector3d currentDir = currentTip - pivot;
            const Eigen::Vector3d desiredDir = p[i] - pivot;
            if (currentDir.norm() < kTiny || desiredDir.norm() < kTiny) continue;

            const Eigen::Quaterniond q_world = Eigen::Quaterniond::FromTwoVectors(currentDir, desiredDir);
            Eigen::Quaterniond appliedWorld;
            applyWorldRotationToBone(bone, q_world, chain.posture, appliedWorld);
            forwardSolver(chain.posture, chain.rootBone);
        }

        if (chain.chainIdx == 0) {
            chain.posture.bone_translations[0] += (chain.currentBasePos - *chain.jointChain[chain.boneNum]);
            forwardSolver(chain.posture, chain.rootBone);
        }
    }

    // CCD refinement，已收斂的話第一輪就 break
    const int ccdIter = CCDChainSolver(chain);
    return fabrikIter + ccdIter;
}

}  // namespace

bool inverseIKSolver(const std::vector<Eigen::Vector4d>& targets, acclaim::Bone* endBone, acclaim::Posture& posture,
                     JointChains& jointChains, BoneChains& boneChains, const Eigen::Vector4d& currentBasePos,
                     IKAlgorithm algorithm, bool enableConvergenceTest) {
    acclaim::Bone* rootBone = endBone - endBone->idx;
    acclaim::Posture originalPosture(posture);
    int totalIter = 0;
    for (int chainIdx = 0; chainIdx < static_cast<int>(boneChains.size()); ++chainIdx) {
        IKChainContext chainContext{
            targets[chainIdx],    rootBone,       posture,  jointChains[chainIdx],
            boneChains[chainIdx], currentBasePos, chainIdx, static_cast<int>(boneChains[chainIdx].size())};
        int chainIter = 0;
        switch (algorithm) {
            case IKAlgorithm::CCD:
                chainIter = CCDChainSolver(chainContext);
                break;
            case IKAlgorithm::FABRIK:
                chainIter = FABRIKChainSolver(chainContext);
                break;
            case IKAlgorithm::Jacobian:
            default:
                chainIter = JacobianChainSolver(chainContext);
                break;
        }
        totalIter += chainIter;
    }

    // 抓 max residual 給 UI 顯示
    double maxResidual = 0.0;
    const std::size_t chainCount = std::min(targets.size(), jointChains.size());
    for (std::size_t i = 0; i < chainCount; ++i) {
        if (jointChains[i].empty() || jointChains[i][0] == nullptr) continue;
        const double r = (targets[i] - *jointChains[i][0]).head<3>().norm();
        if (r > maxResidual) maxResidual = r;
    }
    gLastIterations = totalIter;
    gLastResidual = maxResidual;

    if (enableConvergenceTest && !IKConverged(targets, jointChains)) {
        posture = originalPosture;
        forwardSolver(posture, rootBone);
        return false;
    }
    return true;
}

int getLastIterations() { return gLastIterations; }
double getLastResidual() { return gLastResidual; }

void forwardSolver(const acclaim::Posture& posture, acclaim::Bone* bone) {
    while (bone != nullptr) {
        Eigen::Quaterniond local_rotation = util::rotateDegreeZYX(posture.bone_rotations[bone->idx]);
        local_rotation.normalize();
        Eigen::Affine3d rot_local;
        rot_local = local_rotation.toRotationMatrix();

        if (bone->parent == nullptr) {
            bone->rotation = rot_local;
            bone->start_position = bone->end_position = posture.bone_translations[0];
        } else {
            bone->rotation = bone->parent->rotation * bone->rot_parent_current * rot_local;
            bone->start_position = bone->end_position = bone->parent->end_position;
            bone->end_position += posture.bone_translations[bone->idx];
            bone->end_position += bone->rotation * bone->dir * bone->length;
        }
        forwardSolver(posture, bone->child);
        bone = bone->sibling;
    }
}
}  // namespace kinematics
