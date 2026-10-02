#!/usr/bin/env python3
"""
QuickCart Project Test Coverage Analyzer & Threshold Enforcer
Parses gcov data generated across all CTest suites and verifies that
critical business logic components meet or exceed minimum line coverage.
"""

import sys
import os
import subprocess
import tempfile
import re
from pathlib import Path

# Minimum line coverage threshold required across monitored components
MIN_THRESHOLD_PERCENT = 75.0

CRITICAL_TARGETS = [
    ("CartManager", "models/cartmanager.cpp"),
    ("OrderStateMachine", "core/orderstatemachine.cpp"),
    ("AuthService", "models/authservice.cpp"),
    ("ApiClient", "api/apiclient.cpp"),
    ("PermissionManager", "security/permissionmanager.cpp")
]

def analyze_coverage(build_dir):
    build_path = Path(build_dir).resolve()
    if not build_path.exists():
        print(f"Error: Build directory '{build_dir}' does not exist.", file=sys.stderr)
        return False

    project_root = Path(__file__).resolve().parent.parent

    results = []
    overall_covered = 0
    overall_total = 0
    all_passed = True

    with tempfile.TemporaryDirectory() as tmpdir:
        for name, rel_source in CRITICAL_TARGETS:
            abs_source = (project_root / rel_source).resolve()
            source_filename = abs_source.name
            stem = abs_source.stem
            
            # Find all .gcda files corresponding to this source file
            matching_gcdas = list(build_path.glob(f"CMakeFiles/*.dir/**/{source_filename}.gcda"))
            if not matching_gcdas:
                # Try finding any gcda with the stem
                matching_gcdas = list(build_path.glob(f"CMakeFiles/*.dir/**/{stem}.cpp.gcda"))

            if not matching_gcdas:
                print(f"Warning: No .gcda files found for {name} ({rel_source})")
                results.append((name, rel_source, 0, 0, 0.0, False))
                all_passed = False
                continue

            # Dict mapping line_number -> total_hits across all test suites
            line_coverage = {}

            for idx, gcda in enumerate(matching_gcdas):
                work_dir = Path(tmpdir) / f"{stem}_{idx}"
                work_dir.mkdir(parents=True, exist_ok=True)
                
                cmd = ["gcov", "-o", str(gcda.parent), str(gcda)]
                proc = subprocess.run(cmd, cwd=str(work_dir), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                
                # Look for generated .gcov files in work_dir
                for gcov_file in work_dir.glob("*.gcov"):
                    try:
                        with open(gcov_file, "r", encoding="utf-8", errors="replace") as f:
                            first_line = f.readline()
                            # Check if this gcov file belongs to our source file
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
                            for line in f:
                                parts = line.split(":", 2)
                                if len(parts) >= 3:
                                    hits_str = parts[0].strip()
                                    line_num_str = parts[1].strip()
                                    if line_num_str.isdigit():
                                        line_num = int(line_num_str)
                                        if line_num == 0:
                                            continue
                                        if hits_str == "-":
                                            continue # non-executable line
                                        elif hits_str == "#####":
                                            if line_num not in line_coverage:
                                                line_coverage[line_num] = 0
                                        else:
                                            # Clean potential suffixes like '*'
                                            clean_hits = re.sub(r"[^\d]", "", hits_str)
                                            hits = int(clean_hits) if clean_hits else 0
                                            line_coverage[line_num] = line_coverage.get(line_num, 0) + hits
                    except Exception as e:
                        pass

            total_lines = len(line_coverage)
            covered_lines = sum(1 for hits in line_coverage.values() if hits > 0)
            cov_percent = (covered_lines / total_lines * 100.0) if total_lines > 0 else 0.0
            passed = cov_percent >= MIN_THRESHOLD_PERCENT
            if not passed:
                all_passed = False

            results.append((name, rel_source, covered_lines, total_lines, cov_percent, passed))
            overall_covered += covered_lines
            overall_total += total_lines

    print("================================================================================")
    print("                    QUICKCART C++ CODE COVERAGE REPORT                          ")
    print(f"                    Minimum Enforced Threshold: {MIN_THRESHOLD_PERCENT}%")
    print("================================================================================")
    print(f"{'Module / Class':<22} | {'Lines':<14} | {'Coverage':<10} | {'Status':<10} | {'Source'}")
    print("-----------------------+----------------+------------+------------+-------------")
    for name, rel_source, covered, total, pct, passed in results:
        status_str = "PASS" if passed else "FAIL"
        line_str = f"{covered}/{total}"
        pct_str = f"{pct:6.2f}%"
        print(f"{name:<22} | {line_str:<14} | {pct_str:<10} | {status_str:<10} | {rel_source}")
    print("-----------------------+----------------+------------+------------+-------------")
    overall_pct = (overall_covered / overall_total * 100.0) if overall_total > 0 else 0.0
    overall_status = "PASS" if all_passed and overall_pct >= MIN_THRESHOLD_PERCENT else "FAIL"
    print(f"{'CRITICAL OVERALL':<22} | {f'{overall_covered}/{overall_total}':<14} | {f'{overall_pct:6.2f}%':<10} | {overall_status:<10} | (Enforced Threshold)")
    print("================================================================================")

    if not all_passed:
        print(f"\nFAILED: One or more critical modules failed to meet the {MIN_THRESHOLD_PERCENT}% line coverage threshold.")
        return False
    else:
        print(f"\nSUCCESS: All critical modules met or exceeded the {MIN_THRESHOLD_PERCENT}% line coverage threshold!")
        return True

if __name__ == "__main__":
    build_directory = sys.argv[1] if len(sys.argv) > 1 else "build-cov"
    success = analyze_coverage(build_directory)
    sys.exit(0 if success else 1)
