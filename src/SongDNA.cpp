#include "SongDNA.hpp"
#include <fstream>
#include <random>
#include <filesystem>
#include <iostream>

uint64_t SongDNABuilder::compute_file_hash(const std::string& filepath) {
    // 64-bit FNV-1a offset basis and prime
    constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME = 1099511628211ULL;

    uint64_t hash = FNV_OFFSET_BASIS;

    // Hash the filename itself => it means changing the name file changes the hash value
    for (char c : filepath) {
        hash ^= static_cast<uint64_t>(c);
        hash *= FNV_PRIME;
    }

    // Read the first 64 KB of the file to capture audio header & first frames
    std::ifstream file(filepath, std::ios::binary);
    if (file.is_open()) {
        std::vector<char> buffer(65536); // 64KB
        file.read(buffer.data(), buffer.size());
        std::streamsize bytes_read = file.gcount();

        for (std::streamsize i = 0; i < bytes_read; ++i) {
            // Double cast, what a hassle! But it works: char -> unsigned char -> uint64_t
            hash ^= static_cast<uint64_t>(static_cast<unsigned char>(buffer[i]));
            hash *= FNV_PRIME;
        }
    }

    return hash;
}

SongUniverse SongDNABuilder::extract_dna(const std::string& filepath) {
    SongUniverse universe;
    universe.track_name = std::filesystem::path(filepath).stem().string();
    universe.seed = compute_file_hash(filepath);

    // Seed standard RNG
    std::mt19937_64 rng(universe.seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_int_distribution<int> palette_choice(0, 4);

    // AI-Generated Harmonious Palette Archetypes
    int choice = palette_choice(rng);
    switch (choice) {
        case 0:
            // Cyberpunk Sunset: Neon Pink, Violet, Golden Amber
            universe.palette.a = {0.5f, 0.5f, 0.5f};
            universe.palette.b = {0.5f, 0.5f, 0.5f};
            universe.palette.c = {1.0f, 1.0f, 1.0f};
            universe.palette.d = {0.00f, 0.33f, 0.67f};
            break;
        case 1:
            // Solar Incandescence: Deep Obsidian, Copper, Solar Gold
            universe.palette.a = {0.5f, 0.4f, 0.3f};
            universe.palette.b = {0.5f, 0.4f, 0.3f};
            universe.palette.c = {1.5f, 1.0f, 1.0f};
            universe.palette.d = {0.00f, 0.15f, 0.30f};
            break;
        case 2:
            // Abyssal Emerald: Deep Oceanic Blues, Teal, Bio-luminescent Green
            universe.palette.a = {0.2f, 0.5f, 0.6f};
            universe.palette.b = {0.3f, 0.5f, 0.5f};
            universe.palette.c = {1.0f, 1.0f, 1.0f};
            universe.palette.d = {0.10f, 0.40f, 0.20f};
            break;
        case 3:
            // Cosmic Amethyst: Velvet Indigo, Lilac, Silver Frost
            universe.palette.a = {0.6f, 0.4f, 0.7f};
            universe.palette.b = {0.4f, 0.3f, 0.5f};
            universe.palette.c = {1.2f, 1.0f, 1.2f};
            universe.palette.d = {0.25f, 0.50f, 0.75f};
            break;
        default:
            // Pearlescent Opal: Subtle pastels with prismatic shifts
            universe.palette.a = {0.8f, 0.7f, 0.7f};
            universe.palette.b = {0.2f, 0.3f, 0.3f};
            universe.palette.c = {1.0f, 1.0f, 1.0f};
            universe.palette.d = {0.00f, 0.20f, 0.40f};
            break;
    }

    // Geometry Morphological DNA
    universe.geometry.core_shape_type = std::floor(dist01(rng) * 3.0f);
    universe.geometry.satellite_count = std::floor(1.0f + dist01(rng) * 2.99f);
    universe.geometry.twist_strength = (dist01(rng) - 0.5f) * 1.5f;
    universe.geometry.roughness_factor = 0.8f + dist01(rng) * 1.2f;

    std::cout << "\n========================================\n";
    std::cout << "SONODERM DNA EXTRACTED FOR: " << universe.track_name << "\n";
    std::cout << "Song Seed: 0x" << std::hex << universe.seed << std::dec << "\n";
    std::cout << "Palette Archetype: " << choice << "\n";
    std::cout << "Core Shape Type: " << universe.geometry.core_shape_type << "\n";
    std::cout << "Satellites: " << universe.geometry.satellite_count << "\n";
    std::cout << "========================================\n\n";

    return universe;
}