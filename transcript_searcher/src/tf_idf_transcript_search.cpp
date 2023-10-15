#include "tf_idf_transcript_search.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <cmath>
#include <queue>
#include <functional>
#include <algorithm>

TfIdfTranscriptSearch::TfIdfTranscriptSearch(const std::string database_path) {
    connectDatabase(database_path);
}

void TfIdfTranscriptSearch::connectDatabase(const std::string database_path) {
    // Create unique pointer for database object
    db = std::make_unique<SQLite::Database>(database_path);
}

void TfIdfTranscriptSearch::preprocessTermsCandidates(
    const std::vector<std::string>& search_terms,
    std::unordered_map<std::string, double>& search_terms_idfs,
    std::unordered_map<std::string, double>& candidate_documents
) {
    SQLite::Statement count_query(*db, "SELECT COUNT(*) FROM documents");
    // Resolves to true if we had a result from query meaning this term is found in document(s)
    count_query.executeStep();
    // Get the number of documents for use in IDF calculation
    int num_documents_total = count_query.getColumn(0).getInt();
    // Get the list of documents each term appears in
    for (auto term : search_terms) {
        SQLite::Statement term_query(*db, "SELECT documents FROM terms WHERE term = ?");
        term_query.bind(1, term);
        // Resolves to true if we had a result from query meaning this term is found in document(s)
        if(term_query.executeStep()) {
            rapidjson::Document documents_json;
            std::string result = term_query.getColumn(0);
            documents_json.Parse(result.c_str());
            // Get the number of documents this term appears in
            int num_documents_term = documents_json.Size();
            for (int i = 0; i < num_documents_term; i++) {
                // Add each document to the candidate document set, and initialize its TF-IDF sum score to 0.0
                // (this may be peformed redundantly if the same document appears for other terms, but effectively
                // the map will act as a set of documents which appear in union of terms)
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

int TfIdfTranscriptSearch::getDocumentTermFrequencies(
    const std::string& document,
    const std::vector<std::string>& search_terms,
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
        for (auto& term : search_terms) {
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

void TfIdfTranscriptSearch::calculateTfIdfScores(
    const std::vector<std::string>& search_terms,
    const std::unordered_map<std::string, double>& search_terms_idfs,
    std::unordered_map<std::string, double>& candidate_documents_scores
) {
    // Compute document-sum TF-IDF for each candidate document
    for (auto d_it = candidate_documents_scores.begin(); d_it != candidate_documents_scores.end(); d_it++) {
        std::string document = d_it->first;
        // Get number of terms in document, and frequency of each search term
        std::unordered_map<std::string, int> document_term_frequencies;
        int document_num_terms = getDocumentTermFrequencies(document, search_terms, document_term_frequencies);
        // Accumulate TF-IDF of each search term for this document
        for (auto t_it = search_terms_idfs.begin(); t_it != search_terms_idfs.end(); t_it++) {
            std::string term = t_it->first;
            double idf = t_it->second;
            double tf = (1.0 * document_term_frequencies[term]) / document_num_terms;
            d_it->second += tf * idf;
        }
    }
}

// Custom compare function to alter a priority queue to behave as min heap
bool Compare(scored_transcript a, scored_transcript b) {
    return a.second > b.second;
}

/**
 * Implements a K-best algorithm by using a K-sized minheap to hold documents by score
 * The remaining documents in the heap are the K-best in reverse order
*/
std::vector<scored_transcript> TfIdfTranscriptSearch::getBestDocuments(
    const std::unordered_map<std::string, double>& candidate_documents_scores,
    const unsigned int k
) {
    // Push each document onto the heap sorting by its score, removing the lowest score from heap once we have > K elements
    std::priority_queue<scored_transcript, std::vector<scored_transcript>, std::function<bool(scored_transcript, scored_transcript)>> minHeap (Compare);
    for (auto d_it = candidate_documents_scores.begin(); d_it != candidate_documents_scores.end(); d_it++) {
        minHeap.push(std::make_pair(d_it->first, d_it->second));
        if (minHeap.size() > k) {
            minHeap.pop();
        }
    }
    // Retrieve the K-best documents (sorted worst to best)
    std::vector<scored_transcript> best_candidate_documents;
    while (!minHeap.empty()) {
        best_candidate_documents.push_back(minHeap.top());
        minHeap.pop();
    }
    // Fix the ordering of our documents to be sorted best to worst
    std::reverse(best_candidate_documents.begin(), best_candidate_documents.end());
    return best_candidate_documents;

}

void TfIdfTranscriptSearch::getBestTranscriptMatches(
    const std::vector<std::string>& search_terms,
    const unsigned int k,
    std::vector<scored_transcript>& best_matches
) {
    // Compute the IDF values for each term and gather set of all documents referenced by any search term
    std::unordered_map<std::string, double> search_terms_idfs;
    std::unordered_map<std::string, double> candidate_documents_scores;
    preprocessTermsCandidates(search_terms, search_terms_idfs, candidate_documents_scores);

    // Calculate the sum of search terms IDF's for each document
    calculateTfIdfScores(search_terms, search_terms_idfs, candidate_documents_scores);

    // Use the score of each document to pick the K-best documents from the set
    best_matches = getBestDocuments(candidate_documents_scores, k);
}
