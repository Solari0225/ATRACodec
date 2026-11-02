#!/usr/bin/env python3
"""Build native GUI executables/bundles without starting them.

Use --cli-{windows-x64,macos-x64,macos-arm64} to include a matching real CLI.
The minimal Apple .tbd files describe imports only; no stub goes in the bundle.
This helper deliberately does not edit the codec's top-level build manifests.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shutil
import struct
import subprocess
import zipfile

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def pe_imports(path):
    b = Path(path).read_bytes()
    pe = struct.unpack_from('<I', b, 0x3c)[0]
    if b[:2] != b'MZ' or b[pe:pe+4] != b'PE\0\0':
        raise ValueError('not PE')
    machine, count = struct.unpack_from('<HH', b, pe+4)
    optional_size = struct.unpack_from('<H', b, pe+20)[0]
    opt = pe+24
    magic = struct.unpack_from('<H', b, opt)[0]
    directories = opt+(112 if magic == 0x20b else 96)
    imports_rva = struct.unpack_from('<I', b, directories+8)[0]
    sections = []
    for n in range(count):
        row = opt+optional_size+40*n
        vs, va, rs, raw = struct.unpack_from('<IIII', b, row+8)
        sections.append((va, max(vs, rs), raw))

    def offset(rva):
        for va, size, raw in sections:
            if va <= rva < va+size:
                return raw+rva-va
        if rva < opt+optional_size+40*count:
            return rva
        raise ValueError('invalid RVA')

    imports = []
    if imports_rva:
        row = offset(imports_rva)
        while any(b[row:row+20]):
            name_rva = struct.unpack_from('<I', b, row+12)[0]
            start = offset(name_rva)
            imports.append(b[start:b.index(0, start)].decode('ascii'))
            row += 20
    return {'format': 'PE32+' if magic == 0x20b else 'PE32',
            'machine': hex(machine), 'subsystem': struct.unpack_from('<H', b, opt+68)[0],
            'imports': sorted(imports)}


def macho_loads(path):
    b = Path(path).read_bytes()
    if b[:4] != b'\xcf\xfa\xed\xfe':
        raise ValueError('expected thin Mach-O64')
    cpu, subtype, kind, commands = struct.unpack_from('<IIII', b, 4)
    row, deps, minimum, signature = 32, [], None, False
    for _ in range(commands):
        cmd, size = struct.unpack_from('<II', b, row)
        if size < 8 or row+size > len(b):
            raise ValueError('invalid load command')
        if cmd in (0xc, 0x18, 0x1f, 0x80000018, 0x8000001f, 0x80000023):
            start = row+struct.unpack_from('<I', b, row+8)[0]
            deps.append(b[start:b.index(0, start)].decode())
        elif cmd == 0x32:
            minimum = struct.unpack_from('<I', b, row+12)[0]
        elif cmd == 0x24:
            minimum = struct.unpack_from('<I', b, row+8)[0]
        elif cmd == 0x1d:
            signature = True
        row += size
    return {'format': 'Mach-O64', 'cpu': hex(cpu), 'subtype': subtype, 'filetype': kind,
            'minimum_macos': '.'.join(str(x) for x in (minimum >> 16, (minimum >> 8) & 255, minimum & 255)) if minimum else None,
            'load_dylibs': sorted(deps), 'has_code_signature_command': signature,
            'native_execution_tested': False}


def package_app(output_dir, executable, cli=None):
    app = Path(output_dir)/'ATRACodec Converter.app'
    contents = app/'Contents'
    macos = contents/'MacOS'
    macos.mkdir(parents=True, exist_ok=True)
    gui = macos/'atracodec-converter'
    if gui.resolve() != Path(executable).resolve():
        shutil.copyfile(executable, gui)
    gui.chmod(0o755)
    if cli:
        cli = Path(cli).resolve(strict=True)
        if cli != (macos/'atracodec').resolve():
            shutil.copyfile(cli, macos/'atracodec')
        (macos/'atracodec').chmod(0o755)
    data = {
        'CFBundleExecutable': 'atracodec-converter',
        'CFBundleIdentifier': 'org.atracodec.converter',
        'CFBundleName': 'ATRACodec Converter',
        'CFBundleDisplayName': 'ATRACodec Converter',
        'CFBundlePackageType': 'APPL',
        'CFBundleShortVersionString': '0.5.0',
        'CFBundleVersion': '0.5.0',
        'CFBundleDevelopmentRegion': 'en',
        'CFBundleLocalizations': ['en', 'zh-Hans', 'zh-Hant'],
        'LSMinimumSystemVersion': '11.0',
        'LSApplicationCategoryType': 'public.app-category.music',
        'NSHighResolutionCapable': True,
        'NSPrincipalClass': 'NSApplication',
    }
    (contents/'Info.plist').write_bytes(plistlib.dumps(data, sort_keys=False))
    for language, name in [('en', 'ATRACodec Converter'), ('zh-Hans', 'ATRACodec 音频转换器'), ('zh-Hant', 'ATRACodec 音频转换器')]:
        folder = contents/'Resources'/f'{language}.lproj'
        folder.mkdir(parents=True, exist_ok=True)
        (folder/'InfoPlist.strings').write_text(f'"CFBundleDisplayName" = "{name}";\n', encoding='utf-8')
    return {'path': str(app), 'cli_included': bool(cli),
            'files': [{'relative': str(p.relative_to(app)), 'bytes': p.stat().st_size, 'sha256': sha(p),
                       'archive_posix_mode': '0755' if p.parent.name == 'MacOS' else '0644'}
                      for p in sorted(app.rglob('*')) if p.is_file()]}


def zip_app(app, destination):
    """Preserve executable permission bits even when packaging on Windows."""
    app, destination = Path(app), Path(destination)
    with zipfile.ZipFile(destination, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for p in sorted(app.rglob('*')):
            if not p.is_file():
                continue
            info = zipfile.ZipInfo(str(p.relative_to(app.parent)).replace('\\', '/'))
            info.create_system = 3
            info.external_attr = (0o100755 if p.parent.name == 'MacOS' else 0o100644) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, p.read_bytes())
    return {'path': str(destination), 'bytes': destination.stat().st_size, 'sha256': sha(destination),
            'executable_permissions_preserved': True}


def run(command, commands, cwd=HERE):
    command = [str(x) for x in command]
    p = subprocess.run(command, cwd=cwd, capture_output=True, text=True, encoding='utf-8', errors='replace')
    commands.append({'argv': command, 'cwd': str(cwd), 'exit_code': p.returncode,
                     'stdout': p.stdout, 'stderr': p.stderr})
    if p.returncode:
        raise RuntimeError(p.stderr or p.stdout or f'build exit {p.returncode}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--zig', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--target', choices=['all', 'windows-x64', 'macos-x64', 'macos-arm64'], default='all')
    for arch in ['windows-x64', 'macos-x64', 'macos-arm64']:
        parser.add_argument('--cli-'+arch, type=Path)
    args = parser.parse_args()
    zig, out = args.zig.resolve(strict=True), args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    commands, artifacts, bundles = [], [], []
    targets = ['windows-x64', 'macos-x64', 'macos-arm64'] if args.target == 'all' else [args.target]
    for target in targets:
        folder = out/target
        folder.mkdir(exist_ok=True)
        cli = getattr(args, 'cli_'+target.replace('-', '_'))
        if target == 'windows-x64':
            resource = folder/'converter.res'
            run([zig, 'rc', '/fo', resource, HERE/'converter_windows.rc'], commands)
            executable = folder/'atracodec-converter.exe'
            run([zig, 'c++', '-target', 'x86_64-windows-gnu', '-std=c++17', '-O2', '-municode',
                 '-Wl,--subsystem,windows', '-Wl,-e,WinMainCRTStartup', HERE/'converter_windows.cpp',
                 resource, '-luser32', '-lgdi32', '-lcomdlg32', '-lcomctl32', '-o', executable], commands)
            info = pe_imports(executable)
            if info['subsystem'] != 2:
                raise RuntimeError('GUI must use Windows subsystem')
            if cli:
                shutil.copyfile(cli, folder/'atracodec.exe')
        else:
            executable = folder/'atracodec-converter'
            triple = 'x86_64-macos.11.0' if target == 'macos-x64' else 'aarch64-macos.11.0'
            run([zig, 'c++', '-target', triple, '-std=c++17', '-O2', HERE/'converter_macos.cpp',
                 '-F'+str(HERE/'macos-stubs/System/Library/Frameworks'),
                 '-L'+str(HERE/'macos-stubs/usr/lib'), '-framework', 'AppKit', '-framework', 'Foundation',
                 '-lobjc', '-o', executable], commands)
            info = macho_loads(executable)
            bundle = package_app(folder, executable, cli)
            bundle['archive'] = zip_app(bundle['path'], folder/'ATRACodec Converter.app.zip')
            bundles.append(bundle)
        artifacts.append({'target': target, 'path': str(executable), 'bytes': executable.stat().st_size,
                          'sha256': sha(executable), **info})
    inputs = [p for p in HERE.rglob('*') if p.is_file() and p.suffix in {'.cpp', '.h', '.rc', '.manifest', '.tbd', '.py', '.md', '.json', '.cmake', '.in', '.strings'}]
    report = {'schema': 'atracodec-native-gui-build-v1', 'gui_launched': False,
              'macos_native_execution_tested': False, 'compiler': {'path': str(zig), 'sha256': sha(zig)},
              'inputs': [{'relative': str(p.relative_to(HERE)), 'bytes': p.stat().st_size, 'sha256': sha(p)} for p in sorted(inputs)],
              'commands': commands, 'artifacts': artifacts, 'bundles': bundles}
    (out/'gui-build-verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'ok': True, 'artifacts': len(artifacts), 'bundles': len(bundles),
                      'manifest': str(out/'gui-build-verification.json'), 'gui_launched': False}))


if __name__ == '__main__':
    main()
