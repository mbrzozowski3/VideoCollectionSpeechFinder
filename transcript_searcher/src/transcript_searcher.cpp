#include "transcript_searcher.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <SQLiteCpp/SQLiteCpp.h>

#ifndef PROJECT_BASE_DIR
    #define PROJECT_BASE_DIR "../../"
#endif

using namespace rapidjson;

TranscriptSearcher::TranscriptSearcher(std::string config_path, unsigned int max_search_terms) : max_search_terms(max_search_terms) {
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

bool TranscriptSearcher::readSearchTerms(std::vector<std::string>& search_terms) {
    // Prompt user to enter search terms and read in the line of input
    std::cout << "Enter up to " << max_search_terms << " space-separated search terms: ";
    std::string prompt_input;
    std::getline(std::cin, prompt_input);

    // Turn the input into a string stream and read out individual search terms from it
    std::stringstream prompt_input_ss(prompt_input);
    std::string term;
    // Only read up to `max_search_terms`
    for (unsigned int i = 0; i < max_search_terms; i++) {
        if (prompt_input_ss >> term) {
            search_terms.push_back(term);
        }
    }
    // If there is data left in the prompt, user entered too many terms
    if (prompt_input_ss >> term) {
        std::cout << std::endl << "Too many terms entered" << std::endl;
        return false;
    // Zero terms is not a valid search
    } else if (search_terms.size() == 0) {
        std::cout << std::endl << "No terms entered" << std::endl;
        return false;
    // User has entered valid number of terms
    } else {
        // Normal application path here
        std::cout << std::endl << "Read " << search_terms.size() << " terms: " << std::endl;
        for (auto term : search_terms) {
            std::cout << term << " ";
        }
        std::cout << std::endl;
        return true;
    }
}

void TranscriptSearcher::runSearch() {
    // Main loop for app where we can keep trying new searches
    while (true) {
        std::vector<std::string> search_terms;
        if (readSearchTerms(search_terms)) {
            // Process if we read valid search terms, otherwise it's a retry
            std::cout << "Passthrough" << std::endl;
        }
    }
}

