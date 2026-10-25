"""Regression checks for publication plans and the workflow's packaging commands."""
import importlib.util
import json
import os
import shutil
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SOURCE = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("package_mode", SOURCE / "build-aux/package-mode.py")
mode = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mode)
WORKFLOW = (SOURCE / ".github/workflows/native-cpp.yml").read_text()


def shell_step(name):
    section = WORKFLOW.split(f"      - name: {name}\n", 1)[1]
    script = section.split("        run: |\n", 1)[1]
    lines = []
    for line in script.splitlines():
        if line and not line.startswith("          "):
            break
        lines.append(line[10:])
    return "\n".join(lines)


class PackageModeTests(unittest.TestCase):
    def test_release_tags(self):
        for tag in ("1.2.3", "v1.2.3", "v1.2.3-beta3", "v1.2.3-beta.1", "v1.2.3-rc1"):
            with self.subTest(tag=tag):
                result = mode.plan("push", f"refs/tags/{tag}")
                self.assertEqual(result["mode"], "release")
                self.assertEqual(result["release_tag"], tag)
                self.assertEqual(result["publish"], "true")
                self.assertEqual(result["prerelease"], "true" if "-" in tag else "false")

    def test_nightly(self):
        for event, ref in (("push", "refs/tags/nightly"), ("schedule", "refs/heads/main"),
                           ("workflow_dispatch", "refs/heads/native-cpp")):
            result = mode.plan(event, ref, "nightly")
            self.assertEqual(result["mode"], "nightly")
            self.assertEqual(result["release_tag"], "nightly")
            self.assertEqual(result["publish"], "true")
            self.assertEqual(result["prerelease"], "true")

    def test_manual_tag_obeys_requested_mode(self):
        for requested in ("verify", "nightly", "release"):
            result = mode.plan("workflow_dispatch", "refs/tags/v1.2.3", requested)
            self.assertEqual(result["mode"], requested)
            self.assertEqual(result["publish"], "false" if requested == "verify" else "true")
        result = mode.plan("workflow_dispatch", "refs/tags/nightly", "verify")
        self.assertEqual(result["publish"], "false")

    def test_manual_release_selects_existing_tag(self):
        result = mode.plan("workflow_dispatch", "refs/heads/main", "release", "v1.2.3-beta4")
        self.assertEqual(result["checkout_ref"], "refs/tags/v1.2.3-beta4")
        self.assertEqual(result["release_tag"], "v1.2.3-beta4")
        self.assertEqual(result["publish"], "true")
        self.assertEqual(result["prerelease"], "true")

    def test_manual_release_requires_tag(self):
        for ref in ("refs/heads/main", "refs/tags/nightly"):
            with self.assertRaises(ValueError):
                mode.plan("workflow_dispatch", ref, "release")
        for requested in ("verify", "nightly"):
            with self.assertRaises(ValueError):
                mode.plan("workflow_dispatch", "refs/heads/main", requested, "v1.2.3")

    def test_branch_and_pr_do_not_publish(self):
        for event in ("push", "pull_request"):
            result = mode.plan(event, "refs/heads/native-cpp")
            self.assertEqual(result["mode"], "verify")
            self.assertEqual(result["publish"], "false")

    def test_distribution_steps_require_publication(self):
        # Branch/PR and manual verify must not create, verify or upload packages.
        gated = ["Package Windows", "Verify unpacked Windows package",
                 "Select Windows distribution files", "Upload Windows packages",
                 "Package Unix", "Verify unpacked package",
                 "Select Unix distribution files", "Upload packages"]
        for name in gated:
            self.assertIn(f"- name: {name}\n        if: needs.package-mode.outputs.publish == 'true'\n", WORKFLOW)
        for name in ["Locate binary WiX tools", "Verify MSI payload"]:
            self.assertIn(f"- name: {name}\n        if: env.PACKAGE_MODE == 'release'\n", WORKFLOW)
        for event in ("push", "pull_request", "workflow_dispatch"):
            result = mode.plan(event, "refs/heads/main", "verify" if event == "workflow_dispatch" else "")
            self.assertEqual(result["publish"], "false")
        self.assertIn('run: ctest --preset native-release', WORKFLOW)

    def test_invalid_tag(self):
        for tag in ("v1.2.3-invalid", "bad\noutput=true", "v1.2.3;echo"):
            with self.assertRaises(ValueError):
                mode.plan("push", f"refs/tags/{tag}")

    def test_workflow_wiring(self):
        self.assertEqual(WORKFLOW.count("ref: ${{ needs.package-mode.outputs.build_ref }}"), 3)
        self.assertIn("if: needs.package-mode.outputs.publish == 'true'", WORKFLOW)
        self.assertIn("group: publish-${{ needs.package-mode.outputs.release_tag }}", WORKFLOW)
        self.assertIn("cancel-in-progress: ${{ github.event_name == 'pull_request' || (github.event_name == 'push' && !startsWith(github.ref, 'refs/tags/')) }}", WORKFLOW)
        self.assertIn("- name: Verify mounted macOS DMG\n        if: runner.os == 'macOS' && env.PACKAGE_MODE == 'release'\n", WORKFLOW)
        self.assertIn("needs: [package-mode, windows-native, unix-native]", WORKFLOW)


class WorkflowCommandTests(unittest.TestCase):
    def setUp(self):
        fixtures = SOURCE / "build/release-tests"
        fixtures.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(dir=fixtures)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.env = dict(os.environ, PATH=str(self.bin) + os.pathsep + os.environ["PATH"])

    def tool(self, name, body):
        path = self.bin / name
        path.write_text(f"#!{sys.executable}\n" + body)
        path.chmod(0o755)

    def run_command(self, *args, env=None):
        result = subprocess.run(args, cwd=self.root, env=env or self.env, text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout

    def test_unix_artifacts_for_all_modes(self):
        self.tool("cpack", '''import sys
from pathlib import Path
p=Path("build/native-release/packages");p.mkdir(parents=True,exist_ok=True)
generator=sys.argv[sys.argv.index("-G")+1]
(p/("fixture.zip" if generator=="ZIP" else "fixture.dmg")).touch()
''')
        self.tool("python3", '''from pathlib import Path
Path("build/native-release/packages/fixture.AppImage").touch()
''')
        for system in ("macOS", "Linux"):
            for selected in ("nightly", "release"):
                with self.subTest(system=system, mode=selected):
                    shutil.rmtree(self.root / "build", ignore_errors=True)
                    shutil.rmtree(self.root / "out", ignore_errors=True)
                    env = dict(self.env, RUNNER_OS=system, PACKAGE_MODE=selected)
                    self.run_command("bash", "-euo", "pipefail", "-c", shell_step("Package Unix"), env=env)
                    self.run_command("bash", "-euo", "pipefail", "-c", shell_step("Select Unix distribution files"), env=env)
                    extensions = {p.suffix for p in (self.root / "out/dist").iterdir()}
                    expected = {".zip"} if selected != "release" else set()
                    if system == "macOS" and selected == "release":
                        expected.add(".dmg")
                    elif system == "Linux" and selected == "release":
                        expected.add(".AppImage")
                    self.assertEqual(extensions, expected)

    def test_publish_commands(self):
        self.tool("gh", '''import json, os, sys
from pathlib import Path
with Path("gh-calls.jsonl").open("a") as f: f.write(json.dumps(sys.argv[1:])+"\\n")
if sys.argv[1:3]==["release","view"]: sys.exit(0 if os.environ["EXISTS"]=="true" else 1)
''')
        (self.root / "dist").mkdir()
        (self.root / "dist/app.dmg").touch()
        for tag, prerelease in (("nightly", "true"), ("v1.2.3-beta3", "true"),
                                ("v1.2.3-rc1", "true"), ("v1.2.3", "false")):
            for exists in ("true", "false"):
                with self.subTest(tag=tag, exists=exists):
                    calls_path = self.root / "gh-calls.jsonl"
                    calls_path.unlink(missing_ok=True)
                    env = dict(self.env, RELEASE_TAG=tag, RELEASE_PRERELEASE=prerelease,
                               BUILD_REF="a" * 40, GITHUB_SHA="b" * 40, EXISTS=exists)
                    self.run_command("bash", "-euo", "pipefail", "-c", shell_step("Publish verified packages"), env=env)
                    calls = [json.loads(line) for line in calls_path.read_text().splitlines()]
                    self.assertEqual(calls[-1], ["release", "upload", tag, "dist/app.dmg", "--clobber"])
                    if exists == "false":
                        create = calls[1]
                        self.assertEqual(create[create.index("--target") + 1], "a" * 40)
                        self.assertIn(f"--prerelease={prerelease}", create)
                    else:
                        self.assertEqual(calls[1], ["release", "edit", tag, f"--prerelease={prerelease}"])

    def test_manual_release_pins_tag_commit_and_rejects_missing_tag(self):
        self.run_command("git", "init", "-b", "main")
        self.run_command("git", "config", "user.name", "Release test")
        self.run_command("git", "config", "user.email", "release@example.invalid")
        self.run_command("git", "config", "commit.gpgsign", "false")
        self.run_command("git", "commit", "--allow-empty", "-m", "Tagged source")
        tagged = self.run_command("git", "rev-parse", "HEAD").strip()
        self.run_command("git", "tag", "v1.2.3-beta1")
        self.run_command("git", "commit", "--allow-empty", "-m", "Workflow branch")
        head = self.run_command("git", "rev-parse", "HEAD").strip()
        env = dict(self.env, GITHUB_EVENT_NAME="workflow_dispatch", GITHUB_REF="refs/heads/main",
                   REQUESTED_MODE="release", RELEASE_TAG="v1.2.3-beta1", GITHUB_SHA=head)
        output = self.run_command(sys.executable, str(SOURCE / "build-aux/package-mode.py"), env=env)
        self.assertIn(f"build_ref={tagged}\n", output)
        self.assertNotEqual(tagged, head)
        env["RELEASE_TAG"] = "v9.9.9"
        failed = subprocess.run([sys.executable, str(SOURCE / "build-aux/package-mode.py")],
                                cwd=self.root, env=env, text=True, capture_output=True)
        self.assertNotEqual(failed.returncode, 0)
        self.assertEqual(failed.stdout, "")


if __name__ == "__main__":
    unittest.main()
