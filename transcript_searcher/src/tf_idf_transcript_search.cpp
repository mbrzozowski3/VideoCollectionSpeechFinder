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

// Custom compare function to change priority queue to behave as min heap
bool Compare(scored_transcript a, scored_transcript b) {
    return a.second > b.second;
}

std::vector<scored_transcript> TfIdfTranscriptSearch::getBestDocuments(
    const std::unordered_map<std::string, double>& candidate_documents_scores,
    const unsigned int k
) {
    std::priority_queue<scored_transcript, std::vector<scored_transcript>, std::function<bool(scored_transcript, scored_transcript)>> minHeap (Compare);
    for (auto d_it = candidate_documents_scores.begin(); d_it != candidate_documents_scores.end(); d_it++) {
        minHeap.push(std::make_pair(d_it->first, d_it->second));
        if (minHeap.size() > k) {
            minHeap.pop();
        }
    }
    std::vector<scored_transcript> best_candidate_documents_sorted;
    while (!minHeap.empty()) {
        best_candidate_documents_sorted.push_back(minHeap.top());
        minHeap.pop();
    }
    std::reverse(best_candidate_documents_sorted.begin(), best_candidate_documents_sorted.end());
    return best_candidate_documents_sorted;

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
        
    best_matches = getBestDocuments(candidate_documents_scores, k);
}
