#include <string>

class TranscriptSearcher {
    public:
        TranscriptSearcher(std::string config_file, unsigned int max_search_terms = 5);
        void runSearch();
        const unsigned int max_search_terms;
    private:
        void connectDatabase();
        std::string database_path;
};