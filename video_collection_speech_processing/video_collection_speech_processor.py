import glob
import os

from transcript_preprocessing.tf_idf_transcript_preprocessor import TfIdfTranscriptPreprocessor
from video_transcription.video_transcriber import VideoTranscriber


class VideoCollectionSpeechProcessor:
    """Process speech for a collection of videos.

    Performs speech-to-text on directories of videos, and performs preprocessing
    on the resulting transcriptions for later use in the designated search algorithm frontend.
    """

    def __init__(self, db_path, speech_model="base.en", search_algorithm="tf-df"):
        """Initialize instance with a database, speech model, and search algorithm

        Args:
            db_path: String
                Path to database for use in transcript preprocessing
            speech_model: String
                Model for transcriber to use in speech-to-text
            search_algorithm: String
                Algorithm for preprocessor to handle
        """

        # Speech to text module
        self.__video_transcriber = VideoTranscriber(speech_model)

        # Search algorithm preprocessing module
        if (search_algorithm == "tf-idf"):
            self.__transcript_preprocessor = TfIdfTranscriptPreprocessor(db_path)
        else:
            raise Exception(f"Exception: Invalid search algorithm \"{search_algorithm}\"")

    def process_sources(self, source_path, source_format="mp4"):
        """Transcribe and preprocess sources of a given format in target directory

        Args:
            source_path: String
                Path to directory for which to process sources
            source_format: String
                File format to process
        """
        # Search for all files of type `source_format` in `source_path` directory
        for video_path in glob.glob(f"{source_path}/*.{source_format}"):
            # Take absolute paths for uniqueness
            video_abspath = os.path.abspath(video_path)

            # Don't process previously handled files
            if not self.__transcript_preprocessor.exists(video_abspath):
                # Get raw text transcript
                transcript = self.__video_transcriber.transcribe_source(video_abspath)
                # Perform preprocessing (algorithm dependent)
                self.__transcript_preprocessor.preprocess(video_abspath, transcript)


def main():
    print("VideoCollectionSpeechProcessor")


if __name__ == "__main__":
    main()
