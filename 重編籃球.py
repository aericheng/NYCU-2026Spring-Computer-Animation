# 重編籃球 — 全片籃球動畫（運球貼手下方不穿掌、投籃出手進球、走路收球、ACT3 補球）
#
# 用法：Blender 開主檔 → Scripting → 開啟本檔 → Run Script（約 1-2 分鐘）
#   跑完通知 Claude 算圖。介面會凍住屬正常。
#
# 重點設計：
#   - GAP：運球/握球時球頂落在「手掌面」下方，不會穿進手掌
#   - f601-742：投籃動作 → 球握起 → f682 出手 → 拋物線飛向籃框（框外進球）
#   - f721-1200：走路段球隱藏（空手走，不拿球）
#   - 其餘運球段球可見、會彈、不黏手

import bpy, math
from mathutils import Vector

sc = bpy.data.scenes['Scene']
bpy.context.window.scene = sc
ball = bpy.data.objects['Basketball']
rig  = bpy.data.objects['SGA_Rig']
mid  = rig.pose.bones['mixamorig:RightHandMiddle1']
hand = rig.pose.bones['mixamorig:RightHand']
R, FLOOR, GAP = 0.12, 0.15, 0.09          # 球半徑 / 地面高 / 手掌面餘隙
g = 9.81; av = Vector((0, 0, -g))
HOOP = Vector((-10.09, -0.36, 3.06))      # 球場 -X 端籃框

def palm(f):
    sc.frame_set(f); return (rig.matrix_world @ mid.matrix).translation.copy()
def handz(f):
    sc.frame_set(f); return (rig.matrix_world @ hand.matrix).translation.z
def ins(f, loc):
    ball.location = loc; ball.keyframe_insert("location", frame=f)

ad = ball.animation_data
def clear_path(path):
    for layer in ad.action.layers:
        for st in layer.strips:
            for cb in st.channelbags:
                for fc in list(cb.fcurves):
                    if fc.data_path == path:
                        cb.fcurves.remove(fc)
clear_path('location')

def detect_beats(a, b):
    zs = {f: handz(f) for f in range(a, b + 1, 2)}
    fs = sorted(zs); beats = []
    for i in range(2, len(fs) - 2):
        f = fs[i]; w = [zs[fs[j]] for j in range(i - 2, i + 3)]
        if zs[f] == min(w) and zs[f] < 1.15 and (not beats or f - beats[-1] > 18):
            beats.append(f)
    return beats

def key_hold(a, b, step=4):               # 握球（球在手掌面下方）
    for f in range(a, b + 1, step):
        p = palm(f); ins(f, Vector((p.x, p.y, p.z - R - GAP)))

def key_dribble(a, b):                     # 運球（彈到地面、回到手掌面、不黏手）
    beats = detect_beats(a, b)
    if not beats:
        key_hold(a, b); return
    pa = palm(a); ins(a, Vector((pa.x, pa.y, FLOOR)))
    for i, bt in enumerate(beats):
        p = palm(bt); ins(bt, Vector((p.x, p.y, p.z - R - GAP)))
        if i < len(beats) - 1:
            m = (bt + beats[i + 1]) // 2; pm = palm(m); ins(m, Vector((pm.x, pm.y, FLOOR)))
    pb = palm(b); ins(b, Vector((pb.x, pb.y, FLOOR)))

def key_shot(rel, wind_a, T=0.95, bounce=True):   # 投籃：握→出手→拋物線→（可選）落地彈跳
    for f in range(wind_a, rel + 1, 3):
        p = palm(f); ins(f, Vector((p.x, p.y, p.z - GAP)))
    p0 = palm(rel); p0 = Vector((p0.x, p0.y, p0.z + 0.05)); Tf = int(T * 60)
    v = (HOOP - p0) / T - 0.5 * av * T
    for f in range(rel, rel + Tf + 1, 2):
        t = (f - rel) / 60.0; ins(f, p0 + v * t + 0.5 * av * t * t)
    fh = rel + Tf; ins(fh, HOOP.copy())
    if bounce:
        vz = v.z + av.z * T; tf = (-vz - math.sqrt(vz * vz + 2 * g * (HOOP.z - FLOOR))) / av.z; t = 0.0
        while t < tf:
            z = HOOP.z + vz * t + 0.5 * av.z * t * t
            ins(fh + int(t * 60), Vector((HOOP.x, HOOP.y, max(FLOOR, z)))); t += 2 / 60.0
        fl = fh + int(tf * 60); ins(fl, Vector((HOOP.x, HOOP.y, FLOOR))); vb = abs(vz) * 0.45
        for _ in range(2):
            th = vb / g
            ins(fl + int(th * 60), Vector((HOOP.x, HOOP.y, FLOOR + vb * vb / (2 * g))))
            fl += int(2 * th * 60); ins(fl, Vector((HOOP.x, HOOP.y, FLOOR))); vb *= 0.5

# ===== ACT1 =====
key_hold(1, 178)
key_dribble(180, 600)
key_shot(682, 601, T=1.0, bounce=False)    # ~10秒投籃：出手 → 框外進球
# f721-1200 走路：球隱藏（見下方可見性）
key_dribble(1201, 1488)
key_shot(1518, 1489, T=0.95, bounce=True)  # 主投籃：出手 → 空心 → 落地彈跳
key_dribble(1801, 2100)
key_dribble(2100, 2200)

# ===== ACT3 =====
key_dribble(5440, 6200)
key_hold(6201, 6600)
key_dribble(6600, 7600)

# ===== 可見性（CONSTANT）=====
clear_path('hide_render'); clear_path('hide_viewport')
def vis(f, hidden):
    ball.hide_render = hidden; ball.hide_viewport = hidden
    ball.keyframe_insert('hide_render', frame=f); ball.keyframe_insert('hide_viewport', frame=f)
vis(1, False)
vis(720, False);  vis(721, True)           # 投籃後球飛出 → 走路收球
vis(1200, True);  vis(1201, False)
vis(1641, False); vis(1642, True)
vis(1800, True);  vis(1801, False)
vis(2200, False); vis(2201, True)
vis(5439, True);  vis(5440, False)
vis(7600, False); vis(7601, True)
for layer in ad.action.layers:
    for st in layer.strips:
        for cb in st.channelbags:
            for fc in cb.fcurves:
                if fc.data_path in ('hide_render', 'hide_viewport'):
                    for kp in fc.keyframe_points:
                        kp.interpolation = 'CONSTANT'

sc.frame_set(1)
print("=" * 50)
print("籃球重編完成：運球(下調不穿掌)+投籃出手進球+走路收球+ACT3 補球")
print("=" * 50)
