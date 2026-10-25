"""Plan CI builds and publication without creating or moving release tags."""
import os
import re
import subprocess


RELEASE_TAG = re.compile(r"v?\d+\.\d+\.\d+(?:-(?:beta|rc)[\w.-]*)?")


def plan(event, ref, requested="", release_tag=""):
    tag = ""
    checkout_ref = ""
    publish = False
    if event == "workflow_dispatch":
        mode = requested or "verify"
        if mode not in ("nightly", "release", "verify"):
            raise ValueError("Unknown package mode")
        if release_tag and mode != "release":
            raise ValueError("release_tag is only used with release mode")
        if mode == "release":
            tag = release_tag or (ref.removeprefix("refs/tags/") if ref.startswith("refs/tags/") else "")
            if not RELEASE_TAG.fullmatch(tag):
                raise ValueError("Manual release requires an existing [v]MAJOR.MINOR.PATCH[-betaN|-rcN] tag")
            checkout_ref = f"refs/tags/{tag}"
        publish = mode != "verify"
    elif event == "schedule" or (event == "push" and ref == "refs/tags/nightly"):
        mode = "nightly"
        publish = True
    elif event == "push" and ref.startswith("refs/tags/"):
        tag = ref.removeprefix("refs/tags/")
        if not RELEASE_TAG.fullmatch(tag):
            raise ValueError("Release tags must be [v]MAJOR.MINOR.PATCH[-betaN|-rcN]")
        mode = "release"
        checkout_ref = ref
        publish = True
    else:
        mode = "verify"
    if mode == "nightly":
        tag = "nightly"
    return {
        "mode": mode,
        "checkout_ref": checkout_ref,
        "release_tag": tag,
        "publish": "true" if publish else "false",
        "prerelease": "true" if tag == "nightly" or re.search(r"-(beta|rc)", tag) else "false",
    }


def resolve_build_ref(checkout_ref, sha):
    # Pin all jobs to one commit, including when a manual run selects a tag
    # different from the workflow's branch. Missing release tags fail early.
    return subprocess.run(["git", "rev-parse", "--verify", f"{checkout_ref or sha}^{{commit}}"],
                          check=True, text=True, capture_output=True).stdout.strip()


if __name__ == "__main__":
    result = plan(os.environ["GITHUB_EVENT_NAME"], os.environ["GITHUB_REF"],
                  os.environ.get("REQUESTED_MODE", ""), os.environ.get("RELEASE_TAG", ""))
    result["build_ref"] = resolve_build_ref(result.pop("checkout_ref"), os.environ["GITHUB_SHA"])
    for key, value in result.items():
        print(f"{key}={value}")
