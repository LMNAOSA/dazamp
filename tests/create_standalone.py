from pathlib import Path
from html import escape
BASE=Path(__file__).resolve().parents[1]
html=(BASE/'web/index.html').read_text()
html=html.replace('<link rel="stylesheet" href="style.css">', '<style>\n'+(BASE/'web/style.css').read_text()+'\n</style>')
for script in ['tuner.js','audio.js','app.js']:
    html=html.replace(f'<script src="{script}"></script>', '<script>\n'+(BASE/'web'/script).read_text()+'\n</script>')
assert '<script src=' not in html
assert '<link rel=' not in html
(BASE/'DAZAMP-demo.html').write_text(html)
print('Created standalone',len(html),'bytes')
