#!/usr/bin/env python3
"""check_pin.py against real clones, including every case it must refuse.

    python scripts/test_check_pin.py

The gate refuses a build, so the cases that matter are the refusals. Each one is built here as an
actual git repository in a temporary directory rather than mocked, because the thing under test is
what `git rev-parse` and `git describe` answer, and a mock would be a second opinion about that
rather than the answer.

WHY THIS SUITE EXISTS AT ALL. The gate it tests was written because a pin that lived in a comment
let a build use any revision. A gate for that which itself only ever passes would be the same
defect one level up, and this repository has spent a weekend finding guards that reported passes
while measuring nothing.
"""
from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GATE = ROOT / "scripts" / "check_pin.py"

failures = 0
checks = 0


def run_git(path: Path, *args: str) -> str:
    env = dict(os.environ)
    env.update({
        "GIT_AUTHOR_NAME": "test", "GIT_AUTHOR_EMAIL": "test@example.invalid",
        "GIT_COMMITTER_NAME": "test", "GIT_COMMITTER_EMAIL": "test@example.invalid",
    })
    out = subprocess.run(("git", "-C", str(path)) + args, capture_output=True, text=True,
                         check=True, env=env)
    return out.stdout.strip()


def gate(*args: str) -> tuple[int, str]:
    out = subprocess.run([sys.executable, str(GATE)] + list(args),
                         capture_output=True, text=True)
    return out.returncode, (out.stdout + out.stderr)


def expect(condition: bool, what: str, detail: str = "") -> None:
    global failures, checks
    checks += 1
    if not condition:
        failures += 1
        print("  FAILED: %s" % what)
        if detail:
            for line in detail.strip().splitlines()[:6]:
                print("          " + line)


def make_repo(root: Path) -> tuple[Path, str, str]:
    """A clone with two commits and a tag on the first. Returns the path and both commits."""
    repo = root / "clone"
    repo.mkdir()
    run_git(repo, "init", "--quiet")
    (repo / "a.txt").write_text("one\n", encoding="utf-8")
    run_git(repo, "add", "a.txt")
    run_git(repo, "commit", "--quiet", "-m", "first")
    first = run_git(repo, "rev-parse", "HEAD")
    run_git(repo, "tag", "V11.1.0")
    (repo / "a.txt").write_text("two\n", encoding="utf-8")
    run_git(repo, "commit", "--quiet", "-am", "second")
    second = run_git(repo, "rev-parse", "HEAD")
    return repo, first, second


def main() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        repo, first, second = make_repo(root)

        print("a clone sitting exactly on the pinned commit is accepted")
        run_git(repo, "checkout", "--quiet", first)
        rc, out = gate("--path", str(repo), "--commit", first, "--what", "the kernel")
        expect(rc == 0, "an exact commit match must be accepted", out)
        expect("is at the pinned" in out, "and must say so", out)

        print("the same clone, with the pin as a tag, is accepted")
        rc, out = gate("--path", str(repo), "--tag", "V11.1.0", "--what", "the kernel")
        expect(rc == 0, "a tag resolving to HEAD must be accepted", out)

        print("a clone moved off the pin is REFUSED")
        run_git(repo, "checkout", "--quiet", second)
        rc, out = gate("--path", str(repo), "--commit", first, "--what", "the kernel")
        expect(rc == 1, "a different commit must be refused", out)
        expect("not at the revision" in out, "and must say what is wrong", out)
        expect("checkout" in out, "and must name the command that fixes it", out)

        print("the same, by tag")
        rc, out = gate("--path", str(repo), "--tag", "V11.1.0", "--what", "the kernel")
        expect(rc == 1, "a clone past the tag must be refused", out)

        print("--allow-any builds anyway, and says the build is unpinned")
        rc, out = gate("--path", str(repo), "--commit", first, "--allow-any", "--what", "k")
        expect(rc == 0, "the escape hatch must allow the build", out)
        expect("WARNING" in out, "and must warn", out)
        # The needle stops at the line break the message wraps on. An assertion that spans a
        # wrap tests the formatting rather than the warning, and this one failed for that
        # reason on its first run while the gate was behaving correctly.
        expect("quoted beside a" in out, "and must say what the build may not be used for", out)

        print("a directory that is not a clone at all is REFUSED")
        plain = root / "copied"
        plain.mkdir()
        (plain / "a.txt").write_text("one\n", encoding="utf-8")
        rc, out = gate("--path", str(plain), "--commit", first, "--what", "the kernel")
        expect(rc == 1, "a copy is not a clone and must be refused", out)
        expect("not a git clone" in out, "and must say that is why", out)

        print("a path that does not exist is REFUSED, with the clone command")
        rc, out = gate("--path", str(root / "absent"), "--tag", "V11.1.0",
                       "--url", "https://example.invalid/k.git", "--what", "the kernel")
        expect(rc == 1, "an absent clone must be refused", out)
        expect("git clone" in out, "and must name the clone command", out)

        print("an abbreviated commit is REFUSED as a pin, before any clone is read")
        rc, out = gate("--path", str(repo), "--commit", first[:7], "--what", "the kernel")
        expect(rc == 1, "a short hash must be refused as a pin", out)
        expect("forty hexadecimal" in out, "and must say why", out)

        print("asking for both a tag and a commit, or neither, is REFUSED")
        rc, _ = gate("--path", str(repo), "--tag", "V11.1.0", "--commit", first)
        expect(rc == 1, "both at once must be refused")
        rc, _ = gate("--path", str(repo))
        expect(rc == 1, "neither must be refused")

    print("\n%d check(s), %d failure(s)" % (checks, failures))
    if failures == 0:
        print("every case behaved")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
