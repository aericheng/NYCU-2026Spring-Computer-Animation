# 補算圓柱紅色 — 只重算圓柱出現的三段，每格「明確設定顏色」再算，保證紅色正確
#
# 用法：
#   1. 在 Blender 開啟 籃球短片_主檔.blend（就是現在這個）
#   2. 切到 Scripting 工作區，開啟本檔，按 Run Script（或 Alt+P）
#   3. 約 30-50 分鐘（逐格算，較慢但保證對）；介面會凍住屬正常
#   4. 跑完通知 Claude，會自動覆蓋掉舊的綠色畫面
#
# 輸出：覆蓋 renders/reshot/ 裡這三段的 PNG

import bpy, os, time

sc = bpy.data.scenes['Scene']
bpy.context.window.scene = sc

# 載入圓柱 handler 的函式（用來每格明確設定圓柱顏色）
exec(bpy.data.texts['gn_cylinder_handler'].as_string())

R = sc.render
R.image_settings.file_format = 'PNG'
R.image_settings.color_mode = 'RGB'
R.resolution_x = 1920
R.resolution_y = 1080
R.resolution_percentage = 100
out = bpy.path.abspath("//renders/reshot/")

# 依時間軸標記決定每格用哪台攝影機
markers = sorted([(m.frame, m.camera) for m in sc.timeline_markers if m.camera])
def cam_for(f):
    cam = None
    for mf, mc in markers:
        if mf <= f:
            cam = mc
    return cam

# 圓柱出現的三段（ACT1 犯規、ACT3 判罰、ACT4 西裝店）
ranges = [(2000, 2500), (5601, 6600), (10500, 11500)]
total = sum(b - a + 1 for a, b in ranges)
done = 0
t0 = time.time()
print("=" * 46)
print("補算圓柱顏色，共 %d 格，約 30-50 分鐘..." % total)
print("=" * 46)
for a, b in ranges:
    for f in range(a, b + 1):
        sc.frame_set(f)
        update_gn_cylinders(sc)          # 關鍵：每格明確設定圓柱顏色（紅）
        c = cam_for(f)
        if c:
            sc.camera = c
        R.filepath = os.path.join(out, "f_%05d" % f)
        bpy.ops.render.render(write_still=True)
        done += 1
    print("  完成 %d-%d（累計 %d/%d）" % (a, b, done, total))
print("=" * 46)
print("全部完成 %d 格，耗時 %.1f 分鐘。請通知 Claude 接成最終 MP4" % (total, (time.time() - t0) / 60))
print("=" * 46)
