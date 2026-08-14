#include <core/project.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace core {

nlohmann::json Project::toJson() const {
    nlohmann::json j;
    j["name"] = name;
    j["createdAt"] = createdAt;
    j["modifiedAt"] = modifiedAt;
    j["mesh"]["gridCells"] = mesh.gridCells;
    j["mesh"]["halfExtent"] = mesh.halfExtent;
    j["material"]["shearModulus"] = material.shearModulus;
    j["material"]["bulkModulus"] = material.bulkModulus;
    j["material"]["density"] = material.density;
    return j;
}

Project Project::fromJson(const nlohmann::json& j) {
    Project p;
    p.name = j.value("name", "Unnamed");
    p.createdAt = j.value("createdAt", nowIso8601());
    p.modifiedAt = j.value("modifiedAt", nowIso8601());

    if (j.contains("mesh")) {
        p.mesh.gridCells = j["mesh"].value("gridCells", 24);
        p.mesh.halfExtent = j["mesh"].value("halfExtent", 1.2f);
    }
    if (j.contains("material")) {
        p.material.shearModulus = j["material"].value("shearModulus", 1000.0f);
        p.material.bulkModulus = j["material"].value("bulkModulus", 2000.0f);
        p.material.density = j["material"].value("density", 1000.0f);
    }
    return p;
}

Project Project::createDefault(const std::string& name) {
    Project p;
    p.name = name;
    p.createdAt = nowIso8601();
    p.modifiedAt = p.createdAt;
    // mesh and material already default-initialised
    return p;
}

bool saveProject(const Project& project, const std::filesystem::path& path) {
    try {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream ofs(path);
        ofs << project.toJson().dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "saveProject: " << e.what() << '\n';
        return false;
    }
}

std::optional<Project> loadProject(const std::filesystem::path& path) {
    try {
        std::ifstream ifs(path);
        nlohmann::json j = nlohmann::json::parse(ifs);
        return Project::fromJson(j);
    } catch (const std::exception& e) {
        std::cerr << "loadProject: " << e.what() << '\n';
        return std::nullopt;
    }
}

std::vector<ProjectInfo> scanProjects(const std::filesystem::path& projectsDir) {
    std::vector<ProjectInfo> result;
    if (!std::filesystem::exists(projectsDir)) {
        return result;
    }
    for (const auto& entry : std::filesystem::directory_iterator(projectsDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        // Read just the name and modified time; avoid full parse for speed.
        std::string name = entry.path().stem().string();
        const auto ftime = std::filesystem::last_write_time(entry);
        const auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        const auto t = std::chrono::system_clock::to_time_t(sctp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M");
        result.push_back({name, entry.path(), ss.str()});
    }
    // Newest first.
    std::sort(result.begin(), result.end(),
              [](const ProjectInfo& a, const ProjectInfo& b) {
                  return a.lastModified > b.lastModified;
              });
    return result;
}

std::string nowIso8601() {
    const auto now = std::chrono::system_clock::now();
    const auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

}  // namespace core