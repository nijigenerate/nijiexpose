"""Check Git-derived versions and automatic refresh without compiling dependencies."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


SOURCE = Path(__file__).resolve().parents[1]


class GitVersionTests(unittest.TestCase):
    def setUp(self):
        fixtures = SOURCE / "build/version-tests"
        fixtures.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(dir=fixtures)
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name) / "source"
        self.repo.mkdir()
        self.build = Path(self.temporary.name) / "build"
        shutil.copy2(SOURCE / "cmake/Version.cmake", self.repo)
        (self.repo / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.24)
include(Version.cmake)
nijiexpose_git_version("${CMAKE_CURRENT_SOURCE_DIR}")
project(version_fixture VERSION ${NIJIEXPOSE_NUMERIC_VERSION} LANGUAGES NONE)
file(WRITE "${CMAKE_BINARY_DIR}/version.txt" "${NIJIEXPOSE_VERSION}\\n${PROJECT_VERSION}\\n${NIJIEXPOSE_PACKAGE_VERSION}\\n")
''')
        self.git("init", "-b", "main")
        self.git("config", "user.name", "Version test")
        self.git("config", "user.email", "version@example.invalid")
        self.git("config", "commit.gpgsign", "false")
        self.git("add", ".")
        self.git("commit", "-m", "Initial fixture")

    def run_command(self, *args):
        result = subprocess.run(args, cwd=self.repo, text=True, capture_output=True)
        if result.returncode:
            self.fail(f"{args}:\n{result.stdout}\n{result.stderr}")
        return result.stdout.strip()

    def git(self, *args):
        return self.run_command("git", *args)

    def configure(self):
        self.run_command("cmake", "-S", str(self.repo), "-B", str(self.build))

    def version(self, rebuild=False):
        if rebuild:
            self.run_command("cmake", "--build", str(self.build))
        else:
            self.configure()
        return (self.build / "version.txt").read_text().splitlines()

    def test_no_tag_and_dirty_files(self):
        revision = self.git("rev-parse", "--short=12", "HEAD")
        self.assertEqual(self.version(), [revision, "0.0.0", revision])
        (self.repo / "untracked.txt").write_text("Work in progress")
        self.assertEqual(self.version(rebuild=True)[0], revision)

    def test_exact_tag_then_commit_refresh(self):
        self.git("tag", "v2.3.4-rc1")
        self.assertEqual(self.version(), ["v2.3.4-rc1", "2.3.4", "v2.3.4-rc1"])
        self.git("commit", "--allow-empty", "-m", "Next commit")
        expected = "v2.3.4-rc1-g" + self.git("rev-parse", "--short=12", "HEAD")
        self.assertEqual(self.version(rebuild=True), [expected, "2.3.4", expected])

    def test_added_deleted_and_packed_tags_refresh(self):
        self.configure()
        self.git("tag", "v3.4.5")
        self.assertEqual(self.version(rebuild=True)[0], "v3.4.5")
        self.git("pack-refs", "--all", "--prune")
        self.assertEqual(self.version(rebuild=True)[0], "v3.4.5")
        self.git("tag", "-d", "v3.4.5")
        self.assertEqual(self.version(rebuild=True)[0], self.git("rev-parse", "--short=12", "HEAD"))

    def test_worktree_and_non_numeric_tag(self):
        self.git("tag", "release/special")
        worktree = Path(self.temporary.name) / "worktree"
        self.git("worktree", "add", "--detach", str(worktree), "HEAD")
        self.repo = worktree
        self.assertEqual(self.version(), ["release/special", "0.0.0", "release_special"])
        self.git("commit", "--allow-empty", "-m", "Worktree commit")
        expected = "release/special-g" + self.git("rev-parse", "--short=12", "HEAD")
        self.assertEqual(self.version(rebuild=True)[0], expected)


if __name__ == "__main__":
    unittest.main()
