#!/usr/bin/env python3
"""C% Test Runner — translates, compiles, and runs all example .cpc files."""

import os
import sys
import subprocess
import tempfile
import shutil
import platform

# --- Config ---
CPCT = "./cpct"
LIB_DIR = "src/lib"
LIB_SRC = "src/lib/cpct/data/type/bigint.cpp"
TIMEOUT = 10

EXAMPLE_DIRS = [
    "examples/basic",
    "examples/control",
    "examples/types",
    "examples/builtin",
    "examples/datastructure",
    "examples/_tests",
]

def find_compiler():
    """Find available C++ compiler: MSVC cl.exe or g++."""
    if shutil.which("cl"):
        return "cl", ["/std:c++20", "/EHsc", "/nologo", f"/I{LIB_DIR}"]
    if shutil.which("g++"):
        return "g++", ["-std=c++20", "-O2", f"-I{LIB_DIR}"]
    return None, []

def collect_tests():
    """Collect all .cpc files from example directories."""
    files = []
    for d in EXAMPLE_DIRS:
        if not os.path.isdir(d):
            continue
        for f in sorted(os.listdir(d)):
            if f.endswith(".cpc"):
                files.append(os.path.join(d, f))
    return files

def run_test(cpc_file, tmp_dir, compiler, cxx_flags, mode):
    """Run a single test: translate → compile → run. Returns (status, error_msg)."""
    name = os.path.splitext(os.path.basename(cpc_file))[0]
    cpp_file = os.path.join(tmp_dir, name + ".cpp")
    if platform.system() == "Windows":
        exe_file = os.path.join(tmp_dir, name + ".exe")
    else:
        exe_file = os.path.join(tmp_dir, name)

    # Step 1: Translate (using cpct --translate)
    try:
        r = subprocess.run([CPCT, "--translate", cpc_file, "-o", tmp_dir],
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=TIMEOUT)
        if r.returncode != 0:
            return "translate", r.stderr.decode(errors="replace").strip()
        # cpct --translate writes to tmp_dir/name.cpp
        expected_cpp = os.path.join(tmp_dir, name + ".cpp")
        if not os.path.exists(expected_cpp):
            return "translate", "output file not created"
    except Exception as e:
        return "translate", str(e)

    if mode == "translate":
        return "ok", ""

    # Step 2: Compile
    try:
        if compiler == "cl":
            cmd = ["cl"] + cxx_flags + [f"/Fe:{exe_file}", cpp_file, LIB_SRC]
        else:
            cmd = [compiler] + cxx_flags + ["-o", exe_file, cpp_file, LIB_SRC]
        r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60)
        if r.returncode != 0:
            err = (r.stdout.decode(errors="replace") + r.stderr.decode(errors="replace")).strip()
            lines = err.splitlines()[:3]
            return "compile", "\n".join(lines)
    except Exception as e:
        return "compile", str(e)

    if mode == "compile":
        return "ok", ""

    # Step 3: Run
    try:
        r = subprocess.run([exe_file], stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=TIMEOUT)
        if r.returncode != 0:
            err = (r.stderr.decode(errors="replace") + r.stdout.decode(errors="replace")).strip()
            return "run", err or f"exit code {r.returncode}"
    except subprocess.TimeoutExpired:
        return "run", "timeout"
    except Exception as e:
        return "run", str(e)

    return "ok", ""

def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else "all"
    if mode not in ("all", "translate", "compile"):
        print("Usage: python test.py [all|translate|compile]")
        sys.exit(1)

    compiler, cxx_flags = find_compiler()
    if mode != "translate" and compiler is None:
        print("Error: No C++ compiler found (cl or g++)")
        sys.exit(1)

    tests = collect_tests()
    if not tests:
        print("No .cpc files found.")
        sys.exit(1)

    print(f"C% Test Runner ({compiler or 'translate-only'}, {len(tests)} files)")
    print("=" * 50)

    passed, failed = 0, 0
    failures = []
    tmp_dir = tempfile.mkdtemp(prefix="cpct_test_")

    try:
        for cpc in tests:
            status, err = run_test(cpc, tmp_dir, compiler, cxx_flags, mode)
            if status == "ok":
                passed += 1
                print(f"  OK   {cpc}")
            else:
                failed += 1
                print(f"  FAIL [{status:9s}] {cpc}")
                failures.append((cpc, status, err))
    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)

    print("=" * 50)
    print(f"Results: {passed} passed, {failed} failed")

    if failures:
        print("\nFailures:")
        for cpc, stage, err in failures:
            print(f"\n  {cpc} [{stage}]")
            if err:
                for line in err.splitlines():
                    print(f"    {line}")
        sys.exit(1)

if __name__ == "__main__":
    main()
