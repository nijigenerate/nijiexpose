#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <mediapipe/tasks/c/vision/face_landmarker/face_landmarker.h>
#include <stdexcept>

namespace niji {
using Vec3 = std::array<double, 3>;
using Quat = std::array<double, 4>; // x,y,z,w (SciPy / VMC order)
inline Vec3 sub(Vec3 a, Vec3 b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}
inline Vec3 cross(Vec3 a, Vec3 b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
inline Vec3 unit(Vec3 a) {
  double length = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
  if (length < 1e-9)
    throw std::runtime_error("Degenerate head-pose basis");
  for (auto &v : a)
    v /= length;
  return a;
}
inline Vec3 point(const MpNormalizedLandmarks &l, unsigned i) {
  const auto &p = l.landmarks[i];
  return {p.x, p.y, p.z};
}
struct Pose {
  Vec3 nose{};
  Quat quaternion{0, 0, 0, 1};
  Vec3 euler{}; // yaw, pitch, roll; legacy Python display formulas
  std::array<double, 2> gaze_right{}, gaze_left{};
};
inline std::array<double, 2> gaze(const MpNormalizedLandmarks &l, unsigned center, unsigned outer,
                                  unsigned inner, unsigned top, unsigned bottom) {
  const auto &c = l.landmarks[center];
  const auto &o = l.landmarks[outer];
  const auto &i = l.landmarks[inner];
  const auto &t = l.landmarks[top];
  const auto &b = l.landmarks[bottom];
  double dx = double(i.x) - o.x, dy = (double(b.y) - t.y) / 2;
  return {std::clamp(std::abs(dx) > 1e-6 ? (c.x - o.x) / dx * 2 - 1 : 0.0, -1.0, 1.0),
          std::clamp(std::abs(dy) > 1e-6 ? (c.y - (double(t.y) + b.y) / 2) / dy : 0.0, -1.0, 1.0)};
}
class PoseTracker {
  bool previous_ = false;
  Quat last_q_{};
  Vec3 last_angles_{};

public:
  void reset() {
    previous_ = false;
  }
  Pose calculate(const MpNormalizedLandmarks &l) {
    if (l.landmarks_count < 478)
      throw std::runtime_error("Expected iris-enabled 478-point face model");
    Pose p;
    p.nose = point(l, 1);
    auto x = unit(sub(point(l, 263), point(l, 33)));
    auto z = unit(cross(x, sub(point(l, 152), point(l, 33))));
    auto y = unit(cross(z, x));
    x = unit(cross(y, z));
    const double m[3][3] = {{x[0], y[0], z[0]}, {x[1], y[1], z[1]}, {x[2], y[2], z[2]}};
    auto &q = p.quaternion;
    double s;
    if (m[0][0] + m[1][1] + m[2][2] > 0) {
      s = std::sqrt(1 + m[0][0] + m[1][1] + m[2][2]) * 2;
      q = {(m[2][1] - m[1][2]) / s, (m[0][2] - m[2][0]) / s, (m[1][0] - m[0][1]) / s, s / 4};
    } else if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
      s = std::sqrt(1 + m[0][0] - m[1][1] - m[2][2]) * 2;
      q = {s / 4, (m[0][1] + m[1][0]) / s, (m[0][2] + m[2][0]) / s, (m[2][1] - m[1][2]) / s};
    } else if (m[1][1] > m[2][2]) {
      s = std::sqrt(1 + m[1][1] - m[0][0] - m[2][2]) * 2;
      q = {(m[0][1] + m[1][0]) / s, s / 4, (m[1][2] + m[2][1]) / s, (m[0][2] - m[2][0]) / s};
    } else {
      s = std::sqrt(1 + m[2][2] - m[0][0] - m[1][1]) * 2;
      q = {(m[0][2] + m[2][0]) / s, (m[1][2] + m[2][1]) / s, s / 4, (m[1][0] - m[0][1]) / s};
    }
    double n = 0;
    for (auto v : q)
      n += v * v;
    for (auto &v : q)
      v /= std::sqrt(n);
    if (previous_) {
      double dot = 0;
      for (unsigned j = 0; j < 4; j++)
        dot += q[j] * last_q_[j];
      if (dot < 0)
        for (auto &v : q)
          v = -v;
    }
    constexpr double deg = 180.0 / 3.14159265358979323846;
    p.euler = {std::atan2(m[0][2], m[2][2]) * deg, std::asin(std::clamp(-m[1][2], -1.0, 1.0)) * deg,
               std::atan2(m[1][0], m[1][1]) * deg};
    if (previous_)
      for (unsigned j = 0; j < 3; j++)
        p.euler[j] += 360 * std::round((last_angles_[j] - p.euler[j]) / 360);
    p.gaze_right = gaze(l, 468, 33, 133, 159, 145);
    p.gaze_left = gaze(l, 473, 362, 263, 386, 374);
    last_q_ = q;
    last_angles_ = p.euler;
    previous_ = true;
    return p;
  }
};
} // namespace niji
