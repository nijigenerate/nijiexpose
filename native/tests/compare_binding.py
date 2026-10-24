import argparse,pathlib,subprocess,difflib
p=argparse.ArgumentParser();p.add_argument('--cpp');p.add_argument('--d');p.add_argument('--reference',required=True);p.add_argument('--record',action='store_true');a=p.parse_args()
def run(exe):
    result=subprocess.run([exe],check=True,stdout=subprocess.PIPE,text=True).stdout
    # The original expression signature includes the heap address of its binding.
    return '\n'.join(line for line in result.splitlines() if not line.startswith('[INFO]'))+'\n'
path=pathlib.Path(a.reference)
if a.record:path.write_text(run(a.d),encoding='utf-8')
else:
    actual=run(a.cpp);expected=run(a.d) if a.d else path.read_text(encoding='utf-8')
    if actual!=expected:raise SystemExit(''.join(difflib.unified_diff(expected.splitlines(True),actual.splitlines(True),fromfile='D',tofile='C++')))
    print('Ratio, expression, key event, compound and late plugin update match original D')
