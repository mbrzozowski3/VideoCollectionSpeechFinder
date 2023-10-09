#include "transcript_searcher.h"
#include <iostream>
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <SQLiteCpp/SQLiteCpp.h>

#ifndef PROJECT_BASE_DIR
    #define PROJECT_BASE_DIR "../../"
#endif

using namespace rapidjson;

TranscriptSearcher::TranscriptSearcher(std::string config_path) {
    // Read configuration file into JSON Document
    std::ifstream config_file(config_path);
    std::string config_data((std::istreambuf_iterator<char>(config_file)),
        std::istreambuf_iterator<char>()); // read entire file into string
    Document config;
    config.Parse(config_data.c_str());

    // Initialize using configuration
    database_path = config["Paths"]["database"].GetString();
    std::string full_database_path = PROJECT_BASE_DIR + database_path;
    std::cout << full_database_path << std::endl;

    std::cout << "SQlite3 version " << SQLite::VERSION << " (" << SQLite::getLibVersion() << ")" << std::endl;
    std::cout << "SQliteC++ version " << SQLITECPP_VERSION << std::endl;
}

void TranscriptSearcher::connectDatabase() {
}

void TranscriptSearcher::runSearch() {
}

