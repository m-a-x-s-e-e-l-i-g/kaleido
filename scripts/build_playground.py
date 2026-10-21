"""Assemble a self-contained browser playground, usable directly from disk."""
from pathlib import Path
import base64, re
root=Path(__file__).resolve().parents[1]
site=root/'site'
html=(site/'index.html').read_text(encoding='utf-8')
css=(site/'styles.css').read_text(encoding='utf-8')
def data(path,mime):
    return 'data:'+mime+';base64,'+base64.b64encode(path.read_bytes()).decode('ascii')
for name in ['bricolage-latin.woff2','public-sans-latin.woff2']:
    css=css.replace('assets/fonts/'+name,data(site/'assets/fonts'/name,'font/woff2'))
js=(site/'kaleido.mjs').read_text(encoding='utf-8').replace('export function','function')
app=(site/'app.js').read_text(encoding='utf-8')
app=re.sub(r"^import .*?;\n",'',app,count=1)
image=data(site/'assets/seaside-scene.webp','image/webp')
app=app.replace("'assets/seaside-scene.webp'",repr(image))
html=re.sub(r'  <link rel="preload"[^>]+>\n','',html)
html=html.replace('<link rel="stylesheet" href="styles.css">','<style>'+css+'</style>')
html=html.replace('<script type="module" src="app.js"></script>','')
html=html.replace('src="assets/seaside-scene.webp"','src="'+image+'"')
for name,mime in [('mark.svg','image/svg+xml')]+[(p.name,'image/webp') for p in sorted((site/'assets').glob('kaleido-*.webp'))]:
    asset='assets/'+name;embedded=data(site/asset,mime)
    for attribute in ['src','href']:
        html=html.replace(attribute+'="'+asset+'"',attribute+'="'+embedded+'"')
html=html.replace('href="downloads/KALEIDO-','href="releases/KALEIDO-')
html=html.replace('</body>','<script type="module">\n'+js+'\n'+app+'\n</script>\n</body>')
(root/'playground.html').write_text(html,encoding='utf-8',newline='\n')
print('Standalone playground:',root/'playground.html')
