#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

struct FVector {
    float X = 0, Y = 0, Z = 0;
    FVector() = default;
    FVector(float x, float y, float z) : X(x), Y(y), Z(z) {}
    FVector operator+(const FVector& o) const { return { X + o.X, Y + o.Y, Z + o.Z }; }
    FVector operator-(const FVector& o) const { return { X - o.X, Y - o.Y, Z - o.Z }; }
    FVector operator*(float s) const { return { X * s, Y * s, Z * s }; }
    float Length() const { return std::sqrt(X * X + Y * Y + Z * Z); }
    float DistTo(const FVector& o) const { return (*this - o).Length(); }
};

struct FVector2D {
    float X = 0, Y = 0;
};

struct FRotator {
    float Pitch = 0, Yaw = 0, Roll = 0;
};

struct FMatrix {
    float M[4][4]{};
};

struct FQuat {
    float X = 0, Y = 0, Z = 0, W = 1;
};

// UE FTransform is often 0x60 bytes (aligned). We only need rotation + translation for bone read.
struct FTransform {
    FQuat   Rotation{};
    FVector Translation{};
    float   TranslationPad = 0;
    FVector Scale3D{ 1, 1, 1 };
    float   ScalePad = 0;
};

inline FMatrix QuaternionToMatrix(const FQuat& q) {
    FMatrix m{};
    float x2 = q.X + q.X, y2 = q.Y + q.Y, z2 = q.Z + q.Z;
    float xx = q.X * x2, yy = q.Y * y2, zz = q.Z * z2;
    float xy = q.X * y2, xz = q.X * z2, yz = q.Y * z2;
    float wx = q.W * x2, wy = q.W * y2, wz = q.W * z2;
    m.M[0][0] = 1.f - (yy + zz); m.M[0][1] = xy + wz;         m.M[0][2] = xz - wy;
    m.M[1][0] = xy - wz;         m.M[1][1] = 1.f - (xx + zz); m.M[1][2] = yz + wx;
    m.M[2][0] = xz + wy;         m.M[2][1] = yz - wx;         m.M[2][2] = 1.f - (xx + yy);
    m.M[3][0] = 0; m.M[3][1] = 0; m.M[3][2] = 0; m.M[3][3] = 1;
    return m;
}

inline FVector TransformPoint(const FTransform& t, const FVector& p) {
    FVector s{ p.X * t.Scale3D.X, p.Y * t.Scale3D.Y, p.Z * t.Scale3D.Z };
    FMatrix r = QuaternionToMatrix(t.Rotation);
    FVector o;
    o.X = s.X * r.M[0][0] + s.Y * r.M[1][0] + s.Z * r.M[2][0] + t.Translation.X;
    o.Y = s.X * r.M[0][1] + s.Y * r.M[1][1] + s.Z * r.M[2][1] + t.Translation.Y;
    o.Z = s.X * r.M[0][2] + s.Y * r.M[1][2] + s.Z * r.M[2][2] + t.Translation.Z;
    return o;
}

// Build view-projection from camera location + UE rotator (degrees) + FOV
inline FMatrix BuildViewProjection(const FVector& loc, const FRotator& rot, float fovDeg, float aspect) {
    const float deg2rad = 0.01745329251f;
    float pitch = rot.Pitch * deg2rad;
    float yaw   = rot.Yaw * deg2rad;
    float cp = std::cos(pitch), sp = std::sin(pitch);
    float cy = std::cos(yaw),   sy = std::sin(yaw);

    // UE forward / right / up
    FVector forward{ cp * cy, cp * sy, sp };
    FVector right{ -sy, cy, 0.f };
    FVector up{
        -sp * cy,
        -sp * sy,
        cp
    };

    // View matrix (camera)
    FMatrix v{};
    v.M[0][0] = right.X;   v.M[0][1] = up.X;   v.M[0][2] = forward.X; v.M[0][3] = 0;
    v.M[1][0] = right.Y;   v.M[1][1] = up.Y;   v.M[1][2] = forward.Y; v.M[1][3] = 0;
    v.M[2][0] = right.Z;   v.M[2][1] = up.Z;   v.M[2][2] = forward.Z; v.M[2][3] = 0;
    v.M[3][0] = -(right.X * loc.X + right.Y * loc.Y + right.Z * loc.Z);
    v.M[3][1] = -(up.X * loc.X + up.Y * loc.Y + up.Z * loc.Z);
    v.M[3][2] = -(forward.X * loc.X + forward.Y * loc.Y + forward.Z * loc.Z);
    v.M[3][3] = 1.f;

    float fovRad = (std::max)(1.f, fovDeg) * deg2rad;
    float f = 1.f / std::tan(fovRad * 0.5f);
    float zn = 0.1f, zf = 100000.f;
    FMatrix p{};
    p.M[0][0] = f / aspect;
    p.M[1][1] = f;
    p.M[2][2] = zf / (zf - zn);
    p.M[2][3] = 1.f;
    p.M[3][2] = -zn * zf / (zf - zn);

    // vp = view * proj (row-major multiply)
    FMatrix out{};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            out.M[i][j] =
                v.M[i][0] * p.M[0][j] + v.M[i][1] * p.M[1][j] +
                v.M[i][2] * p.M[2][j] + v.M[i][3] * p.M[3][j];
        }
    }
    return out;
}

inline bool WorldToScreen(const FVector& world, const FMatrix& vp, float screenW, float screenH, FVector2D& out) {
    float w = world.X * vp.M[0][3] + world.Y * vp.M[1][3] + world.Z * vp.M[2][3] + vp.M[3][3];
    if (w < 0.001f) return false;
    float inv = 1.f / w;
    float x = (world.X * vp.M[0][0] + world.Y * vp.M[1][0] + world.Z * vp.M[2][0] + vp.M[3][0]) * inv;
    float y = (world.X * vp.M[0][1] + world.Y * vp.M[1][1] + world.Z * vp.M[2][1] + vp.M[3][1]) * inv;
    out.X = (screenW * 0.5f) + (x * screenW * 0.5f);
    out.Y = (screenH * 0.5f) - (y * screenH * 0.5f);
    return out.X >= -200 && out.X <= screenW + 200 && out.Y >= -200 && out.Y <= screenH + 200;
}
