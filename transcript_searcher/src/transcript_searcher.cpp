#include "transcript_searcher.h"
#include <iostream>
#include <sstream>

TranscriptSearcher::TranscriptSearcher(
    const std::string database_path,
    const std::string search_algorithm,
    const unsigned int max_search_terms,
    const unsigned int num_best_results
) : max_search_terms(max_search_terms), num_best_results(num_best_results) {
    if (search_algorithm == "tf-idf") {
        transcript_search_algorithm = new TfIdfTranscriptSearch(database_path);
    } else {
        throw std::runtime_error("Error: invalid search algorithm \"" + search_algorithm + "\"\n");
    }
}

bool TranscriptSearcher::performNewSearch() {
    // Prompt user to enter search terms and read in the line of input
    std::cout << "ENTER to continue, \"exit\" to quit" << std::endl;
    std::string prompt_input;
    std::getline(std::cin, prompt_input);

    // Turn the input into a string stream and read out individual search terms from it
    std::stringstream prompt_input_ss(prompt_input);
    std::string term;
    if (prompt_input_ss >> term) {
        if (term == "exit") {
            return false;
        }
    }
    return true;
}

bool TranscriptSearcher::readSearchTerms(std::vector<std::string>& search_terms) {
    // Prompt user to enter search terms and read in the line of input
    std::cout << "Enter up to " << max_search_terms << " search terms: ";
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
    } else if (search_terms.empty()) {
        std::cout << std::endl << "No terms entered" << std::endl;
        return false;
    // User has entered valid number of terms
    } else {
        return true;
    }
}

void TranscriptSearcher::runSearch() {
    while (true) {
        if (performNewSearch()) {
            std::vector<std::string> search_terms;
            if (readSearchTerms(search_terms)) {
                // Get only `num_best_results` results
                std::vector<scored_transcript> best_transcripts;
                transcript_search_algorithm->getBestTranscriptMatches(search_terms, num_best_results, best_transcripts);

                // Output the results
                outputResult(best_transcripts);
            }
        } else {
            return;
        }
    }
}

void TranscriptSearcher::outputResult(const std::vector<scored_transcript>& result) {
    for (auto& element : result) {
        std::cout << "Score: " << element.second << " | File: " << element.first << std::endl;
    }
}
