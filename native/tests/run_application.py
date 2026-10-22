import json,os,pathlib,struct,subprocess,sys,socket,zlib
root=pathlib.Path(sys.argv[2]).resolve();root.mkdir(parents=True,exist_ok=True)
config=root/'config';config.mkdir(exist_ok=True)
def png_chunk(kind, data):
    return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
(root/'drag-image.png').write_bytes(b'\x89PNG\r\n\x1a\n'+png_chunk(b'IHDR',struct.pack('>IIBBBBB',120,120,8,6,0,0,0))+png_chunk(b'IDAT',zlib.compress((b'\0'+bytes([0,0,255,255])*120)*120))+png_chunk(b'IEND',b''))
probe=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);probe.bind(('127.0.0.1',0));port=probe.getsockname()[1];probe.close()
(config/'settings.json').write_text(json.dumps({'window':{'width':1024,'height':768},'throttlingRate':0,'shouldPostProcess':False,'bgColor':[.92,.94,.97,1],'space':{'zones':[{'name':'Local verification','sources':[{'type':'VMC Receiver','options':{'port':str(port)}}]}]}}))
b=(pathlib.Path(__file__).parent/'fixtures/original-model.bin').read_bytes();n=struct.unpack('>I',b[8:12])[0];doc=json.loads(b[12:12+n])
doc['meta']['name']='Motion verification fixture'
doc['nodes']['mesh']['verts']=[v*40 for v in doc['nodes']['mesh']['verts']]
doc['param'][0]['bindings']=[{'node':doc['nodes']['uuid'],'param_name':'transform.t.x','values':[[0],[400]],'isSet':[[True],[True]],'interpolate_mode':'Linear'}]
doc['animations']['Move']['length']=121
doc['animations']['Move']['lanes'][0]['keyframes'][1]['frame']=120
encoded=json.dumps(doc,separators=(',',':')).encode();model=root/'motion.inp';model.write_bytes(b[:8]+struct.pack('>I',len(encoded))+encoded+b[12+n:])
for i in range(3):
    plugin=config/'plugins'/f'fixture{i}';plugin.mkdir(parents=True,exist_ok=True)
    (plugin/'info.lua').write_text('{pluginId="ignored",pluginName="UI Plugin '+str(i)+'",pluginVersion="1"}')
    (plugin/'init.lua').write_text("updates=0;clicks=0;function onUpdate(dt) updates=updates+1 end;function onRenderUI() if ui.button('Increment') then clicks=clicks+1 end;ui.label('Clicks: '..tostring(clicks)) end;function verifyUI() assert(clicks==1 and updates>0) end")
result=subprocess.run([sys.argv[1],str(model),str(root)],env=dict(os.environ,IN_CONFIG_PATH=str(config),NIJI_TEST_PORT=str(port)),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding='utf-8',errors='replace',timeout=90)
(root/'application.log').write_text(result.stdout,encoding='utf-8')
lines=result.stdout.splitlines()
captures=[line[8:] for line in lines if line.startswith('CAPTURE ')]
observations=[line for line in lines if line and not line.startswith(('[','CAPTURE '))]
report={'status':'passed' if result.returncode==0 else 'failed','exit_code':result.returncode,'fixture':'Synthetic INP with a parameter-driven translation, derived from the D-authored fixture','application_path':'initializeApplication / original main window / actual mouse and keyboard input','observations':observations,'captures':captures}
(root/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print('\n'.join(observations))
print('Application actions: '+report['status'])
sys.exit(result.returncode)
