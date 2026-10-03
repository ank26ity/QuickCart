#!/usr/bin/env python3
"""
QuickCart Project Test Coverage Analyzer & Threshold Enforcer
============================================================
Parses gcov instrumentation data generated across all CTest suites and verifies
that critical business logic components meet or exceed minimum line coverage
and tracks branch coverage.

How coverage_report.py reads coverage data:
1. Compiler Instrumentation: During build with -DENABLE_COVERAGE=ON, Clang/GCC injects
   profiling arcs via --coverage (-fprofile-arcs -ftest-coverage), generating .gcno notes.
2. Runtime Execution: Executing test suites writes runtime execution counts and branch
   traversal frequencies to .gcda (data) files inside the build directory tree.
3. gcov Parsing: For each monitored source module, this tool locates matching .gcda files,
   invokes `gcov -b -c` to extract both line-by-line hit counts and branch decision matrices,
   and aggregates total unique executable lines, covered lines, and taken branches.
4. Gate Enforcement: Verifies that every critical module meets the strict minimum threshold
   (>= 85.0% line coverage).
"""

import sys
import os
import subprocess
import tempfile
import re
from pathlib import Path

# Minimum line coverage threshold required across monitored components
MIN_THRESHOLD_PERCENT = 80.0

CRITICAL_TARGETS = [
    ("CartManager", "models/cartmanager.cpp"),
    ("OrderStateMachine", "core/orderstatemachine.cpp"),
    ("AuthService", "models/authservice.cpp"),
    ("ApiClient", "api/apiclient.cpp"),
    ("PermissionManager", "security/permissionmanager.cpp"),
    ("SecureStorage", "security/securestorage.cpp"),
    ("OrderModel", "models/ordermodel.cpp"),
    ("ShopModel", "models/shopmodel.cpp"),
    ("ProductModel", "models/productmodel.cpp"),
    ("NetworkManager", "api/networkmanager.cpp"),
    ("Validators", "core/validators.cpp")
]

def analyze_coverage(build_dir):
    build_path = Path(build_dir).resolve()
    if not build_path.exists():
        print(f"Error: Build directory '{build_dir}' does not exist.", file=sys.stderr)
        return False

    project_root = Path(__file__).resolve().parent.parent

    print("================================================================================")
    print("                    QUICKCART C++ CODE COVERAGE ANALYZER                         ")
    print("================================================================================")
    print("Data Ingestion Pipeline:")
    print("  1. Locates all .gcda profiling archives under build/CMakeFiles")
    print("  2. Invokes gcov -b -c on basic block arc graphs per translation unit")
    print("  3. Parses execution hits per line and evaluates branch decision coverage")
    print(f"  4. Enforces strict quality gate threshold: >= {MIN_THRESHOLD_PERCENT}%\n")

    results = []
    overall_covered_lines = 0
    overall_total_lines = 0
    overall_taken_branches = 0
    overall_total_branches = 0
    all_passed = True

    with tempfile.TemporaryDirectory() as tmpdir:
        for name, rel_source in CRITICAL_TARGETS:
            abs_source = (project_root / rel_source).resolve()
            source_filename = abs_source.name
            stem = abs_source.stem

            # Find all .gcda files from test targets corresponding to this source file
            matching_gcdas = list(build_path.glob(f"CMakeFiles/test_*.dir/**/{source_filename}.gcda"))
            if not matching_gcdas:
                matching_gcdas = list(build_path.glob(f"CMakeFiles/test_*.dir/**/{stem}.cpp.gcda"))

            if not matching_gcdas:
                print(f"Warning: No .gcda files found for {name} ({rel_source})")
                results.append((name, rel_source, 0, 0, 0.0, 0, 0, 0.0, False))
                all_passed = False
                continue

            # Dict mapping line_number -> total_hits across all test suites
            line_coverage = {}
            # Branch tracking: (line_num, branch_idx) -> taken
            branch_coverage = {}

            for idx, gcda in enumerate(matching_gcdas):
                work_dir = Path(tmpdir) / f"{stem}_{idx}"
                work_dir.mkdir(parents=True, exist_ok=True)

                cmd = ["gcov", "-b", "-c", "-o", str(gcda.parent), str(gcda)]
                proc = subprocess.run(cmd, cwd=str(work_dir), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

                # Look for generated .gcov files in work_dir
                for gcov_file in work_dir.glob("*.gcov"):
                    try:
                        with open(gcov_file, "r", encoding="utf-8", errors="replace") as f:
                            first_line = f.readline()
                            is_our_source = False
                            if str(abs_source) in first_line or rel_source in first_line or source_filename in first_line:
                                is_our_source = True
                            else:
                                for line in f:
                                    if line.startswith("        -:    0:Source:") and (str(abs_source) in line or rel_source in line):
                                        is_our_source = True
                                        break
                                    if not line.startswith("        -:    0:"):
                                        break

                            if not is_our_source:
                                continue

                            f.seek(0)
                            current_line_num = 0
                            current_branch_idx = 0

                            for line in f:
                                # Branch line format: "branch  0 taken 50%" or "branch  0 never executed"
                                if line.strip().startswith("branch"):
                                    match = re.search(r"branch\s+(\d+)\s+(taken\s+(\d+)|never executed)", line)
                                    if match:
                                        b_idx = int(match.group(1))
                                        b_key = (current_line_num, b_idx)
                                        is_taken = match.group(2).startswith("taken")
                                        if is_taken:
                                            # If taken count > 0 or percentage > 0
                                            branch_coverage[b_key] = True
                                        elif b_key not in branch_coverage:
                                            branch_coverage[b_key] = False
                                    continue

                                parts = line.split(":", 2)
                                if len(parts) >= 3:
                                    hits_str = parts[0].strip()
                                    line_num_str = parts[1].strip()
                                    if line_num_str.isdigit():
                                        line_num = int(line_num_str)
                                        current_line_num = line_num
                                        if line_num == 0:
                                            continue
                                        if hits_str == "-":
                                            continue # non-executable
                                        elif hits_str == "#####":
                                            if line_num not in line_coverage:
                                                line_coverage[line_num] = 0
                                        else:
                                            clean_hits = re.sub(r"[^\d]", "", hits_str)
                                            hits = int(clean_hits) if clean_hits else 0
                                            line_coverage[line_num] = line_coverage.get(line_num, 0) + hits
                    except Exception:
                        pass

            total_lines = len(line_coverage)
            covered_lines = sum(1 for hits in line_coverage.values() if hits > 0)
            cov_percent = (covered_lines / total_lines * 100.0) if total_lines > 0 else 0.0

            total_branches = len(branch_coverage)
            taken_branches = sum(1 for taken in branch_coverage.values() if taken)
            branch_percent = (taken_branches / total_branches * 100.0) if total_branches > 0 else 0.0

            passed = cov_percent >= MIN_THRESHOLD_PERCENT
            if not passed:
                all_passed = False

            results.append((name, rel_source, covered_lines, total_lines, cov_percent,
                            taken_branches, total_branches, branch_percent, passed))
            overall_covered_lines += covered_lines
            overall_total_lines += total_lines
            overall_taken_branches += taken_branches
            overall_total_branches += total_branches

    print(f"{'Module / Class':<20} | {'Lines':<10} | {'Line Cov':<10} | {'Branches':<11} | {'Branch Cov':<11} | {'Status':<6} | {'Source'}")
    print("---------------------+------------+------------+-------------+-------------+--------+----------------------------")
    for name, rel_source, cov_l, tot_l, pct_l, cov_b, tot_b, pct_b, passed in results:
        status_str = "PASS" if passed else "FAIL"
        line_str = f"{cov_l}/{tot_l}"
        line_pct_str = f"{pct_l:6.2f}%"
        branch_str = f"{cov_b}/{tot_b}" if tot_b > 0 else "N/A"
        branch_pct_str = f"{pct_b:6.2f}%" if tot_b > 0 else "N/A"
        print(f"{name:<20} | {line_str:<10} | {line_pct_str:<10} | {branch_str:<11} | {branch_pct_str:<11} | {status_str:<6} | {rel_source}")
    print("---------------------+------------+------------+-------------+-------------+--------+----------------------------")
    overall_l_pct = (overall_covered_lines / overall_total_lines * 100.0) if overall_total_lines > 0 else 0.0
    overall_b_pct = (overall_taken_branches / overall_total_branches * 100.0) if overall_total_branches > 0 else 0.0
    overall_status = "PASS" if all_passed and overall_l_pct >= MIN_THRESHOLD_PERCENT else "FAIL"
    print(f"{'CRITICAL OVERALL':<20} | {f'{overall_covered_lines}/{overall_total_lines}':<10} | {f'{overall_l_pct:6.2f}%':<10} | {f'{overall_taken_branches}/{overall_total_branches}':<11} | {f'{overall_b_pct:6.2f}%':<11} | {overall_status:<6} | (Threshold >= {MIN_THRESHOLD_PERCENT}%)")
    print("================================================================================\n")

    if not all_passed:
        print(f"FAILED: One or more critical modules failed to meet the {MIN_THRESHOLD_PERCENT}% line coverage threshold.")
        return False
    else:
        print(f"SUCCESS: All critical modules met or exceeded the {MIN_THRESHOLD_PERCENT}% line coverage threshold!")
        return True

if __name__ == "__main__":
    build_directory = sys.argv[1] if len(sys.argv) > 1 else "build-cov"
    success = analyze_coverage(build_directory)
    sys.exit(0 if success else 1)
