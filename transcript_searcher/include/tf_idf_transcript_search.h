#include "transcript_search_algorithm.h"
#include <unordered_map>
#include <SQLiteCpp/SQLiteCpp.h>

/**
 * This is an implementation of a TranscriptSearchAlgorithm which utilizes the TF-IDF algorithm.
 * 
 * It uses a database under the hood which has been populated with preprocessed results and metadata
 * about the candidate transcripts as part of a distributed population of the algorithm.
 * This module serves as the front end which performs final calculations using provided search terms
 * to determine the best matching transcripts.
*/
class TfIdfTranscriptSearch : public TranscriptSearchAlgorithm {
    public:
        // Remove default constructor
        TfIdfTranscriptSearch() = delete;

        // Remove copy constructor and copy assignment
        TfIdfTranscriptSearch(const TfIdfTranscriptSearch&) = delete;
        TfIdfTranscriptSearch& operator= (const TfIdfTranscriptSearch&) = delete;

        /**
         * Initialize a TfIdfTranscriptSearch instance
         * 
         * @param database_path Path to database which stores corpus state for this search algorithm
        */
        TfIdfTranscriptSearch(std::string database_path);

        /**
         * Uses search terms to determine the k-best matching transcripts and stores the 
         * transcripts and their scores in a Vector.
         * 
         * @param search_terms Vector of terms to use in the search
         * @param k Number of best matches to return
         * @param best_matches Vector to store the transcript-score pairs
        */
        void getBestTranscriptMatches(
            std::vector<std::string>& search_terms,
            unsigned int k,
            std::vector<scored_transcript>& best_matches
        );

    private:
        /**
         * Connects to the database which stores corpus state for the search algorithm
         * 
         * @param database_path Path to database
        */
        void connectDatabase(std::string database_path);

        /**
         * Perform term-based preprocessing based on the input search terms
         * 
         * Calculates the corpus IDF for each individual search term, and assembles the entire set of 
         * candidate documents which will be further considered by the search algorithm.
         * These operations are tightly coupled as determining a search term's IDF depends on quantifying 
         * the documents in which it appears. While already viewing the documents in which the term appears,
         * we can assemble the set of potential documents which may be good matches for our search algorithm
         * (as opposed to blindly performing a brute force TF-IDF calculation across all documents).
         * 
         * We use a map to store the results - for search_terms_idfs, we store term and its idf score.
         * For candidate_documents, we really just need a set, but to avoid unneccessary copying we can just form
         * the map of document-tf-idf-score that we will need in the following operation anyway.
         * 
         * @param search_terms Vector of terms to use in the search
         * @param search_terms_idfs Map of term-idf score to be populated by the method
         * @param candidate_documents Map of documents-tf-idf-score to be populated by the method (score initialized to zero)
         * */
        void preprocessTermsCandidates(
            std::vector<std::string>& search_terms,
            std::unordered_map<std::string, double>& search_terms_idfs,
            std::unordered_map<std::string, double>& candidate_documents
        );

        /**
         * For a given document, find the frequency of each search term in that document, and the number of unique terms in the document.
         * 
         * This is an intermediate step for TF-IDF calculation for a document
         * 
         * @param document Name of the document to search
         * @param search_terms Vector of all search terms
         * @param document_term_frequencies Map to populate with terms and their frequencies in this document
         * 
        */
        int getDocumentTermFrequencies(
            std::string& document,
            std::vector<std::string>& search_terms,
            std::unordered_map<std::string, int>& document_term_frequencies
        );

        /**
         * Provided a list of search terms and their corpus IDF scores, calculate the sum of TF-IDF scores of all search terms over each document.
         * 
         * @param search_terms Vector of all search terms
         * @param search_terms_idfs Map of search terms and their corpus IDF scores
         * @param candidate_documents_scores Map of candidate documents and their sum of TF-IDF scores of all search terms
        */
        void calculateTfIdfScores(
            std::vector<std::string>& search_terms,
            std::unordered_map<std::string, double>& search_terms_idfs,
            std::unordered_map<std::string, double>& candidate_documents_scores
        );

        /**
         * Transform a map of documents and their TF-IDF sum scores into a list containing the best K documents and their scores in a pair
         * 
         * @param candidate_documents_scores Map of candidate documents and their sum of TF-IDF scores of all search terms
         * @param k Number of best matches to return
         * 
         * @return Vector of pairs containing best matching documents and their scores
        */
        std::vector<scored_transcript> getBestDocuments(
            std::unordered_map<std::string, double>& candidate_documents_scores,
            unsigned int k
        );

        // Custom destructor to delete dynamic resources
        ~TfIdfTranscriptSearch();

        // Pointer to database instance
        SQLite::Database* db = nullptr;
};