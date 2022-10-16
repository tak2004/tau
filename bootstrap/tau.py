import os
import sys
from pathlib import Path
from tau_parser import build as parse
from tau_compiler import build as compile
from tau_ast import *

if __name__ == '__main__':
    module_dir = Path(os.getenv('APPDATA'))/"tau"/"modules"
    module_dir.mkdir(parents=True, exist_ok=True)
    module_directory = module_dir.as_posix()+'/'
    cache_dir = Path(os.getenv('APPDATA'))/"tau"/"cache"
    cache_dir.mkdir(parents=True, exist_ok=True)
    cache_directory = cache_dir.as_posix()+'/'
    tau_dir = Path(os.curdir)
    tau_directory = tau_dir.as_posix()+'/'

    build_modules = []
    build_app = ''
    for arg in sys.argv[1:]:
        if os.path.isfile(arg):
            build_app = arg
        else:
            build_modules.append(arg)
    
    compile_modules = []
    for mod in build_modules:
        compile_modules.append(parse(mod, module_directory, cache_directory, tau_directory))
    for mod in compile_modules:
        compile(mod, module_directory, cache_directory, tau_directory)
    if build_app != '':        
        module = parse(build_app, module_directory, cache_directory, tau_directory)
        linkModule = cache_directory+module[:3]+'.cpp'        
        compile(module, module_directory, cache_directory, tau_directory, linkModule)