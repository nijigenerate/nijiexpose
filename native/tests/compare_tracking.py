"""Exercise original D and native receivers through actual local UDP sockets."""
import argparse, pathlib, socket, struct, subprocess, json
def string(s):
    b=s.encode()+b'\0'
    return b+b'\0'*((-len(b))%4)
def message(path,*args):
    tags=''; data=b''
    for v in args:
        if isinstance(v,str): tags+='s'; data+=string(v)
        elif isinstance(v,int): tags+='i'; data+=struct.pack('>i',v)
        else: tags+='f'; data+=struct.pack('>f',v)
    return string(path)+string(','+tags)+data
def bundle(*msgs):
    return b'#bundle\0'+b'\0'*8+b''.join(struct.pack('>I',len(m))+m for m in msgs)
def exercise(exe):
    lines=[]
    with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as sender:
        for mode in ['vmc','phiz','llf','ifm','fm3d','osf','vts']:
            with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as reserve:
                reserve.bind(('127.0.0.1',0)); port=reserve.getsockname()[1]
            if mode=='llf': port=11111
            if mode in ('ifm','fm3d'): port=49983
            if mode=='vts': port=21412
            p=subprocess.Popen([exe,mode,str(port)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
            try:
                assert p.stdout.readline().strip()=='READY'
                if mode=='vmc':
                    steps=[(None,1),(bundle(message('/VMC/Ext/Blend/Val','Smile',0.625),bundle(message('/VMC/Ext/Bone/Pos','Head',1.,-2.,3.,0.1,0.2,0.3,0.4),message('/VMC/Ext/Key',1,'a',97),message('/VMC/Ext/Key',1,'accent',233))),1),
                           (message('/Other/Ext/Blend/Val','PermittedByAnd',-0.75),1),(message('/Other/Other/Blend/Val','Ignored',1.),1),(None,16),(None,1),
                           (message('/VMC/Ext/Key',0,'A',65),1)]
                elif mode=='phiz':
                    steps=[(None,1),(bundle(message('/phiz/headRotation',0.1,0.2,0.3,0.4),message('/phiz/leftEyeRotation',0.,-0.5,0.5,1.),message('/phiz/rightEyeRotation',-1.,0.,1.,0.)),1),
                           (message('/phiz/blendshapes',*[float(i)/64 for i in range(52)]),1),(None,16),(None,1)]
                elif mode=='llf':
                    header=bytes(43)+struct.pack('>H',4)+b'test'+bytes(17)
                    steps=[(None,1),(bytes(45),1),(header+b''.join(struct.pack('>f',(i-30)/32) for i in range(61)),1),
                           (header+struct.pack('>ff',1.,-1.),1),(None,16),(None,1)]
                elif mode=='osf':
                    def osf(frame,points=True):
                        prefix=struct.pack('<di4f?f10f',1.25,7,640.,480.,0.75,0.25,points,0.01,0.1,0.2,0.3,0.9,10.,20.,30.,1.,-2.,3.)
                        positions=[(i/100,i/200,i/300) for i in range(70)]
                        positions[66]=(frame-2.,frame/3-1.,1.-frame/2)
                        positions[67]=(1.-frame/2,frame/4-1.,frame-2.)
                        positions[68]=positions[69]=(0.,0.,0.)
                        return prefix+struct.pack('<68f',*([0.5]*68))+struct.pack('<136f',*([0.25]*136))+b''.join(struct.pack('<3f',*v) for v in positions)+struct.pack('<14f',*(i/16 for i in range(14)))
                    steps=[(None,1),(bytes(12),1)]+[(osf(i),1) for i in range(6)]+[(osf(1,False),1),(None,16),(None,1)]
                elif mode=='vts':
                    keys=[base+side for side in ('Left','Right') for base in ('eyeBlink','eyeLookOut','eyeLookIn','eyeLookUp','eyeLookDown','eyeSquint','eyeWide','mouthLowerDown','mouthUpperUp','mouthSmile','mouthFrown')]+['mouthLeft','mouthRight','jawOpen']
                    def vts(format,found=True):
                        def rename(k):
                            if format=='ios':return k[0].upper()+k[1:]
                            if format=='android':
                                if k.startswith('eyeBlink'):return k[0].upper()+k[1:]
                                if k not in ('mouthLeft','mouthRight'):return k.replace('Left','_L').replace('Right','_R')
                            return k
                        return json.dumps({'Timestamp':1234,'Hotkey':2,'FaceFound':found,'Rotation':{'x':10,'y':20,'z':30},'Position':{'x':1,'y':2,'z':3},'BlendShapes':[{'k':rename(k),'v':i/32} for i,k in enumerate(keys)]}).encode()
                    steps=[(None,1),(vts('android'),1),(vts('meowface'),1),(vts('ios'),1),(vts('ios',False),1),(None,16),(None,1)]
                else:
                    keys=[base+side for side in ('Left','Right') for base in ('eyeBlink','eyeLookOut','eyeLookIn','eyeLookUp','eyeLookDown','eyeSquint','eyeWide','mouthLowerDown','mouthUpperUp','mouthSmile','mouthFrown')]+['mouthLeft','mouthRight']
                    if mode=='ifm': keys=[k.replace('Left','_L').replace('Right','_R') if k not in ('mouthLeft','mouthRight') else k for k in keys]
                    delimiter='-' if mode=='ifm' else '&'
                    shapes='|'.join(k+delimiter+str((i*7)%101) for i,k in enumerate(keys))+'|'
                    packet=(shapes+'=head#10,20,30,1,2,3|leftEye#2,3,4|rightEye#5,6,7|').encode()
                    steps=[(None,1),(packet,1),(None,161),(None,1)]
                for packet,count in steps:
                    if packet: sender.sendto(packet,('127.0.0.1',port))
                    p.stdin.write(str(count)+'\n'); p.stdin.flush()
                    lines.append(mode)
                    while True:
                        line=p.stdout.readline().strip()
                        if line=='END': break
                        if not line: raise RuntimeError(p.stderr.read())
                        lines.append(line)
                p.stdin.write('quit\n'); p.stdin.flush()
                assert p.stdout.readline().strip()=='STOPPED'
                assert p.wait(timeout=5)==0
            finally:
                if p.poll() is None: p.kill(); p.wait()
    return '\n'.join(lines)+'\n'
if __name__=='__main__':
    parser=argparse.ArgumentParser(); parser.add_argument('--cpp'); parser.add_argument('--d'); parser.add_argument('--reference',required=True); parser.add_argument('--record',action='store_true'); args=parser.parse_args()
    path=pathlib.Path(args.reference)
    if args.record:
        assert args.d
        path.write_text(exercise(args.d),encoding='utf-8')
    else:
        actual=exercise(args.cpp); expected=exercise(args.d) if args.d else path.read_text(encoding='utf-8')
        if actual!=expected:
            import difflib
            raise SystemExit(''.join(difflib.unified_diff(expected.splitlines(True),actual.splitlines(True),fromfile='D',tofile='C++')))
        print('All 7 app tracking receivers: 44 UDP snapshots match D, including nested bundle, gaze, bones, blendshapes, keys, truncation and timeout')
