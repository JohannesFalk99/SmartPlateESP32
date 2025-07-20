# extra_script.py
Import("env")
import os
import shutil

def copy_compile_db(source, target, env):
    src = os.path.join(env['PROJECT_BUILD_DIR'], 'compile_commands.json')
    dst = os.path.join(env['PROJECT_DIR'], 'compile_commands.json')
    if os.path.exists(src):
        shutil.copy(src, dst)
        print("[Clangd] compile_commands.json copied to project root")
    else:
        print("[Clangd] compile_commands.json not found")

env.AddPostAction("buildprog", copy_compile_db)
