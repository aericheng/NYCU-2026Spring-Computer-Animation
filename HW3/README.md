# Computer Animation and Special Effects — HW3

Inverse Kinematics on the CMU mocap skeleton (AMC/ASF), with three IK solvers and an interactive ImGui demo for dragging end-effector targets and watching the chain reach to them in real time.

NYCU 2026 Spring · Student: 113950011 · 鄭名翔

---

## 課程作業簡介

題目：對 CMU mocap 骨架實作 Inverse Kinematics。使用者拖滑鼠球 (end-effector target)，骨架要即時把對應的手指/腳趾關節 reach 過去，過程中要：

- 尊重每根骨頭的 DOF 與 rotation limits（ASF 檔定義）
- 維持其他 anchor 點不動（透過 root translation fix）
- 支援切換 algorithm 比較收斂行為

作業要求的五個必填函式都在 `src/simulation/kinematics.cpp`：

| 函式 | 功用 |
|------|------|
| `IKConverged()` | 收斂判定（殘差 < 1e-3） |
| `clampBoneRotation()` | 套 DOF mask + 角度 clamp |
| `pseudoInverseLinearSolver()` | DLS pseudo-inverse via SVD |
| `JacobianChainSolver()` | Jacobian + damped least squares IK |
| `CCDChainSolver()` | Cyclic Coordinate Descent IK |

## Implemented Algorithms

| Algorithm | 特性 |
|-----------|------|
| **Jacobian (DLS)** | 所有 DOF 同時解，姿勢自然，奇異點靠 damping 撐住 |
| **CCD** | 末端 → base 逐骨對齊，反應快，末端骨擺動大 |
| **FABRIK + CCD hybrid** | 先 FABRIK position pass 把骨架擺到對的形狀，再 CCD 收尾把殘差收緊。簡單 chain 收斂最快、跨身體 chain 0 也能搞定 |

## Bonus 功能

1. **FABRIK + CCD hybrid**（新增 `IKAlgorithm::FABRIK`）
2. **Joint weighting**：distal 大、base 小，動作更像人
3. **UI 顯示 Iter / Residual**：Control 面板即時顯示，方便調 step size
4. **Damped Least Squares**：`σ/(σ²+λ²)` 取代 `1/σ`，奇異點不爆
5. **CCD step-size slerp**：避免單步轉太猛
6. **Gimbal-lock fallback**：ZYX→Euler 在 `cos(θy)≈0` 改用替代公式
7. **Root rotation 不被 clamp 鎖死**：偵測 `rxmin == rxmax` 視為無限制

詳見 `report.pdf` §4。

## Repo 內容

| 路徑 | 說明 |
|------|------|
| `src/simulation/kinematics.cpp` | 5 個必填函式 + FABRIK + joint weighting |
| `include/simulation/kinematics.h` | 加了 `IKAlgorithm::FABRIK` + 兩個 UI getter |
| `InverseKinematics/main.cpp` | UI 加 FABRIK 選項 + Iter/Residual 顯示 |
| `report.md` / `report.pdf` | 作業報告（Question / Implementation / Discussion / Bonus / Misc） |
| `HW3_113950011.zip` | 最終繳交的 zip 快照 |
| `package.ps1` | 自動把 cpp/h/main/pdf 包成 zip 的腳本 |
| `md_to_pdf.py` | `report.md` → `report.html` 的轉檔腳本（再用 Edge headless 印 PDF） |
| `HW3_IK.pdf` | 作業說明書 |

---

## Build Instructions

### Windows + Visual Studio 2017/2019/2022

- Open `Kinematics.sln`
- Build (Release / x64 推薦)
- Executable 出現在 `./bin/`

或者用 CMake：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 8
```

### Other platforms (less tested)

#### Prerequisite

- [CMake](https://cmake.org) (>= 3.14)
- 任一 C++17 編譯器（GCC 10+ / Clang / MSVC 14.3+）

#### Instruction

```bash
cmake -S . -B build
cmake --build build --config Release --parallel 8
```

Executable 在 `./bin`。

#### Linux 額外依賴（擇一）

- `xorg-dev` (X11)
- `libwayland-dev wayland-protocols extra-cmake-modules libxkbcommon-dev` (Wayland)
- `libosmesa6-dev` (OSMesa)

### ⚠️ 路徑含中文會 crash

CMU 提供的 `util::PathFinder` 用 `std::filesystem::path::string()` 轉路徑，在 zh-TW codepage 下遇到非 Big5 字元會丟例外、整個程式 fail-fast。如果要從含中文的路徑跑（例如 `Desktop\電腦動畫\HW3`），會出現 exit code `0xC0000409`。

解法：build 完之後把整個資料夾複製到 ASCII-only 路徑（例如 `C:\HW3_test\`）執行。

## Usage

1. 跑起 `./bin/InverseKinematics.exe`
2. 畫面上有 4 顆黃色球（雙手指尖、雙腳趾尖）跟一具骨架
3. 左下「Control」面板：
   - **IK Algorithm** 下拉切 Jacobian / CCD / FABRIK
   - **Iter / Residual** 顯示上一次解算的 iteration 數與末端殘差
   - **Enable IKConverged test** 勾起來代表嚴格 mode：解不到 1e-3 會 revert 整個姿勢
   - **Animation** 切 IK / walk / running / jump 四個 amc
   - **Current Frame** 用 slider 或 `[+] [-]` 移動到任一幀
4. 左鍵拖任一顆黃球當作 IK target，骨架會即時 reach 過去
5. 按 **Camera Panel** 開鏡頭控制；**Debug Mode** 切自由視角（F9 鎖滑鼠視角，W/A/S/D 移動）

## License / Credits

- 骨架資料 (`.asf`, `.amc`) 來自 [CMU Graphics Lab Motion Capture Database](http://mocap.cs.cmu.edu/)
- Skeleton I/O 與 ImGui demo template 由助教提供
- Eigen / GLFW / glad / Dear ImGui / stb 在 `extern/` 內
- IK 演算法實作（5 個必填 + FABRIK + 各項 bonus）由本人完成
