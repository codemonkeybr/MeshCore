# PlatformIO pre-script: keep examples/companion_radio/main.cpp in sync with the
# bot headers.
#
# `the_mesh` is defined in main.cpp, which includes BotMesh.h only through a
# macro (`#include MESH_CLASS_HEADER`). The dependency scanner does not follow
# macro includes, so main.cpp.o is never rebuilt when a bot header changes and
# ends up sizing `the_mesh` off a stale sizeof(BotMesh) -> out-of-bounds writes.
#
# Work around it by touching main.cpp whenever any bot header is newer, forcing
# a recompile of the translation unit that allocates the global.

import os
import glob

Import("env")

proj = env["PROJECT_DIR"]
main_cpp = os.path.join(proj, "examples", "companion_radio", "main.cpp")

headers = glob.glob(os.path.join(proj, "examples", "companion_radio_bot", "*.h"))
headers += glob.glob(os.path.join(proj, "examples", "companion_radio_bot", "commands", "*.h"))

if os.path.isfile(main_cpp) and headers:
    newest = max(os.path.getmtime(h) for h in headers)
    if os.path.getmtime(main_cpp) < newest:
        os.utime(main_cpp, None)
        print("force_main_dep: touched main.cpp (bot header changed)")
