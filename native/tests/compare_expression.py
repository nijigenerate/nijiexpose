import argparse,pathlib,subprocess,difflib
parser=argparse.ArgumentParser();parser.add_argument('--cpp');parser.add_argument('--d');parser.add_argument('--reference',required=True);parser.add_argument('--record',action='store_true');args=parser.parse_args()
expressions=['','1+2*3','time()','BLEND("missing")','BONE_X("missing")','ROLL("missing")']
for name in ['sin','cos','tan','sinh','cosh','tanh','psin','pcos','ptan','usin','ucos','utan','abs','sqrt','floor','ceil','round','degrees','radians','simplex','usimplex','sign']:
    for value in ['0','0.25','-1.5','3.141592653589793']:expressions.append(f'{name}({value})')
expressions+=['min(-1,2)','max(-1,2)','clamp(2,0,1)','lerp(1,3,0.25)','cubic(1,2,3,4,0.73)','atan2(0.3,-0.7)','sin()','sin(1,2)','sin("1")','false','"hello"','missing()','1+','os.time()','math.sin(1)','sign(0/0)']
def run(exe):
    return subprocess.run([exe],input='\n'.join(expressions)+'\n',text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True).stdout.replace('\r\n','\n')
path=pathlib.Path(args.reference)
if args.record:path.write_text(run(args.d),encoding='utf-8')
else:
    actual=run(args.cpp);expected=run(args.d) if args.d else path.read_text(encoding='utf-8')
    if actual!=expected:raise SystemExit(''.join(difflib.unified_diff(expected.splitlines(True),actual.splitlines(True),fromfile='D',tofile='C++')))
    print(f'{len(expressions)} original Lua expressions: values and errors match D')
