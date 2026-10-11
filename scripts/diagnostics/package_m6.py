"""Audit and package measured M6 artifacts, without sending UI input."""
from pathlib import Path
import hashlib, json, re, shutil, statistics, html
from PIL import Image, ImageDraw
root=Path(__file__).resolve().parents[2]
out=root/'docs/evidence/m6';out.mkdir(parents=True,exist_ok=True)
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):
    data=p.read_bytes()
    return data.decode('utf-16' if data.startswith(b'\xff\xfe') else 'utf-8-sig',errors='replace')
manifest={'initial_commit':'b27da1e','tests':{},'files':[],'gui':[],'performance':{}}
for arch in (32,64):
    for config in ('debug','release'):
        variant=f'{arch}-{config}'
        test=root/f'build/m6-test-{variant}.log';build=root/f'build/m6-build-{variant}.log'
        text=read(test)
        assert 'All tests passed.' in text and not re.search(r'\bFAIL\s',text),variant
        assert not re.search(r'\berror:',read(build)),variant
        checks=sum(map(int,re.findall(r'^(\d+) [^\r\n]*cases\r?$',text,re.M)))+sum(map(int,re.findall(r'Results: (\d+)/',text)))+sum(map(int,re.findall(r'Results: (\d+) tests, 0 failures',text)))+1
        manifest['tests'][variant]={'checks':checks,'pass_labels':len(re.findall(r'\bPASS\b',text)),'executables':len(re.findall(r'^==>',text,re.M)),
            'test_binary_sha256':digest(root/f'build/{arch}/{config}/test-test_eq_window.exe'),
            'application_sha256':digest(root/f'build/{arch}/{config}/LiteSic.exe')}
        for p in (build,test):shutil.copy2(p,out/p.name)
    data=json.loads((root/f'build/m6/performance-{arch}-release.json').read_text(encoding='utf-8-sig'))
    summary={}
    for baseline,mode in [(True,6)]+[(False,x) for x in (6,0,1,2,3,4,5)]:
        samples=[x['sample'] for x in data if x['baseline']==baseline and x['sample']['mode']==mode]
        assert len(samples)==3
        summary['before' if baseline else str(mode)]={k:statistics.median(s[k] for s in samples) for k in samples[0]}
    manifest['performance'][str(arch)]=summary
    shutil.copy2(root/f'build/m6/performance-{arch}-release.json',out/f'performance-{arch}-release.json')
events=[json.loads(x) for x in (root/'build/gui/events.jsonl').read_text(encoding='utf-8').splitlines()]
selected=[]
for event in events:
    label=event.get('args',{}).get('label') or ''
    if not label.startswith('m6-') or not event.get('ok') or 'image' not in event.get('result',{}):continue
    result=event['result'];p=Path(result['image'])
    if not p.exists() or digest(p)!=result['sha256']:continue # overwritten attempts are not evidence
    manifest['gui'].append(event);shutil.copy2(p,out/p.name);selected.append(p.name)
catalog=json.loads((root/'build/themes-catalog-64-debug.json').read_text())
assert len(catalog)==36
for i,item in enumerate(catalog):
    p=root/f'build/m6/eq-theme-{i}-64-debug.bmp';assert p.exists()
    Image.open(p).save(out/f'eq-{item["id"]}.png')
cards=[]
for item in catalog:
    cards.append(f'<figure><img src="eq-{item["id"]}.png"><figcaption>{html.escape(item["id"])}</figcaption></figure>')
(out/'index.html').write_text('<!doctype html><meta charset="utf-8"><title>M6: 36 temas</title><style>body{background:#eee;font:16px sans-serif}figure{display:inline-block;margin:8px}img{width:460px;display:block}figcaption{padding:8px}</style><h1>EQ: capturas das janelas reais</h1><p>Win32 PrintWindow, harness x64 Debug. Integridade automatizada; aceite estético humano pendente.</p>'+''.join(cards),encoding='utf-8')
ids=['original','dark','light','inverse','high-contrast','summer','christmas','cyberpunk']
# Match catalog IDs without guessing spelling.
available=[x['id'] for x in catalog]
chosen=[0,1,2,3,4,6,12,available.index('cyberpunk')]
sheet=Image.new('RGB',(920,4*270),'#dddddd');draw=ImageDraw.Draw(sheet)
for position,i in enumerate(chosen):
    item=catalog[i];im=Image.open(out/f'eq-{item["id"]}.png').convert('RGB');im.thumbnail((450,244))
    x=(position%2)*460;y=(position//2)*270;sheet.paste(im,(x,y));draw.text((x+8,y+246),item['id'],fill='black')
sheet.save(out/'representative-themes.png')
for folder in ('src','tests','scripts'):
    for p in (root/folder).rglob('*'):
        if p.is_file() and (p.suffix in ('.cpp','.h','.py','.ps1') or 'eq_fixtures' in p.parts):
            if 'eq' in p.name.lower() or p.name in ('app.cpp','audio.cpp','ui_theme.cpp','ui_theme.h','mainwnd.cpp','ui_player.cpp','litesic_gui.py','gui.ps1','dev.ps1','test_player_window.cpp'):
                manifest['files'].append({'path':str(p.relative_to(root)).replace('\\','/'),'sha256':digest(p)})
manifest['files'].append({'path':'Makefile','sha256':digest(root/'Makefile')})
shutil.copy2(root/'build/m6-gui-guards.log',out/'gui-guards.log')
for p in sorted(out.iterdir()):
    if p.is_file() and p.name!='manifest.json':manifest['files'].append({'path':str(p.relative_to(root)).replace('\\','/'),'sha256':digest(p)})
(out/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({'tests':manifest['tests'],'gui_captures':len(selected),'theme_captures':36,'performance':manifest['performance']},indent=2))
