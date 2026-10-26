#pragma once

#include <string>
#include <array>
#include <cstdint> // for fixed type: uint64_t

// 4 vectors (a, b, c, d) of 3 floats representing the cosine palette:
// color(t) = a + b * cos(2 * PI * (c * t + d))
struct PaletteDNA {
    std::array<float, 3> a{};
    std::array<float, 3> b{};
    std::array<float, 3> c{};
    std::array<float, 3> d{};
};

// Morphological parameters that define the 3D structure of this song's world
struct GeometryDNA {
    float core_shape_type    = 0.0f; // 0.0 = Pure Sphere, 1.0 = Rounded Box, 2.0 = Octahedron
    float satellite_count    = 1.0f; // 1, 2, or 3 orbiting elements
    float twist_strength     = 0.0f; // Geometric space torsion
    float roughness_factor   = 1.0f; // Surface micro-texture scale
};

struct SongUniverse {
    uint64_t    seed = 0;
    std::string track_name;
    PaletteDNA  palette;
    GeometryDNA geometry;
};

class SongDNABuilder {
public:
    // Build a deterministic universe from an audio filepath
    static SongUniverse extract_dna(const std::string& filepath);

private:
    // 64-bit FNV-1a hash algorithm
    static uint64_t compute_file_hash(const std::string& filepath);
};