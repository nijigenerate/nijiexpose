"""Select CI artifacts without creating or moving release tags."""
import os
import re


def select(event, ref, requested=""):
    if event == "workflow_dispatch":
        mode = requested or "verify"
        if mode not in ("nightly", "release", "verify"):
            raise ValueError("Unknown package mode")
        return mode
    if event == "schedule" or ref == "refs/tags/nightly":
        return "nightly"
    if ref.startswith("refs/tags/"):
        match = re.fullmatch(r"v?(\d+\.\d+\.\d+)(?:-rc[\w.-]*)?", ref.removeprefix("refs/tags/"))
        if not match:
            raise ValueError("Release tags must be [v]MAJOR.MINOR.PATCH[-rcN]")
        return "release"
    mode = requested or "verify"
    if mode not in ("nightly", "release", "verify"):
        raise ValueError("Unknown package mode")
    return mode


if __name__ == "__main__":
    mode = select(os.environ["GITHUB_EVENT_NAME"], os.environ["GITHUB_REF"],
                           os.environ.get("REQUESTED_MODE", ""))
    print(f"mode={mode}")
