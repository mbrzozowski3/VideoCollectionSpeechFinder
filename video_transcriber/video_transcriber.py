import os
import glob
import openai
import sqlite3
import string
import json
import collections
import backoff
import whisper
import nltk
from moviepy.editor import VideoFileClip
from nltk.corpus import stopwords

nltk.download("stopwords", quiet=True)

class VideoTranscriber:
    # Basic initialization
    def __init__(self, args):
        self.project_root_path = args.project_root_path
        self.sources_path = args.source_content_path
        self.source_format = args.source_format
        self.config_path = self.project_root_path + args.config_file
        self.audio_ext = "mp3"
        self._transcript_off_ = args.TRANSCRIPT_OFF
        self.conn = None
        self.config()
        self.connect_db()
        self.model = whisper.load_model("base.en")

    # Any config file-based setup required (accessing non-relative locations)
    def config(self):
        with open(self.config_path, encoding="utf-8") as config_file:
            config_dict = json.load(config_file)
            openai.api_key = config_dict["ApiKeys"]["openai"]
            self.db_path = self.project_root_path + config_dict["Paths"]["database"]

    # Connect to the database
    def connect_db(self):
        self.conn = sqlite3.connect(self.db_path)
        self.cur = self.conn.cursor()

    # Create a path to use for a temp audio file for a given video file
    def create_audio_path(self, video_path):
        path, _ = os.path.splitext(video_path)
        audio_path = f"{path}.{self.audio_ext}"
        return audio_path

    # Demux audio from video and write to a file
    def extract_audio(self, video_path, audio_path):
        clip = VideoFileClip(video_path)
        clip.audio.write_audiofile(audio_path, verbose=False, logger=None)

    # Wrap API call in backoff
    @backoff.on_exception(backoff.expo, openai.error.RateLimitError)
    def transcribe_with_backoff(self, audio_file):
        return openai.audio.transcribe("whisper-1", audio_file)

    # Perform speech to text
    def speech_to_text(self, audio_path):
        # Use a dummy transcription
        if self._transcript_off_:
            return "DEBUG,! Transcription? - this is a transcription"
        # Perform transcription using OpenAI whisper model
        else:
            result = self.model.transcribe(audio_path, language="english")
            return result["text"]

    # Get existing documents from the DB (return cursor)
    def get_documents(self):
        result = self.cur.execute("SELECT file FROM documents")
        return result

    # Get existing global term from DB (return cursor)
    def get_term(self, term):
        data = (term,)
        result = self.cur.execute("SELECT * FROM terms WHERE term=?", data)
        return result

    # Insert a document for a given video path into the DB
    def insert_document(self, video_path, transcript, term_frequencies, num_terms):
        data = (video_path, transcript, term_frequencies, num_terms)
        self.cur.execute("INSERT INTO documents VALUES (?, ?, ?, ?)", data)
        self.conn.commit()

    # Calculate a dictionary of term frequencies for a transcript
    def get_term_frequencies(self, transcript):
        # Get stop words - 50 most common words
        stop_words = stopwords.words("english")
        # Remove stop words from term frequency counting
        word_list = [word for word in transcript.split() if word not in stop_words]
        return collections.Counter(word_list)

    # Insert an inverse index containing the document this term appears in
    def insert_term(self, term, document):
        data = (term, json.dumps([document]))
        self.cur.execute("INSERT INTO terms VALUES (?, ?)", data)
        self.conn.commit()

    # Update DB with additional inverted index document
    def update_term(self, term, documents):
        data = (json.dumps(documents), term)
        self.cur.execute("UPDATE terms SET documents = ? WHERE term = ?", data)
        self.conn.commit()

    # For a dict of terms in a given document, update the global state of that term in DB
    def update_terms(self, document, term_frequencies):
        for term, _ in term_frequencies.items():
            result = self.get_term(term)
            # If exists, update entry, else create a new one
            entry = result.fetchone()
            if entry:
                # Add document to inverted index
                documents = (json.loads(entry[1]))
                documents.append(document)
                # Update term
                self.update_term(term, documents)
            else:
                # Initialize a new term with this document
                self.insert_term(term, document)

    # Perform DB updates as required by the TF-IDF algorithm implementation
    def update_db_tf_idf(self, path, transcript):
        # Clean up transcript by removing punctuation and converting to lowercase
        transcript = transcript.translate(str.maketrans(dict.fromkeys(string.punctuation))).lower()
        # Generate term frequencies and use them to create a new document and update terms
        term_frequencies = self.get_term_frequencies(transcript)
        self.insert_document(path, transcript, json.dumps(term_frequencies), len(term_frequencies))
        self.update_terms(path, term_frequencies)

    # For a given file and its transcript, perform algorithm-dependent DB operations
    def update_db(self, path, transcript):
        # TODO - Can perform switch here based on which algorithm is used
        self.update_db_tf_idf(path, transcript)

    # Transcribe all sources in a directory and store in the DB (Don't regenerate if already present)
    def transcribe_sources(self):
        documents = self.get_documents().fetchall()
        # Search for all files of type `.source_format`
        for video_path in glob.glob(f"{self.sources_path}/*.{self.source_format}"):
            # Need absolute paths for uniqueness in DB
            full_video_path = os.path.abspath(video_path)
            # Avoid processing anything already transcripted in the DB
            if (full_video_path,) not in documents:
                # Get audio split from the video
                temp_audio_path = self.create_audio_path(video_path)
                self.extract_audio(video_path, temp_audio_path)
                # Perform speech to text
                transcript = self.speech_to_text(temp_audio_path)
                # Remove the temporary audio file we just needed for the API call
                if os.path.isfile(temp_audio_path):
                    os.remove(temp_audio_path)
                # Update DB (implementation and what's under the hood here might be algorithm-dependent)
                self.update_db(full_video_path, transcript)

    # Clean up resources in destructor
    def __del__(self):
        if self.conn:
            self.conn.close()


def main():
    print("VideoTranscriber")


if __name__ == "__main__":
    main()
