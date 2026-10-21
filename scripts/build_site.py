"""Assemble the static playground with its exact candidate download."""
from pathlib import Path
import shutil
root=Path(__file__).resolve().parents[1]
candidate=root/'releases/KALEIDO-0.1.0-rc2-EOSM202.zip'
if not candidate.is_file():raise SystemExit('Build and package the candidate first.')
output=root/'.site-build'
shutil.copytree(root/'site',output,dirs_exist_ok=True)
(output/'downloads').mkdir(exist_ok=True)
shutil.copy2(candidate,output/'downloads'/candidate.name)
print('Static site assembled:',output)
