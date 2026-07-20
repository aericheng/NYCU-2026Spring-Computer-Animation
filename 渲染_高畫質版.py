# 期末專案 — 最終高畫質批次渲染
#
# 用法：
#   1. 在 Blender 開啟主檔（籃球短片_主檔.blend）
#   2. 切到 Scripting 工作區，開啟本檔，按 Run Script（或 Alt+P）
#
# 渲染時 Blender 介面會凍住，這是正常的；進度會顯示在狀態列（Frame X/Y）。
# 輸出是 PNG 序列，放在 .blend 同層的 renders/final/，最後再用 ffmpeg 接成 MP4。

import bpy, os, time

# ---- 畫質設定 ----
FRAME_STEP   = 1            # 1 = 每格都算（60fps）；2 = 隔格算（30fps，比較快）
SAMPLES      = 64           # EEVEE 取樣數，越高越乾淨也越慢
RES_X, RES_Y = 1920, 1080   # 解析度

S = bpy.context.scene
R = S.render
R.engine = 'BLENDER_EEVEE'
R.resolution_x = RES_X
R.resolution_y = RES_Y
R.resolution_percentage = 100
R.image_settings.file_format = 'PNG'
R.image_settings.color_mode = 'RGB'
try:
    S.eevee.taa_render_samples = SAMPLES
except Exception:
    pass

out = bpy.path.abspath("//renders/final/")
os.makedirs(out, exist_ok=True)
R.filepath = os.path.join(out, "f_")

S.frame_step  = FRAME_STEP
S.frame_start = 1
S.frame_end   = 12600

n = len(range(1, 12601, FRAME_STEP))
print("=" * 50)
print("開始渲染")
print("解析度 %dx%d | 取樣 %d | step %d（%dfps）| 共 %d 格"
      % (RES_X, RES_Y, SAMPLES, FRAME_STEP, 60 // FRAME_STEP, n))
print("輸出：", out)
print("=" * 50)

t = time.time()
bpy.ops.render.render(animation=True)
mins = (time.time() - t) / 60

print("=" * 50)
print("渲染完成，共 %d 格，耗時 %.1f 分鐘" % (n, mins))
print("接著用 ffmpeg 把 renders/final/ 的 PNG 序列編成 MP4")
print("=" * 50)
