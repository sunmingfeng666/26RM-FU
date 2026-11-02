"""Build the real slave7 app against host GPIO/time/LED stubs; no hardware access."""
from pathlib import Path
import os
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
board = root / "wanwanwanfufu/dbus_switch_energy/fu_slave7"
out = root / "build/slave7-host-tests"
out.mkdir(parents=True, exist_ok=True)
gcc = shutil.which("gcc")
if not gcc:
    raise SystemExit("Host gcc is required (MSYS2 UCRT64 on Windows).")

args = [gcc, "-std=c11", "-O0", "-g", "-Wall", "-Wextra", "-Werror"]
if os.environ.get("HOST_GCC_PREFIX"):
    args += ["-B", os.environ["HOST_GCC_PREFIX"]]
args += ["-I" + os.path.relpath(board / p, out) for p in ("tests/stubs", "user/APP")]
args += [os.path.relpath(board / p, out) for p in ("user/APP/slave_key_test.c", "tests/test_slave_key.c")]
exe = out / ("test_slave7.exe" if os.name == "nt" else "test_slave7")
subprocess.run(args + ["-o", exe.name], cwd=out, check=True)
subprocess.run([str(exe)], check=True)
