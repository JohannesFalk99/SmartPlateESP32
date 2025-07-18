Import("env")
import os

print("[ccache.py] Script loaded")

wrapper_path = r"C:\Users\Johan\ccache_wrappers"
current_path = env['ENV'].get('PATH', '')
print(f"[ccache.py] Original PATH: {current_path}")
env['ENV']['PATH'] = wrapper_path + os.pathsep + current_path
print(f"[ccache.py] Modified PATH: {env['ENV']['PATH']}")

# Only prepend ccache to compile commands, not linker
env.Replace(
    CC = env['CC'],
    CXX = env['CXX']
)

# Modify compile commands to add ccache prefix
env['CCCOM'] = 'ccache ' + env['CCCOM']
env['CXXCOM'] = 'ccache ' + env['CXXCOM']

print(f"[ccache.py] Modified CCCOM: {env['CCCOM']}")
print(f"[ccache.py] Modified CXXCOM: {env['CXXCOM']}")
