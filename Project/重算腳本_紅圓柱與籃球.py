# 重算腳本 — 籃球修正 + 紅色圓柱（只算有改動的三段）
#
# 用法：
#   1. 在 Blender 開啟 籃球短片_主檔.blend
#   2. 切到 Scripting 工作區，開啟本檔，按 Run Script（或 Alt+P）
#   3. 渲染期間 Blender 介面會凍住，這是正常的；進度顯示在系統主控台
#   4. 三段約 4,700 格，預估 60–70 分鐘；跑完通知 Claude 接片
#
# 輸出：PNG 序列 → renders/reshot/f_#####.png

import bpy, os, time

# 1) 確保圓柱 / HUD 的 frame handler 有註冊（重開 Blender 後一定要先跑這個，
#    否則圓柱顏色不會更新、紅色不會出現）
if 'gn_cylinder_handler' in bpy.data.texts:
    exec(bpy.data.texts['gn_cylinder_handler'].as_string())
    print("圓柱 handler 已註冊")

sc = bpy.context.scene
R  = sc.render
R.image_settings.file_format = 'PNG'
R.image_settings.color_mode  = 'RGB'
R.resolution_x = 1920
R.resolution_y = 1080
R.resolution_percentage = 100

os.makedirs(bpy.path.abspath("//renders/reshot/"), exist_ok=True)
R.filepath = "//renders/reshot/f_#####"   # 5 位數補零，方便接片

# 只重算有改動的段落
ranges = [
    (1, 2700),       # 運球同步 + 投籃進球 + ACT1 圓柱紅
    (5600, 6601),    # ACT3 裁判判罰：圓柱綠→紅
    (10500, 11500),  # ACT4 西裝店：圓柱淡淡紅
]

t0 = time.time()
total = sum(b - a + 1 for a, b in ranges)
print("=" * 50)
print("開始重算，共 %d 格" % total)
for a, b in ranges:
    sc.frame_start, sc.frame_end = a, b
    print("--- 算 %d–%d（%d 格） ---" % (a, b, b - a + 1))
    bpy.ops.render.render(animation=True)
    print("    完成 %d–%d" % (a, b))

# 還原全片範圍
sc.frame_start, sc.frame_end = 1, 12600
print("=" * 50)
print("全部完成：%d 格，耗時 %.1f 分鐘" % (total, (time.time() - t0) / 60))
print("輸出在 renders/reshot/，跑完請通知 Claude 接片")
print("=" * 50)
