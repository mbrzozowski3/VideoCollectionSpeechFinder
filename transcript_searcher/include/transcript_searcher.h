#include <string>
#include <vector>
#include <unordered_map>
#include <SQLiteCpp/SQLiteCpp.h>
#include "tf_idf_transcript_search.h"

/**
 * Search a corpus of transcripts using a number of user-provided search terms.
 * 
 * Interacts with the user, and utilizes a TranscriptSearchAlgorithm under the hood to perform the search.
*/
class TranscriptSearcher {
    public:
        // Remove default constructor
        TranscriptSearcher() = delete;

        // Remove copy constructor and copy assignment
        TranscriptSearcher(const TranscriptSearcher&) = delete;
        TranscriptSearcher& operator= (const TranscriptSearcher&) = delete;

        /**
         * Initialize a TranscriptSearcher instance
         * 
         * @param database_path Path to database which stores corpus state for the given search algorithm
         * @param search_algorithm Algorithm to be used for searching transcripts
         * @param max_search_terms Maximum number of terms allowed for a user to search for at once
         * @param num_best_results Number of top-scoring results to return to the user
        */
        TranscriptSearcher(
            const std::string database_path,
            const std::string search_algorithm = "tf-idf",
            const unsigned int max_search_terms = 5,
            const unsigned int num_best_results = 3
        );

        /**
         * Runs the user-facing main loop.
         * 
         * Prompts user for input, feeds it to the search algorithm, and presents the user with results. 
        */
        void runSearch();
        
        // Default destructor
        ~TranscriptSearcher() = default;
        
    private:
        /**
         * Prompts user if they would like to perform a new search and returns their decision.
         * 
         * @return `true` if user would like to search, `false` if they are done.
        */
        bool performNewSearch();

        /**
         * Prompts user to enter search terms, retrieves them, and stores them in `search_terms`
         * 
         * @param search_terms Vector to be updated with individual search terms as elements
         * @return `true` if valid input was provided, otherwise `false`
        */
        bool readSearchTerms(std::vector<std::string>& search_terms);

        /**
         * Formats and outputs the results for the user.
         * 
         * @param result Vector of scored transcripts, ordered by score
        */
        void outputResult(const std::vector<scored_transcript>& result);

        // Maximum number of terms the transcript searcher will accept
        const unsigned int max_search_terms;
        // Number of top results that should be displayed to the user
        const unsigned int num_best_results;

        // Base pointer to a TranscriptSearchAlgorithm implementation
        TranscriptSearchAlgorithm* transcript_search_algorithm = nullptr;
};