#pragma once

#include <string>
#include <filesystem>
#include <chrono>

#include <nlohmann/json.hpp>

namespace core {

// Configuration for the simulation mesh (will map to sim::Mesh in M3).
struct MeshConfig {
    int gridCells = 24;       // subdivisions per side
    float halfExtent = 1.2f;  // world half-size
};

// Material parameters for the compressible Neo-Hookean model.
struct MaterialConfig {
    float shearModulus = 1000.0f;   // μ (mu)
    float bulkModulus  = 2000.0f;   // λ (lambda) or K
    float density      = 1000.0f;   // ρ for future dynamics
};

// A serialisable simulation project. Pure data with no sim/render deps.
struct Project {
    std::string name;
    std::string createdAt;   // ISO 8601 UTC
    std::string modifiedAt;  // ISO 8601 UTC
    MeshConfig mesh;
    MaterialConfig material;
    // Future: boundary conditions, solver params, contact settings

    nlohmann::json toJson() const;
    static Project fromJson(const nlohmann::json&);

    // Helper: returns a fresh project with default values and timestamps.
    static Project createDefault(const std::string& name);
};

// Lightweight entry for the menu's project browser.
struct ProjectInfo {
    std::string name;
    std::filesystem::path path;
    std::string lastModified;
};

// Scan the projects directory and return a list sorted by last write time
// (newest first). Non-.json files are ignored.
std::vector<ProjectInfo> scanProjects(const std::filesystem::path& projectsDir);

// Save/load a project to/from a .json file. Returns success / loaded project.
bool saveProject(const Project& project, const std::filesystem::path& path);
std::optional<Project> loadProject(const std::filesystem::path& path);
std::string nowIso8601();  // Current UTC time in ISO 8601 format

}  // namespace core