#include <string>
#include <vector>

class TranscriptSearcher {
    public:
        TranscriptSearcher(std::string config_file, unsigned int max_search_terms = 5);
        void runSearch();
        const unsigned int max_search_terms;
    private:
        void connectDatabase();
        bool readSearchTerms(std::vector<std::string>& search_terms);
        std::string database_path;
};