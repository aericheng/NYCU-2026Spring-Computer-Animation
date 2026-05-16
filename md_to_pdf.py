"""Convert report.md -> report.html with CJK-friendly styling, then exit.
Edge --headless --print-to-pdf is invoked from PowerShell after this."""
import markdown
import sys
import os

src = os.path.join(os.path.dirname(os.path.abspath(__file__)), "report.md")
dst = os.path.join(os.path.dirname(os.path.abspath(__file__)), "report.html")

with open(src, "r", encoding="utf-8") as f:
    md_text = f.read()

# extensions: tables for | a | b | syntax, fenced_code for ``` blocks, toc, sane_lists
html_body = markdown.markdown(
    md_text,
    extensions=["tables", "fenced_code", "sane_lists", "nl2br"],
)

css = """
@page { size: A4; margin: 18mm 16mm; }
html, body {
    font-family: "Microsoft JhengHei", "PingFang TC", "Noto Sans CJK TC", "Segoe UI", sans-serif;
    font-size: 10.5pt;
    line-height: 1.55;
    color: #1a1a1a;
}
/* Headings: keep with next paragraph; never orphan at page bottom */
h1, h2, h3, h4 {
    page-break-after: avoid;
    break-after: avoid-page;
    page-break-inside: avoid;
    break-inside: avoid-page;
}
h1 { font-size: 18pt; border-bottom: 2px solid #333; padding-bottom: 4px; margin-top: 14px; }
h2 {
    font-size: 14pt; border-bottom: 1px solid #ccc; padding-bottom: 2px; margin-top: 18px;
    page-break-before: auto;
}
h3 { font-size: 12pt; margin-top: 14px; color: #1e3a8a; }
h4 { font-size: 11pt; margin-top: 10px; color: #1e3a8a; }
p, li { orphans: 3; widows: 3; }
code {
    font-family: "Cascadia Code", "Consolas", "Source Code Pro", monospace;
    background: #f0f0f0;
    padding: 1px 4px;
    border-radius: 3px;
    font-size: 9.5pt;
}
pre {
    background: #f7f7f9;
    border: 1px solid #ddd;
    border-radius: 4px;
    padding: 8px 12px;
    overflow-x: auto;
    font-size: 9pt;
    page-break-inside: avoid;
    break-inside: avoid;
}
pre code { background: none; padding: 0; font-size: 9pt; }
/* Tables: never split across pages; if too tall, push the whole table to the next page */
table {
    border-collapse: collapse;
    margin: 8px 0;
    width: 100%;
    font-size: 10pt;
    page-break-inside: avoid;
    break-inside: avoid;
}
thead { display: table-header-group; }
tr { page-break-inside: avoid; break-inside: avoid; }
th, td {
    border: 1px solid #ccc;
    padding: 4px 8px;
    text-align: left;
    vertical-align: top;
}
th { background: #eef; }
blockquote {
    border-left: 3px solid #888;
    margin: 8px 0;
    padding: 4px 12px;
    background: #fafafa;
    color: #555;
    page-break-inside: avoid;
    break-inside: avoid;
}
hr { border: none; border-top: 1px solid #bbb; margin: 16px 0; }
ul, ol { margin: 4px 0; padding-left: 24px; }
li { margin: 2px 0; }
strong { color: #000; }
"""

html_full = f"""<!DOCTYPE html>
<html lang="zh-Hant">
<head>
<meta charset="UTF-8">
<title>HW3 Inverse Kinematics Report</title>
<style>{css}</style>
</head>
<body>
{html_body}
</body>
</html>
"""

with open(dst, "w", encoding="utf-8") as f:
    f.write(html_full)

print(f"OK wrote {dst}")
