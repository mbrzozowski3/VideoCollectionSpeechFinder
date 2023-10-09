#include "transcript_searcher.h"
#include <iostream>
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#ifndef PROJECT_BASE_DIR
    #define PROJECT_BASE_DIR "../../"
#endif

using namespace std;
using namespace rapidjson;

TranscriptSearcher::TranscriptSearcher(string config_path) {
    // Read configuration file into JSON Document
    ifstream config_file(config_path);
    string config_data((istreambuf_iterator<char>(config_file)), istreambuf_iterator<char>()); // read entire file into string
    Document config;
    config.Parse(config_data.c_str());

    // Initialize using configuration
    database_path = config["Paths"]["database"].GetString();
    string full_database_path = PROJECT_BASE_DIR + database_path;
}

void TranscriptSearcher::connectDatabase() {
}

void TranscriptSearcher::runSearch() {
}

