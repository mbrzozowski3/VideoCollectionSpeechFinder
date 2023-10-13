import collections
import json
import string

import nltk
from nltk.corpus import stopwords
import sqlite3

from transcript_preprocessing.transcript_preprocessor import TranscriptPreprocessor

nltk.download("stopwords", quiet=True)


class TfIdfTranscriptPreprocessor(TranscriptPreprocessor):
    """Performs transcript preprocessing for the TF-IDF algorithm.

    Interfaces with an SQLite database to store intermediate preprocessed results
    to improve performance of the frontend module of the search algorithm.
    """

    def __init__(self, db_path):
        """Initialize instance with a database

        Args:
            db_path: String
                Path to database for use in transcript preprocessing
        """
        self.__conn = None
        self.__connect_db(db_path)
        self.__existing_documents = None

    def __connect_db(self, db_path):
        """Connect to the SQLite database for intermediate results

        Args:
            db_path: String
                Path to database for use in transcript preprocessing
        """
        self.__conn = sqlite3.connect(db_path)
        self.__cur = self.__conn.cursor()

    def preprocess(self, path, transcript):
        """Preprocess the transcript for a given path according to the supported search algorithm

        Args:
            path: String
                Path of source file from which the transcript was generated
            transcript: String
                Text transcript of speech in source file
        """
        # Clean up transcript by removing punctuation and converting to lowercase
        transcript = transcript.translate(str.maketrans(dict.fromkeys(string.punctuation))).lower()

        # Generate document term frequencies
        term_frequencies = self.__get_term_frequencies(transcript)

        # Create a new document, update global state of terms which appeared in this document
        self.__insert_document(path, transcript, json.dumps(term_frequencies), len(term_frequencies))
        self.__update_terms(path, term_frequencies)

    def __get_term_frequencies(self, transcript):
        """Generate a dictionary of terms and their number of appearances in a transcript

        Args:
            transcript: String
                Text transcript for which to count term frequency

        Returns:
            Dictionary containing terms in this transcript and their number of appearances
        """
        # Remove stop words (50 most common words) from term frequency counting
        stop_words = stopwords.words("english")
        term_list = [term for term in transcript.split() if term not in stop_words]

        # Generate frequency dictionary for remaining terms
        return collections.Counter(term_list)

    def __insert_document(self, path, transcript, term_frequencies, num_terms):
        """Insert a new document and its metadata into the database

        Args:
            path: String
                Path to this document's original file location (from which transcript was generated)
            transcript: String
                Text transcript generated from original file
            term_frequencies: Dict
                Dictionary containing terms and their number of appearances in the transcript
            num_terms: Int
                Number of unique terms that appear in this transcript
        """
        data = (path, transcript, term_frequencies, num_terms)
        self.__cur.execute("INSERT INTO documents VALUES (?, ?, ?, ?)", data)
        self.__conn.commit()

    def __update_terms(self, document, term_frequencies):
        """Update the database's global state for the terms which appear in a document

        Args:
            document: String
                Unique absolute path representing this document
            term_frequencies: Dict
                Dictionary containing terms and their number of appearances in the document's transcript
        """
        for term, _ in term_frequencies.items():
            # Try to get the term's current DB entry
            result = self.__get_term(term)
            entry = result.fetchone()

            # If an entry already exists (term appears in previously seen documents), update it
            if entry:
                # Fetch the list of documents this term appears in, and add the new document to it
                documents = (json.loads(entry[1]))
                documents.append(document)
                # Update the term with its new list of documents
                self.__update_term(term, documents)

            # If an entry does not exist, create a new one
            else:
                # Insert a term (which only appears in this new document at this point)
                self.__insert_term(term, document)

    def __get_term(self, term):
        """Fetch DB entry for a given term

        Args:
            term: String
                term to search for in the DB

        Returns:
            A query object which can be used to fetch the result of this term
        """
        data = (term,)
        result = self.__cur.execute("SELECT * FROM terms WHERE term=?", data)
        return result

    def __update_term(self, term, documents):
        """Update a term's DB entry with a new inverted index of documents in list form

        Args:
            term: String
                term to update in the DB
            documents: List
                list of documents in which this term appears
        """
        data = (json.dumps(documents), term)
        self.__cur.execute("UPDATE terms SET documents = ? WHERE term = ?", data)
        self.__conn.commit()

    def __insert_term(self, term, document):
        """Insert a new term into the DB with the document it has appeared in

        Args:
            term: String
                term to update in the DB
            document: String
                document in which this term appears
        """
        data = (term, json.dumps([document]))
        self.__cur.execute("INSERT INTO terms VALUES (?, ?)", data)
        self.__conn.commit()

    def exists(self, path):
        """Return whether or not a given source file has already been processed

        Args:
            path: String
                Path to source file being queried

        Returns:
            Boolean true if file has already been processed false otherwise
        """
        if not self.__existing_documents:
            self.__existing_documents = self.__get_documents().fetchall()
        return (path,) in self.__existing_documents

    def __get_documents(self):
        """Get existing documents from the DB

        Returns:
            A query object which can be used to fetch the result of this search
        """
        result = self.__cur.execute("SELECT file FROM documents")
        return result

    def __del__(self):
        """Destructor which closes database connection
        """
        if self.__conn:
            self.__conn.close()
