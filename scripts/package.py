from pathlib import Path
import hashlib,json,zipfile
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'dist';OUT.mkdir(exist_ok=True)

def package(root,filename,provider=False):
    if provider:
        paths=[root/p for p in ['hellforge-mod.json','icon.png','README.md','CHANGELOG.md','LICENSE','HELLFORGE-PAGE.md','THIRD-PARTY-NOTICES.md']]
        for directory in ['src','include','docs']:
            paths+=sorted(p for p in (root/directory).rglob('*')if p.is_file())
    else:
        paths=sorted(p for p in root.rglob('*')if p.is_file())
    path=OUT/filename
    with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED)as archive:
        for p in paths:archive.write(p,p.relative_to(root).as_posix())
    with zipfile.ZipFile(path)as archive:
        assert archive.testzip()is None
        assert not any(n.endswith('.dll')for n in archive.namelist())
    digest=hashlib.sha256(path.read_bytes()).hexdigest()
    path.with_suffix('.zip.sha256').write_text(f'{digest}  {path.name}\n')
    print(path)

version=json.loads((ROOT/'hellforge-mod.json').read_text())['version']
package(ROOT,f'explosives-library-{version}.zip',True)
package(ROOT/'examples/ops-debug',f'explosives-library-ops-debug-{version}.zip')
