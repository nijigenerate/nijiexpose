# Bundled MediaPipe models

These unmodified Google MediaPipe model bundles are licensed under Apache-2.0.
See LICENSE.apache-2.0. Versions, download URLs and SHA256 digests are recorded
in models.lock.json; builds verify the checked-in files without downloading.
The face bundle contains the face detector, FaceMesh V2 and blendshape models.
The hand bundle contains the palm detector and hand landmark model.

Official model cards and license declarations:

- Face detector: https://storage.googleapis.com/mediapipe-assets/MediaPipe%20BlazeFace%20Model%20Card%20%28Short%20Range%29.pdf
- FaceMesh V2: https://storage.googleapis.com/mediapipe-assets/Model%20Card%20MediaPipe%20Face%20Mesh%20V2.pdf
- Blendshape V2: https://storage.googleapis.com/mediapipe-assets/Model%20Card%20Blendshape%20V2.pdf
- Hand tracking: https://storage.googleapis.com/mediapipe-assets/Model%20Card%20Hand%20Tracking%20%28Lite_Full%29%20with%20Fairness%20Oct%202021.pdf

The tracker currently uses the face bundle. The hand bundle is shipped for the
static SDK's hand tracking support and is exercised by the SDK integration test.
