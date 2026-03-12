#!/usr/bin/env python3
"""Prepare a PlotJuggler release: bump versions, update changelog, commit, tag, push."""

import argparse
import re
import subprocess
import sys
from datetime import date
from pathlib import Path

REPO = Path(__file__).resolve().parent

VERSION_FILES = {
    "cmake": REPO / "CMakeLists.txt",
    "package": REPO / "package.xml",
    "installer_config": REPO / "installer" / "config.xml",
    "installer_pkg": REPO / "installer" / "io.plotjuggler.application" / "meta" / "package.xml",
    "changelog": REPO / "CHANGELOG.rst",
}


def run(cmd, **kwargs):
    return subprocess.run(cmd, cwd=REPO, check=True, capture_output=True, text=True, **kwargs)


def read_current_version():
    text = VERSION_FILES["cmake"].read_text()
    m = re.search(r"VERSION\s+(\d+\.\d+\.\d+)\)", text)
    if not m:
        sys.exit("Error: cannot parse VERSION from CMakeLists.txt")
    return m.group(1)


def bump_patch(version):
    major, minor, patch = version.split(".")
    return f"{major}.{minor}.{int(patch) + 1}"


def check_preconditions(new_version):
    # Check for uncommitted changes (besides the files we'll touch)
    result = run(["git", "status", "--porcelain"])
    dirty = [
        line for line in result.stdout.splitlines()
        if line.strip() and not any(str(f.relative_to(REPO)) in line for f in VERSION_FILES.values())
    ]
    if dirty:
        print("Warning: working tree has uncommitted changes:")
        for line in dirty:
            print(f"  {line}")
        answer = input("Continue anyway? [y/N] ")
        if answer.lower() != "y":
            sys.exit("Aborted.")

    # Check tag doesn't already exist
    result = run(["git", "tag", "-l", new_version])
    if result.stdout.strip():
        sys.exit(f"Error: tag '{new_version}' already exists")

    # Warn if no Forthcoming section (changelog will be skipped)
    text = VERSION_FILES["changelog"].read_text()
    if "Forthcoming" not in text:
        print("Warning: no 'Forthcoming' section in CHANGELOG.rst — changelog will not be updated")


def update_cmake(old, new):
    path = VERSION_FILES["cmake"]
    text = path.read_text()
    path.write_text(text.replace(f"VERSION {old})", f"VERSION {new})"))
    return f"VERSION {old} -> {new}"


def update_package_xml(new):
    path = VERSION_FILES["package"]
    text = path.read_text()
    m = re.search(r"<version>([^<]*)</version>", text)
    old_ver = m.group(1) if m else "?"
    path.write_text(re.sub(r"<version>[^<]*</version>", f"<version>{new}</version>", text))
    return f"{old_ver} -> {new}"


def update_installer_config(old, new):
    path = VERSION_FILES["installer_config"]
    text = path.read_text()
    path.write_text(text.replace(f"<Version>{old}</Version>", f"<Version>{new}</Version>"))
    return f"{old} -> {new}"


def update_installer_package(old, new, today):
    path = VERSION_FILES["installer_pkg"]
    text = path.read_text()
    updated = text.replace(f"<Version>{old}</Version>", f"<Version>{new}</Version>")
    updated = re.sub(r"<ReleaseDate>[^<]*</ReleaseDate>", f"<ReleaseDate>{today}</ReleaseDate>", updated)
    path.write_text(updated)
    return f"{old} -> {new}, date -> {today}"


def update_changelog(new, today):
    path = VERSION_FILES["changelog"]
    text = path.read_text()
    if "Forthcoming" not in text:
        return None
    title = f"{new} ({today})"
    underline = "-" * len(title)
    path.write_text(re.sub(r"Forthcoming\n-+", f"{title}\n{underline}", text))
    return f"Forthcoming -> {title}"


def git_commit_tag_push(new_version, push):
    files = [str(f.relative_to(REPO)) for f in VERSION_FILES.values()]
    run(["git", "commit", "-o", "-m", new_version, "--"] + files)
    run(["git", "tag", new_version])
    pushed = False
    if push:
        run(["git", "push"])
        run(["git", "push", "--tags"])
        pushed = True
    return pushed


def print_summary(old, new, file_results, committed, tag, pushed):
    print()
    print(f"  Release {old} -> {new}")
    print()
    print("  Files updated:")
    for fname, detail in file_results:
        print(f"    {fname}: {detail}")
    print()
    if committed:
        print(f"  Commit:  created ({new})")
    if tag:
        print(f"  Tag:     created ({tag})")
    if pushed:
        print(f"  Push:    done")
    elif committed:
        print(f"  Push:    skipped (git push && git push --tags)")
    print()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version", nargs="?", help="New version (default: bump patch)")
    parser.add_argument("--dry-run", action="store_true", help="Show what would change")
    parser.add_argument("--no-push", action="store_true", help="Commit and tag but don't push")
    args = parser.parse_args()

    old = read_current_version()
    new = args.version or bump_patch(old)
    today = date.today().strftime("%Y-%m-%d")

    # Validate version format
    if not re.fullmatch(r"\d+\.\d+\.\d+", new):
        sys.exit(f"Error: invalid version format '{new}' (expected X.Y.Z)")

    print(f"Preparing release: {old} -> {new}")

    if args.dry_run:
        print("Dry run — no files will be modified:")
        print(f"  CMakeLists.txt: VERSION {old}) -> VERSION {new})")
        m = re.search(r"<version>([^<]*)</version>", VERSION_FILES["package"].read_text())
        print(f"  package.xml: {m.group(1) if m else '?'} -> {new}")
        print(f"  installer/config.xml: {old} -> {new}")
        print(f"  installer/.../package.xml: {old} -> {new}, date -> {today}")
        text = VERSION_FILES["changelog"].read_text()
        if "Forthcoming" in text:
            print(f"  CHANGELOG.rst: Forthcoming -> {new} ({today})")
        else:
            print(f"  CHANGELOG.rst: no Forthcoming section — skipped")
        return

    check_preconditions(new)

    file_results = []
    file_results.append(("CMakeLists.txt", update_cmake(old, new)))
    file_results.append(("package.xml", update_package_xml(new)))
    file_results.append(("installer/config.xml", update_installer_config(old, new)))
    file_results.append(("installer/.../package.xml", update_installer_package(old, new, today)))
    cl_result = update_changelog(new, today)
    if cl_result:
        file_results.append(("CHANGELOG.rst", cl_result))
    else:
        file_results.append(("CHANGELOG.rst", "skipped (no Forthcoming section)"))

    pushed = git_commit_tag_push(new, push=not args.no_push)
    print_summary(old, new, file_results, committed=True, tag=new, pushed=pushed)


if __name__ == "__main__":
    main()
