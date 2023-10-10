#include <string>
#include <vector>
#include <unordered_map>
#include <SQLiteCpp/SQLiteCpp.h>

class TranscriptSearcher {
    public:
        // Remove default constructor
        TranscriptSearcher() = delete;

        // Remove copy constructor and copy assignment
        TranscriptSearcher(const TranscriptSearcher&) = delete;
        TranscriptSearcher& operator= (const TranscriptSearcher&) = delete;

        TranscriptSearcher(
            std::string config_file,
            unsigned int max_search_terms = 5,
            unsigned int num_best_results = 3
        );

        void runSearch();

        ~TranscriptSearcher();
        
    private:
        void connectDatabase();

        void preprocessTermsCandidates(
            std::vector<std::string>& search_terms,
            std::unordered_map<std::string, double>& search_terms_idfs,
            std::unordered_map<std::string, double>& candidate_documents
        );

        int getDocumentTermFrequencies(
            std::string& document,
            std::unordered_map<std::string, double>& search_terms_idfs,
            std::unordered_map<std::string, int>& document_term_frequencies
        );

        void calculateTfIdfScores(
            std::unordered_map<std::string, double>& search_terms_idfs,
            std::unordered_map<std::string, double>& candidate_documents
        );

        std::vector<std::pair<std::string, double>> getBestCandidateDocumentsSorted(
            std::unordered_map<std::string, double>& candidate_documents
        );

        void outputResult(std::vector<std::pair<std::string, double>> result);

        bool readSearchTerms(std::vector<std::string>& search_terms);

        std::string database_path;
        SQLite::Database* db = nullptr;
        const unsigned int max_search_terms;
        const unsigned int num_best_results;
};