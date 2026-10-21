"""Verify the Kaleido candidate, full extracted tree and reproducible archive."""
from pathlib import Path
import hashlib, json, sys, tempfile, zipfile
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools'))
from audit_module import audit
from reproducible_zip import write_archive, member_name
digest=lambda b:hashlib.sha256(b).hexdigest()
archive=root/'releases/KALEIDO-0.1.0-rc1-EOSM202.zip'
with zipfile.ZipFile(archive) as z:
    names=z.namelist()
    assert len(names)==len(set(n.casefold() for n in names))
    for name in names:member_name(name)
    assert 'ML/MODULES/kaleido.mo' in names
    assert not any(n.endswith(('autoexec.bin','.fir','.FIR')) for n in names)
    sums={name:sha for sha,name in (line.split('  ',1) for line in z.read('SHA256SUMS').decode().splitlines())}
    assert set(names)==set(sums)|{'SHA256SUMS'}
    for name,sha in sums.items():assert digest(z.read(name))==sha,name
    build=json.loads(z.read('BUILD.json'))
    assert build['module']=='kaleido.mo' and build['version']=='0.1.0-rc1'
    assert build['verification']['exact_installed_core_imports']
    assert build['verification']['defined_property_handlers_registered']
    assert not build['verification']['camera_loaded'] and not build['verification']['camera_capture_tested']
    for name,sha in build['source_files'].items():assert digest(z.read('source/'+name))==sha,name
    assert digest(z.read('ML/MODULES/kaleido.mo'))==build['files']['kaleido.mo']
    recipe=json.loads(z.read('PACKAGE.json'))
    with tempfile.TemporaryDirectory(prefix='kaleido-verify-') as temp:
        tree=Path(temp)/'tree';tree.mkdir();z.extractall(tree)
        rebuilt=Path(temp)/'repacked.zip';write_archive(tree,rebuilt,recipe['source_date_epoch'])
        assert rebuilt.read_bytes()==archive.read_bytes(),'Repacked ZIP differs'
        report=audit(tree/'ML/MODULES/kaleido.mo',root/'.cache/magiclantern.2025-06-20.EOSM.202.zip',tree/'source/targets/EOSM.202/EOSM_202.sym')
        assert report['module_name']=='kaleido' and not report['missing_imports']
        assert not report['unregistered_property_handlers']
    (root/'releases/SHA256SUMS').write_text(digest(archive.read_bytes())+'  '+archive.name+'\n',encoding='utf-8',newline='\n')
    print(f'PASS: {len(names)} members; every checksum; matching source/module; exact imports; reproducible ZIP')
    print('SHA256 '+digest(archive.read_bytes()))
    print('Camera load/capture remain unverified.')
