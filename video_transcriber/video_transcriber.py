import os
import glob
import openai
import sqlite3
import string
import json
import collections
import backoff
import whisper

from configparser import ConfigParser
from moviepy.editor import VideoFileClip

import nltk
nltk.download('stopwords', quiet=True)
from nltk.corpus import stopwords


class VideoTranscriber:
    # Basic initialization
    def __init__(self, args):
        self.sources_path = args.source_content_path
        self.source_format = args.source_format
        self.config_path = args.config_file
        self.audio_ext = "mp3"
        self.__TRANSCRIPT_OFF__ = args.TRANSCRIPT_OFF
        self.conn = None
        self.config()
        self.connect_db()
        self.model = whisper.load_model("base.en")

    # Any config file-based setup required
    def config(self):
        self.config = ConfigParser()
        self.config.read(self.config_path)
        openai.api_key = self.config['ApiKeys']['openai']
        self.db_path = self.config['Paths']['database']
    
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
    def transcribe_with_backoff(audio_file):
        return openai.audio.transcribe("whisper-1", audio_file)

    # Perform speech to text
    def speech_to_text(self, audio_path):
        # Use a dummy transcription
        if (self.__TRANSCRIPT_OFF__):
            return "DEBUG,! Transcription? - this is a transcription"
        # Perform transcription using OpenAI whisper model
        else:
            result = self.model.transcribe(audio_path, language='english')
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
    def insert_document(self, video_path, transcript, term_frequencies):
        data = (video_path, transcript, term_frequencies)
        self.cur.execute("INSERT INTO documents VALUES (?, ?, ?)", data)
        self.conn.commit()

    # Calculate a dictionary of term frequencies for a transcript
    def get_term_frequencies(self, transcript):
        # Get stop words - 50 most common words which offer little help in differentiating candidates
        stop_words = stopwords.words('english')
        # Remove stop words from term frequency counting
        wordList = [word for word in transcript.split() if word not in stop_words]
        return collections.Counter(wordList)

    # Insert an inverse index containing the document this term appears in and its global frequency
    def insert_term(self, term, document, frequency):
        data = (term, json.dumps([document]), frequency)
        self.cur.execute("INSERT INTO terms VALUES (?, ?, ?)", data)
        self.conn.commit()

    # Update DB with additional inverted index document and global frequency value
    def update_term(self, term, documents, frequency):
        data = (json.dumps(documents), frequency, term)
        self.cur.execute("UPDATE terms SET documents = ?, globalFrequency = ? WHERE term = ?", data)
        self.conn.commit()

    # For a dict of terms and their frequencies in a given document, update the global state of that term in DB
    def update_terms(self, document, termFrequencies):
        for term, frequency in termFrequencies.items():
            result = self.get_term(term)
            # There can only be one global entry for an exact given term, so just fetch one
            entry = result.fetchone()
            # If it already exists, update instead of inserting a new entry
            if entry:
                # Retrieve the inverted index array of documents which contain this term, and add this new document to it
                documents = (json.loads(entry[1]))
                documents.append(document)
                # Retrieve the global frequency and increment it by the number of times the term appears in this new document
                globalFrequency = entry[2] + frequency
                # Update this term with its new list of documents and global frequency
                self.update_term(term, documents, globalFrequency)
            else:
                # Initialize a new term with this document and its frequency
                self.insert_term(term, document, frequency)

    # Perform DB updates as required by the TF-IDF algorithm implementation
    def update_db_tf_idf(self, path, transcript):
        # Clean up transcript by removing punctuation and converting to lowercase
        transcript = transcript.translate(str.maketrans(dict.fromkeys(string.punctuation))).lower()
        # Create term frequency dictionary
        termFrequencies = self.get_term_frequencies(transcript)
        # Add the resulting transcription and term frequency calculation to DB
        self.insert_document(path, transcript, json.dumps(termFrequencies))
        # Update the inverted index and global term frequency count in DB
        self.update_terms(path, termFrequencies)
    
    # For a given file and its transcript, perform algorithm-dependent DB operations
    def update_db(self, path, transcript):
        # TODO - Can perform switch here based on which algorithm is used
        self.update_db_tf_idf(path, transcript)

    # Transcribe all sources in a directory and store in the DB (Don't regenerate if already present)
    def transcribe_sources(self):
        # Just returns a cursor so we still need to fetch to get the documents out of DB
        documents = self.get_documents().fetchall()
        # Search for all videos of designated type in the source directory
        for video_path in glob.glob(f"{self.sources_path}/*.{self.source_format}"):
            # Will need absolute paths for uniqueness in DB
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
        if (self.conn):
            self.conn.close()


def main():
    print("VideoTranscriber")


if __name__ == "__main__":
    main()
