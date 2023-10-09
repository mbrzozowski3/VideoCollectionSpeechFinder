#include <string>

class TranscriptSearcher {
    public:
        TranscriptSearcher(std::string config_file);
        void runSearch();
    private:
        void connectDatabase();
        std::string database_path;
};