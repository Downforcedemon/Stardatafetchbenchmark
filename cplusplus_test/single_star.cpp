#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <curl/curl.h>
#include <filesystem>
#include "fitsio.h"

// Namespace for filesystem (C++17)
namespace fs = std::filesystem;

// Function to write data fetched by CURL into a file
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

// Function to fetch metadata from the MAST API
std::string fetch_metadata(const std::string& star_id, const std::string& log_file) {
    try {
        // Format the query URL
        std::ostringstream metadata_url;
        metadata_url << "https://mast.stsci.edu/api/v0/invoke?request="
                     << R"({"service":"Mast.Name.Lookup","params":{"input":")"
                     << star_id
                     << R"(","format":"json"}})";

        // Initialize CURL for metadata query
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("CURL initialization failed for metadata query.");
        }

        std::ostringstream response;
        curl_easy_setopt(curl, CURLOPT_URL, metadata_url.str().c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* data, size_t size, size_t nmemb, std::ostringstream* writer) -> size_t {
            writer->write(static_cast<const char*>(data), size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            throw std::runtime_error("CURL error while fetching metadata: " + std::string(curl_easy_strerror(res)));
        }

        curl_easy_cleanup(curl);
        return response.str();  // Return the metadata response as a string
    } catch (const std::exception& e) {
        std::ofstream log_stream(log_file, std::ios::app);
        log_stream << "Failed to fetch metadata for star_id: " << star_id
                   << ". Error: " << e.what() << "\n";
        log_stream.close();
        return "";
    }
}

// Setup logging function
void setup_logging(const std::string& log_file) {
    std::ofstream log_stream(log_file, std::ios::app);
    if (!log_stream.is_open()) {
        throw std::runtime_error("Failed to open log file: " + log_file);
    }

    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    log_stream << "====================================================\n";
    log_stream << "Logging session started: "
               << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S")
               << "\n";
    log_stream << "====================================================\n";
    log_stream.close();
}

// Fetch star data function
double fetch_star_data(const std::string& star_id, const std::string& log_file) {
    try {
        // Fetch metadata and parse required details
        std::string metadata = fetch_metadata(star_id, log_file);
        if (metadata.empty()) {
            throw std::runtime_error("Failed to retrieve metadata for star ID: " + star_id);
        }

        // Extract observation and file details (mock for now)
        // Replace this with proper JSON parsing logic
        std::string file_name = "tess2021258175143-s0043-0000000084441541-0214-s_lc.fits";

        // Construct the URL
        std::ostringstream url;
        url << "https://mast.stsci.edu/api/v0.1/Download/file?uri=mast:TESS/product/" << file_name;

        // Log the constructed URL
        std::ofstream log_stream(log_file, std::ios::app);
        log_stream << "Constructed URL: " << url.str() << "\n";
        log_stream.close();

        // Initialize CURL and perform the request
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("CURL initialization failed");
        }

        std::string output_dir = "data/raw/";
        fs::create_directories(output_dir);
        std::string output_path = output_dir + star_id + "_cpp.fits";

        FILE* file = fopen(output_path.c_str(), "wb");
        if (!file) {
            curl_easy_cleanup(curl);
            throw std::runtime_error("Failed to open file: " + output_path);
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

        CURLcode res = curl_easy_perform(curl);
        fclose(file);

        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            throw std::runtime_error("CURL error: " + std::string(curl_easy_strerror(res)));
        }

        curl_easy_cleanup(curl);

        auto end_time = std::chrono::high_resolution_clock::now();
        double elapsed_time = std::chrono::duration<double>(end_time - std::chrono::high_resolution_clock::now()).count();

        log_stream.open(log_file, std::ios::app);
        log_stream << "Successfully fetched data for " << star_id
                   << " in " << elapsed_time << " seconds\n";
        log_stream.close();

        return elapsed_time;
    } catch (const std::exception& e) {
        std::ofstream log_stream(log_file, std::ios::app);
        log_stream << "Failed to fetch data for " << star_id
                   << ". Error: " << e.what() << "\n";
        log_stream.close();
        return -1.0;
    }
}

// Main function
int main() {
    const std::string log_file = "/home/sm/Projects/StarDataFetchBenchmark/data/logs/single_star_cpp.log";

    try {
        // Set up logging
        setup_logging(log_file);
        std::cout << "Logging setup complete." << std::endl;

        // Load star IDs from inputs/star_ids.txt
        const std::string input_file = "/home/sm/Projects/StarDataFetchBenchmark/inputs/star_ids.txt";
        std::ifstream file(input_file);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open input file: " + input_file);
        }

        std::vector<std::string> star_ids;
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                star_ids.push_back(line);
            }
        }
        file.close();

        if (star_ids.empty()) {
            throw std::runtime_error("No star IDs found in the input file.");
        }

        std::cout << "Starting single-star fetch test..." << std::endl;

        // Fetch data for the first star in the list
        std::string star_id = star_ids[0];
        std::cout << "Fetching data for star ID: " << star_id << std::endl;
        double time_taken = fetch_star_data(star_id, log_file);

        if (time_taken >= 0) {
            std::cout << "Success: Fetched data for " << star_id
                      << " in " << time_taken << " seconds" << std::endl;
        } else {
            std::cerr << "Failed to fetch data for star ID: " << star_id << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
