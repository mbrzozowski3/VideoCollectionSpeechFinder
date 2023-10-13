from abc import ABC, abstractmethod

class TranscriptPreprocessor(ABC):
    """Abstract interface class deining a TranscriptPreprocessor

    The TranscriptPreprocessor is reponsible for preprocessing on a text transcript based on the
    search algorithm it is supporting.
    """
    
    @abstractmethod
    def preprocess(self, path, transcript):
        """Preprocess a source file given its transcript

        Args:
            path: String
                Path to source file of the transcript
            transcript: String
                Corresponding transcript to the source file
        """
        pass

    @abstractmethod
    def exists(self, path):
        """Return whether or not a given source file has already been processed

        Args:
            path: String
                Path to source file being queried

        Returns:
            Boolean true if file has already been processed false otherwise
        """
        pass