// Dual Licensing Either :
// - AGPL
// or
// - Subscription license for commercial usage (without requirement of licensing propagation).
//   please contact ballandfys@protonmail.com for additional information about this subscription commercial licensing.
//
// Created by FyS on 23.07.25. License 2022-2025
//
// In the case no license has been purchased for the use (modification or distribution in any way) of the software stack
// the APGL license is applying.
//

#ifndef FIL_TEMPORARY_HH
#define FIL_TEMPORARY_HH

namespace fil {

/**
 * @brief Create a temporary file with a unique generated name on the temporary folder of the current OS.
 * This file is removed upon destruction of the object
 */
class temporary_file {
  public:
    explicit temporary_file(const std::string& content, const std::string& prefix = "temp_file", const std::string& extension = ".txt")
        : path_([&] {
            // Generate a unique filename
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1000, 9999);

            const auto filename  = fmt::format("{}_{}{}", prefix, dis(gen), extension);
            const auto temp_path = std::filesystem::temp_directory_path() / filename;
            std::ofstream file(temp_path);
            if (!file) {
                throw std::runtime_error(fmt::format("Could not open file for writing: {}", temp_path.string()));
            }

            file << content;
            if (!file) {
                throw std::runtime_error(fmt::format("Error writing to file: {}", temp_path.string()));
            }

            return temp_path;
        }()) {}

    ~temporary_file() { std::filesystem::remove(path_); }

    operator std::filesystem::path() const { return path_; }

  private:
    std::filesystem::path path_;
};
} // namespace fil

#endif // FIL_TEMPORARY_HH
