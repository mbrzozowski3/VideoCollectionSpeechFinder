#include <string>

using namespace std;

class TranscriptSearcher {
    public:
        TranscriptSearcher(string config_file);
        void runSearch();
    private:
        void connectDatabase();
        string database_path;
};