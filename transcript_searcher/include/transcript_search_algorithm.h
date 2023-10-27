#include <string>
#include <vector>

// Create an alias for this frequently used type denoting a transcript's path and its score
typedef std::pair<std::string, double> scored_transcript;

/**
 * Abstract base class for a TranscriptSearchAlgorithm, which must provide an implementation capable
 * of using search terms to produce the k-best matches in a corpus of transcripts.
*/
class TranscriptSearchAlgorithm {
    public:
        // Default constructor
        TranscriptSearchAlgorithm() = default;

        // Default destructor
        virtual ~TranscriptSearchAlgorithm() = default;

        // Remove copy constructor and copy assignment
        TranscriptSearchAlgorithm(const TranscriptSearchAlgorithm&) = delete;
        TranscriptSearchAlgorithm& operator= (const TranscriptSearchAlgorithm&) = delete;
        /**
         * Uses search terms to determine the k-best matching transcripts and stores the 
         * transcripts and their scores in a Vector.
         * 
         * @param search_terms Vector of terms to use in the search
         * @param k Number of best matches to return
         * @param best_matches Vector to store the transcript-score pairs
        */
        virtual void getBestTranscriptMatches(
            const std::vector<std::string>& search_terms,
            const unsigned int k,
            std::vector<scored_transcript>& best_matches
        ) = 0;
};