"""Portable Zig build. Only this directory, standard Python and user-supplied Zig are required."""
from pathlib import Path
import argparse,concurrent.futures,hashlib,json,os,platform,shutil,struct,subprocess,sys,time

ROOT=Path(__file__).resolve().parent
TARGETS={'windows-x64':('x86_64-windows-gnu',0x8664),
         'macos-arm64':('aarch64-macos.11.0',0x100000c),
         'macos-x64':('x86_64-macos.11.0',0x1000007)}
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def native_target():
    if sys.platform=='win32':return 'windows-x64'
    if sys.platform=='darwin':return 'macos-arm64'if platform.machine().lower()in ['arm64','aarch64']else'macos-x64'
    return None

def run(argv,cwd=ROOT):
    p=subprocess.run([str(a)for a in argv],cwd=cwd,capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=300)
    if p.returncode:raise RuntimeError('Build command failed:\n'+json.dumps([str(a)for a in argv])+'\n'+p.stdout+p.stderr)
    return p.stdout.strip()

def verify_inputs(manifest):
    expected={**manifest['canonical_encoder_inputs_sha256'],**manifest['canonical_gui_inputs_sha256']}
    for name,digest in expected.items():
        file=(ROOT/name).resolve()
        if not file.is_relative_to(ROOT):raise RuntimeError('Source path leaves this directory: '+name)
        if not file.is_file()or sha(file)!=digest:raise RuntimeError('Source missing or changed: '+name)
    return expected

def build(zig,label,destination,manifest,jobs,gui,example):
    target,cpu=TARGETS[label];windows=label=='windows-x64';destination.mkdir(parents=True,exist_ok=True)
    objects_dir=destination/'obj';objects_dir.mkdir(exist_ok=True)
    flags=['-O2','-fno-fast-math','-ffp-contract=off','-fno-math-errno','-fPIC']
    flags+=['-D'+name for name in manifest.get('compile_definitions',[])]
    names=manifest['c_sources']+manifest['cpp_sources']
    def compile_one(pair):
        i,name=pair;cpp=name.endswith('.cpp');obj=objects_dir/('%03d-%s.o'%(i,Path(name).stem))
        run([zig,'c++'if cpp else'cc','-target',target,'-std=c++17'if cpp else'-std=c11',*flags,'-c',name,'-o',obj])
        data=obj.read_bytes()
        if windows:assert struct.unpack_from('<H',data)[0]==cpu
        else:assert data[:4]==b'\xcf\xfa\xed\xfe'and struct.unpack_from('<I',data,4)[0]==cpu
        return obj,{'source':name,'sha256':sha(obj)}
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs)as pool:compiled=list(pool.map(compile_one,enumerate(names)))
    objects=[row[0]for row in compiled]
    static=destination/('atracodec-static.lib'if windows else'libatracodec.a')
    # Replacing only this generated archive avoids stale members from older source lists.
    if static.exists():static.unlink()
    run([zig,'ar','rcs',static,*objects])
    shared=destination/('atracodec.dll'if windows else'libatracodec.dylib')
    importlib=destination/'atracodec-import.lib'
    shared_options=['cmake/atracodec.def','-Wl,--out-implib,'+str(importlib)]if windows else[
        '-Wl,-install_name,@rpath/libatracodec.dylib']
    run([zig,'c++','-target',target,*flags,'-shared',*objects,*shared_options,'-o',shared])
    main=objects_dir/'main.o';run([zig,'c++','-target',target,'-std=c++17',*flags,'-c',manifest['cli_source'],'-o',main])
    cli=destination/('atracodec.exe'if windows else'atracodec')
    run([zig,'c++','-target',target,*flags,main,static,*(['-lshell32']if windows else[]),'-o',cli])
    outputs=[static,shared,cli]+([importlib]if windows else[])
    if example:
        for name,file in [('atracodec-c-example','examples/encode_pcm16.c'),('atracodec-roundtrip-example','examples/roundtrip_memory.c'),('atracodec-stream-example','examples/stream_roundtrip.c')]:
            source=objects_dir/(name+'.o');out=destination/(name+('.exe'if windows else''))
            run([zig,'cc','-target',target,'-std=c11',*flags,'-Iproduct','-c',file,'-o',source])
            run([zig,'cc','-target',target,source,importlib if windows else shared,
                 *([]if windows else['-Wl,-rpath,@executable_path']),'-o',out]);outputs.append(out)
    report={'target':target,'c_sources':len(manifest['c_sources']),'cpp_sources':len(manifest['cpp_sources']),'compile_definitions':manifest.get('compile_definitions',[]),
            'object_architectures_checked':True,'objects':[row[1]for row in compiled],
            'artifacts':[{'file':p.name,'bytes':p.stat().st_size,'sha256':sha(p)}for p in outputs],
            'gui_built':any(p.name=='atracodec-test-gui.exe'for p in outputs),'gui_launched':False,
            'canonical_sources_modified':False,'native_macos_executed':False}
    (destination/'build-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Built '+label+': '+str(destination),flush=True);return report

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--zig',default=shutil.which('zig'))
    p.add_argument('--target',choices=['all',*TARGETS],default=native_target())
    p.add_argument('--build-dir',default='build');p.add_argument('--jobs',type=int,default=min(os.cpu_count()or 1,8))
    p.add_argument('--no-gui',action='store_true');p.add_argument('--no-example',action='store_true');a=p.parse_args()
    if not a.zig:p.error('Zig not found in PATH. Pass --zig PATH_TO_ZIG (tested with0.14.1).')
    if not a.target:p.error('Choose --target windows-x64, macos-arm64, macos-x64, or all.')
    if a.jobs<1:p.error('--jobs must be positive')
    zig=shutil.which(a.zig)or str(Path(a.zig).resolve());version=run([zig,'version'])
    manifest=json.loads((ROOT/'sources.json').read_text());expected=verify_inputs(manifest)
    destination=Path(a.build_dir).expanduser()
    if not destination.is_absolute():destination=(ROOT/destination).resolve()
    started=time.monotonic();selected=list(TARGETS)if a.target=='all'else[a.target]
    reports={name:build(zig,name,destination/name,manifest,a.jobs,not a.no_gui,not a.no_example)for name in selected}
    gui_report=None
    if not a.no_gui:
        gui_dir=destination/'native-gui'
        command=[sys.executable,'product/gui/build_gui.py','--zig',zig,'--out',gui_dir,'--target',a.target]
        for name in selected:command+=['--cli-'+name,destination/name/('atracodec.exe'if name=='windows-x64'else'atracodec')]
        run(command)
        gui_report=json.loads((gui_dir/'gui-build-verification.json').read_text(encoding='utf-8'))
        for item in gui_report['artifacts']:
            name=item['target'];out=destination/name/Path(item['path']).name;shutil.copy2(item['path'],out)
            reports[name]['artifacts'].append({'file':out.name,'bytes':out.stat().st_size,'sha256':sha(out)})
            reports[name]['gui_built']=True;reports[name]['gui_launched']=False
            if name!='windows-x64':
                source=gui_dir/name/'ATRACodec Converter.app';bundle=destination/name/source.name
                shutil.copytree(source,bundle,dirs_exist_ok=True)
                reports[name]['native_app_bundle']={'directory':bundle.name,'CLI_included':True,'native_execution_tested':False}
            (destination/name/'build-report.json').write_text(json.dumps(reports[name],indent=2)+'\n')
    assert verify_inputs(manifest)==expected
    result={'zig_version':version,'targets':reports,'input_sha256':expected,'sources_json_sha256':sha(ROOT/'sources.json'),
            'compiler_only_external_dependency':True,'original_binaries_or_fixtures_required':False,'seconds':time.monotonic()-started}
    if gui_report is not None:result['native_GUI_build_report_sha256']=sha(gui_dir/'gui-build-verification.json')
    destination.mkdir(parents=True,exist_ok=True);(destination/'build-verification.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'zig_version':version,'targets':selected,'seconds':result['seconds']}))

if __name__=='__main__':
    try:main()
    except (OSError,RuntimeError,AssertionError)as error:raise SystemExit(str(error))
