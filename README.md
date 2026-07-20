# Computer Animation and Special Effects — NYCU, Spring 2026

國立陽明交通大學「電腦動畫與特效」課程作業與期末專題集

## Course Info ｜ 課程資訊

- **Institution:** National Yang Ming Chiao Tung University (NYCU 國立陽明交通大學)
- **Semester:** Spring 2026（114學年度第2學期）
- **Course:** Computer Animation and Special Effects 電腦動畫與特效（graduate-level course, 多媒體所）
- **Instructor:** 林文杰 Wen-Chieh Lin
- **Field:** Computer Graphics / Animation

## Overview

This repository collects two homework assignments and one final project from the
NYCU Computer Animation and Special Effects course. HW1 implements a mass-spring
particle system (a jelly/soft-body simulator with terrain collision) from scratch
in C++/OpenGL. HW3 implements Inverse Kinematics on a CMU mocap skeleton (AMC/ASF)
with three solvers — Jacobian (damped least squares), Cyclic Coordinate Descent,
and a FABRIK+CCD hybrid — driven by an interactive ImGui demo. The final project
is a short animated film produced in Blender, visualizing (and satirizing) the
NBA's "principle of verticality" foul rule through a 3D short with full pipeline
(modeling, rigging, rendering, editing, and a presentation report).
本作業集涵蓋質點彈簧系統模擬、反向運動學（Inverse Kinematics）與 Blender 動畫短片
期末專題，涉及物理模擬、骨架動畫演算法與動畫製作管線。

## Contents ｜ 內容

| Folder | Topic | Summary |
|--------|-------|---------|
| [HW1](HW1/) | Mass-Spring Particle System（質點彈簧系統模擬） | From-scratch C++/OpenGL soft-body simulator built on a mass-spring system (particles, springs, semi-implicit integrator) with terrain collision, rendered with custom GLSL shaders (particle/render/shadow/skybox). Includes the assignment spec, written report, and full source (`code/`). Local submission that had never previously been pushed to GitHub. |
| [HW3](HW3/) | Inverse Kinematics（反向運動學） | IK solver on the CMU mocap skeleton (AMC/ASF format) with an interactive ImGui demo for dragging end-effector targets in real time. Implements DOF-respecting Jacobian (damped least squares via SVD), Cyclic Coordinate Descent (CCD), and a bonus FABRIK+CCD hybrid solver, plus joint weighting, gimbal-lock fallback, and live convergence/residual display. |
| [Project](Project/) | Final Project — NBA Cylinder Foul Rule Short Film（NBA犯規機制之侵犯圓柱體規則討論及視覺化） | A short animated film (Blender, EEVEE Next, 1920×1080 @ 60fps, ~3:30 runtime) satirizing the NBA's "principle of verticality" defensive foul rule by literally visualizing the invisible foul cylinder. Includes full production pipeline scripts (rendering, stitching, recalculation passes), slides, and written report. Group final project. |

## Notes

Each `HW*/` folder is self-contained and keeps its own original README with
setup instructions and implementation details — please refer to the subfolder
README before running any code. Commit history for HW3 and Project has been
preserved from their original individual repositories. HW1 is added as a fresh
commit since it existed only as an uncommitted local working copy (never
previously pushed anywhere); a ~70MB demo video and a duplicate submission zip
(identical to the loose files already present) were excluded to keep the
repository lean.
