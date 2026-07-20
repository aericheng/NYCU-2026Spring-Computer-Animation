# 重算受影響鏡頭 — 籃球(ACT1+ACT3)+ 裁判側邊進場(ACT4)
#
# 用法：Blender 開主檔 → Scripting → 開啟本檔 → Run Script（Alt+P）
#   逐格算，介面會凍住屬正常；約 5,261 格、1.5 小時。
#   輸出 renders/ballfix/f_#####.png；跑完通知 Claude 接片編碼。
#
# 重點：
#  - 清掉所有 frame handler，避免 ground_stick_sga 把角色黏地（抵消抬升 bake、壓掉跳躍）
#  - 每格明確呼叫 update_gn_cylinders → 圓柱紅色正確（handler 在算圖時不一定觸發）
#  - 籃球、裁判、地板抬升都是關鍵格，frame_set 會自動套用

import bpy, os, time

sc = bpy.data.scenes['Scene']
bpy.context.window.scene = sc

# ---- 圓柱更新函式（只定義，不註冊 handler）----
def get_all_fcurves(act):
    fcs = []
    try:
        for layer in act.layers:
            for s in layer.strips:
                for cb in s.channelbags:
                    fcs.extend(cb.fcurves)
    except Exception:
        try: fcs = list(act.fcurves)
        except: pass
    return fcs

def eval_prop(obj, prop_name, default=0.0):
    if not obj.animation_data or not obj.animation_data.action:
        return obj.get(prop_name, default)
    fcs = get_all_fcurves(obj.animation_data.action)
    target = '["' + prop_name + '"]'
    f = bpy.context.scene.frame_current
    for fc in fcs:
        if fc.data_path == target:
            return fc.evaluate(f)
    return obj.get(prop_name, default)

def update_gn_cylinders(scene):
    pairs = [("Cyl_SGA", "Defender_RightHand_Empty"), ("Cyl_Defender", "SGA_RightHand_Empty")]
    for cyl_name, hand_name in pairs:
        cyl = bpy.data.objects.get(cyl_name)
        hand = bpy.data.objects.get(hand_name)
        if not cyl or not hand: continue
        mod = cyl.modifiers.get("GN_Cylinder")
        if not mod or not mod.node_group: continue
        sids = {s.name: s.identifier for s in mod.node_group.interface.items_tree if s.in_out == 'INPUT'}
        ov_active = bool(int(eval_prop(cyl, "override_active", 0)))
        if ov_active:
            oc = cyl.get("override_color", (0.5, 0.5, 0.5, 0.3))
            color = tuple(oc) if len(oc) == 4 else (oc[0], oc[1], oc[2], 0.3)
            emission = float(eval_prop(cyl, "override_emission", 0.0))
        else:
            dist = (cyl.matrix_world.translation - hand.matrix_world.translation).length
            threshold = float(eval_prop(cyl, "foul_threshold", 0.3))
            if dist < threshold:
                color = (1.0, 0.1, 0.1, 0.4); emission = 5.0
            else:
                color = (0.2, 1.0, 0.3, 0.3); emission = 0.0
        mod[sids["Color"]] = color
        mod[sids["Emission Strength"]] = emission
        mod[sids["Radius"]] = float(eval_prop(cyl, "radius", 0.6))
        mod[sids["Height"]] = float(eval_prop(cyl, "height", 2.0))

# ---- 清掉所有 frame handler（防 ground_stick 黏地）----
for hl in [bpy.app.handlers.frame_change_post, bpy.app.handlers.render_pre,
           bpy.app.handlers.render_post, bpy.app.handlers.frame_change_pre]:
    hl.clear()

# ---- 算圖設定（與成片一致）----
R = sc.render
R.image_settings.file_format = 'PNG'
R.image_settings.color_mode = 'RGB'
R.resolution_x = 1920
R.resolution_y = 1080
R.resolution_percentage = 100
out = bpy.path.abspath("//renders/ballfix/")
os.makedirs(out, exist_ok=True)

markers = sorted([(m.frame, m.camera) for m in sc.timeline_markers if m.camera])
def cam_for(f):
    cam = None
    for mf, mc in markers:
        if mf <= f: cam = mc
    return cam

# 受影響範圍：ACT1 籃球 / ACT3 籃球 / ACT4 裁判側進
ranges = [(1, 2200), (5440, 7600), (11701, 12600)]
total = sum(b - a + 1 for a, b in ranges)
done = 0
t0 = time.time()
print("=" * 50)
print("開始重算，共 %d 格（約 1.5 小時）" % total)
print("=" * 50)
for a, b in ranges:
    for f in range(a, b + 1):
        sc.frame_set(f)
        update_gn_cylinders(sc)
        c = cam_for(f)
        if c: sc.camera = c
        R.filepath = os.path.join(out, "f_%05d" % f)
        bpy.ops.render.render(write_still=True)
        done += 1
    print("  完成 %d-%d（累計 %d/%d，%.1f 分）" % (a, b, done, total, (time.time() - t0) / 60))
print("=" * 50)
print("BALLFIX_DONE 共 %d 格，耗時 %.1f 分鐘 → 通知 Claude 接片編碼" % (done, (time.time() - t0) / 60))
print("=" * 50)
