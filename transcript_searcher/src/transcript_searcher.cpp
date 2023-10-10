#include "transcript_searcher.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <cmath>
#include <queue>
#include <functional>
#include <ranges>

#ifndef PROJECT_BASE_DIR
    #define PROJECT_BASE_DIR "../../"
#endif

TranscriptSearcher::TranscriptSearcher(
    std::string config_path,
    unsigned int max_search_terms,
    unsigned int num_best_results
) : max_search_terms(max_search_terms), num_best_results(num_best_results) {
    // Read configuration file into JSON Document
    std::ifstream config_file(config_path);
    std::string config_data((std::istreambuf_iterator<char>(config_file)),
        std::istreambuf_iterator<char>()); // read entire file into string
    rapidjson::Document config;
    config.Parse(config_data.c_str());

    // Initialize database using configuration
    std::string database_relative_path = config["Paths"]["database"].GetString();
    database_path = PROJECT_BASE_DIR + database_relative_path;
    connectDatabase();
}

void TranscriptSearcher::connectDatabase() {
    db = new SQLite::Database(database_path);
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
    } else if (search_terms.empty()) {
        std::cout << std::endl << "No terms entered" << std::endl;
        return false;
    // User has entered valid number of terms
    } else {
        return true;
    }
}

/*
Gather candidate documents and compute search term idfs in a single function -
These operations are tightly coupled and utilize the same information from the database,
so we can consolidate database operations by doing it all right here
*/
void TranscriptSearcher::preprocessTermsCandidates(
    std::vector<std::string>& search_terms,
    std::unordered_map<std::string, double>& search_terms_idfs,
    std::unordered_map<std::string, double>& candidate_documents
) {
    SQLite::Statement count_query(*db, "SELECT COUNT(*) FROM documents");
    // Resolves to true if we had a result from query meaning this term is found in document(s)
    count_query.executeStep();
    int num_documents_total = count_query.getColumn(0).getInt();
    // Get the number of documents for use in IDF calculation
    for (auto term : search_terms) {
        SQLite::Statement term_query(*db, "SELECT documents FROM terms WHERE term = ?");
        term_query.bind(1, term);
        // Resolves to true if we had a result from query meaning this term is found in document(s)
        if(term_query.executeStep()) {
            rapidjson::Document documents_json;
            std::string result = term_query.getColumn(0);
            documents_json.Parse(result.c_str());
            int num_documents_term = documents_json.Size();
            for (int i = 0; i < num_documents_term; i++) {
                candidate_documents[documents_json[i].GetString()] = 0.0;
            }
            // Compute and store IDF for this term
            double term_idf = log2((1.0 + num_documents_total) / (1.0 + num_documents_term));
            search_terms_idfs[term] = term_idf;
        } else {
            search_terms_idfs[term] = 0.0;
        }
    } 
}

int TranscriptSearcher::getDocumentTermFrequencies(
    std::string& document,
    std::unordered_map<std::string, double>& search_terms_idfs,
    std::unordered_map<std::string, int>& document_term_frequencies
) {
    // Query term frequency dict and total number of terms in this document
    SQLite::Statement document_query(*db, "SELECT termFrequencies, numTerms FROM documents WHERE file = ?");
    document_query.bind(1, document);
    int document_num_terms = 0;
    if(document_query.executeStep()) {
        // Get term frequency dict as json
        rapidjson::Document termFrequencies_json;
        std::string result = document_query.getColumn(0);
        termFrequencies_json.Parse(result.c_str());

        // For each term, check if it exists in this document and if so store its frequency in the output
        for (auto t_it = search_terms_idfs.begin(); t_it != search_terms_idfs.end(); t_it++) {
            std::string term = t_it->first;
            if (termFrequencies_json.HasMember(term.c_str())) {
                document_term_frequencies[term] = termFrequencies_json[term.c_str()].GetInt();
            } else {
                document_term_frequencies[term] = 0;
            }
        }

        // Get total number of terms in this document
        document_num_terms = document_query.getColumn(1).getInt();
    }
    return document_num_terms;
}

void TranscriptSearcher::calculateTfIdfScores(
    std::unordered_map<std::string, double>& search_terms_idfs,
    std::unordered_map<std::string, double>& candidate_documents
) {
    // Compute document-sum TF-IDF for each candidate document
    for (auto d_it = candidate_documents.begin(); d_it != candidate_documents.end(); d_it++) {
        std::string document = d_it->first;
        // Get number of terms in document, and frequency of each search term
        std::unordered_map<std::string, int> document_term_frequencies;
        int document_num_terms = getDocumentTermFrequencies(document, search_terms_idfs, document_term_frequencies);
        // Accumulate TF-IDF of each search term for this document
        for (auto t_it = search_terms_idfs.begin(); t_it != search_terms_idfs.end(); t_it++) {
            std::string term = t_it->first;
            double idf = t_it->second;
            double tf = (1.0 * document_term_frequencies[term]) / document_num_terms;
            d_it->second += tf * idf;
        }
    }
}

// Overwrite compare to create min-heap
typedef std::pair<std::string, double> name_score;
bool Compare(name_score a, name_score b) {
    return a.second > b.second;
}

// Implement a K-best algorithm by using a min-heap and popping when size > K
std::vector<name_score> TranscriptSearcher::getBestCandidateDocumentsSorted(
    std::unordered_map<std::string, double>& candidate_documents
) {
    std::priority_queue<name_score, std::vector<name_score>, std::function<bool(name_score, name_score)>> minHeap (Compare);
    for (auto d_it = candidate_documents.begin(); d_it != candidate_documents.end(); d_it++) {
        minHeap.push(std::make_pair(d_it->first, d_it->second));
        if (minHeap.size() > num_best_results) {
            minHeap.pop();
        }
    }
    std::vector<name_score> best_candidate_documents_sorted;
    while (!minHeap.empty()) {
        best_candidate_documents_sorted.push_back(minHeap.top());
        minHeap.pop();
    }
    return best_candidate_documents_sorted;

}

// Format and output result
void TranscriptSearcher::outputResult(std::vector<std::pair<std::string, double>> result) {
    for (auto& element : std::ranges::views::reverse(result)) {
        std::cout << "Score: " << element.second << " | File: " << element.first << std::endl;
    }
}

// Main loop for app where we can continue trying new searches
void TranscriptSearcher::runSearch() {
    while (true) {
        std::vector<std::string> search_terms;
        if (readSearchTerms(search_terms)) {
            // Computer the IDF values for each term and gather set of all documents referenced by any search term
            std::unordered_map<std::string, double> search_terms_idfs;
            std::unordered_map<std::string, double> candidate_documents;
            preprocessTermsCandidates(search_terms, search_terms_idfs, candidate_documents);

            // Calculate the sum of search terms IDF's for each document
            calculateTfIdfScores(search_terms_idfs, candidate_documents);

            // Get only the best few documents as specified by `num_best_terms`
            std::vector<std::pair<std::string, double>> best_candidate_documents_sorted;
            best_candidate_documents_sorted = getBestCandidateDocumentsSorted(candidate_documents);

            // Output the results
            outputResult(best_candidate_documents_sorted);
        }
    }
}

TranscriptSearcher::~TranscriptSearcher() {
    if (db) {
        delete db;
    }
}
