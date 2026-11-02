"""Refresh this source tree's hash inventory after an intentional source edit."""
import hashlib,json,pathlib,re
root=pathlib.Path(__file__).resolve().parent
path=root/'sources.json';m=json.loads(path.read_text(encoding='utf-8'))
pending=[root/p for p in m['c_sources']+m['cpp_sources']+[m['cli_source'],'examples/encode_pcm16.c','examples/roundtrip_memory.c','examples/stream_roundtrip.c']]
inputs={}
while pending:
    p=pending.pop().resolve();relative=p.relative_to(root).as_posix()
    if relative in inputs:continue
    raw=p.read_bytes();inputs[relative]=hashlib.sha256(raw).hexdigest()
    for name in re.findall(r'^\s*#\s*include\s*"([^"]+)"',raw.decode('utf-8-sig'),re.M):
        child=(p.parent/name).resolve()
        if not child.is_file():child=(root/'product'/name).resolve()
        child.relative_to(root)
        if not child.is_file():raise SystemExit('Missing local include: '+str(child))
        pending.append(child)
m['canonical_encoder_inputs_sha256']=inputs
m['canonical_gui_inputs_sha256']={p:hashlib.sha256((root/p).read_bytes()).hexdigest()for p in m['canonical_gui_inputs_sha256']}
path.write_text(json.dumps(m,indent=2)+'\n',encoding='utf-8')
print('Recorded',len(inputs),'codec/CLI/example files and',len(m['canonical_gui_inputs_sha256']),'GUI files.')
