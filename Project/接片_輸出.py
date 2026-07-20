# 接片 — 把「原片 + 三段新畫面（投籃進球 + 紅圓柱）」合成後輸出成 JPEG 序列
#
# 用法：
#   1. 在 Blender 開啟 籃球短片_主檔.blend
#   2. 切到 Scripting 工作區，開啟本檔，按 Run Script（或 Alt+P）
#   3. 約 5-7 分鐘，介面會凍住（正常）；跑完通知 Claude 接成 MP4
#
# 輸出：renders/spliced/f_#####.jpg

import bpy, os

# 重建 Splice 場景（自帶設定，重開 Blender 也能跑）
old = bpy.data.scenes.get("Splice")
if old:
    bpy.data.scenes.remove(old)
sp = bpy.data.scenes.new("Splice")
sp.render.fps = 60
sp.render.resolution_x = 1920
sp.render.resolution_y = 1080
sp.render.resolution_percentage = 100
sp.frame_start = 1

base = bpy.path.abspath("//")
mp4 = os.path.join(base, "籃球短片_成片_1080p60.mp4")
reshot = os.path.join(base, "renders", "reshot")

sp.sequence_editor_create()
strips = sp.sequence_editor.strips

# 第 1 軌：原片打底
mov = strips.new_movie("orig", mp4, 1, 1)
sp.frame_end = mov.frame_final_duration

# 第 2-4 軌：三段新畫面疊在對應影格上
def add_seq(name, a, b, channel):
    st = strips.new_image(name, os.path.join(reshot, "f_%05d.png" % a), channel, a)
    for f in range(a + 1, b + 1):
        st.elements.append("f_%05d.png" % f)

add_seq("new1", 1, 2700, 2)        # 運球同步 + 投籃進球 + ACT1 紅圓柱
add_seq("new2", 5600, 6601, 3)     # ACT3 判罰：圓柱綠→紅
add_seq("new3", 10500, 11500, 4)   # ACT4 西裝店：淡淡紅

# 輸出設定：JPEG 序列（之後 Claude 接成 MP4）
sp.render.use_sequencer = True
sp.render.image_settings.file_format = 'JPEG'
sp.render.image_settings.quality = 95
os.makedirs(os.path.join(base, "renders", "spliced"), exist_ok=True)
sp.render.filepath = "//renders/spliced/f_#####"
bpy.context.window.scene = sp

print("=" * 44)
print("開始輸出接片序列，共 %d 格，約 5-7 分鐘，介面會凍住..." % sp.frame_end)
print("=" * 44)
bpy.ops.render.render(animation=True)
print("=" * 44)
print("完成！輸出在 renders/spliced/，請通知 Claude 接成 MP4")
print("=" * 44)
