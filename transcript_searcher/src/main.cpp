#include <iostream>
#include "transcript_searcher.h"
#include "argparse/argparse.hpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <fstream>

#ifndef PROJECT_BASE_DIR
    #define PROJECT_BASE_DIR "../../"
#endif

int main(int argc, char** argv) {

    // Configure the CLI
    argparse::ArgumentParser program("transcript_searcher");
    program.add_argument("-c", "--config_file").default_value(std::string{"config.json"});
    program.add_argument("-a", "--search_algorithm").default_value(std::string{"tf-idf"});
    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(1);
    }

    // Find configuration file
    std::string config_path = program.get<std::string>("--config_file");
    std::string config_abspath = PROJECT_BASE_DIR + config_path;

    // Read configuration file into JSON Document
    std::ifstream config_file(config_abspath);
    std::string config_data((std::istreambuf_iterator<char>(config_file)),
        std::istreambuf_iterator<char>()); // read entire file into string

    // Parse config into json
    rapidjson::Document config;
    config.Parse(config_data.c_str());

    // Get database path from configuration
    std::string database_relative_path = config["Paths"]["database"].GetString();
    std::string database_abspath = PROJECT_BASE_DIR + database_relative_path;


    // Get search algorithm from args
    std::string search_algorithm = program.get<std::string>("search_algorithm");

    // Initialize a TranscriptSearcher and launch the search process
    try {
        TranscriptSearcher transcript_searcher(database_abspath, search_algorithm);
        transcript_searcher.runSearch();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        std::exit(1);
    }

    return 0;
}
