import os
import glob
import openai
import sqlite3
from configparser import ConfigParser
from moviepy.editor import VideoFileClip


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


    # Perform speech to text using an API call to OpenAI
    def speech_to_text(self, audio_path):
        audio_file = open(audio_path, "rb")
        # TODO: This may fail due to rate limits, etc.
        result = openai.Audio.transcribe("whisper-1", audio_file)
        return result.text

    
    # Get existing transcriptions from the DB, but return the cursor before fetching so we aren't moving around data needlessly
    def get_transcriptions(self):
        result = self.cur.execute("SELECT file FROM transcriptions")
        return result


    # Insert a transcription for a given video path into the DB
    def insert_transcription(self, video_path, transcript):
        data = (video_path, transcript)
        self.cur.execute("INSERT INTO transcriptions VALUES (?, ?)", data)
        self.conn.commit()


    # Transcribe all sources in a directory and store in the DB (Don't regenerate if already present)
    def transcribe_sources(self):
    
        # Just returns a cursor so we still need to fetch to get the transcriptions out of DB
        transcriptions = self.get_transcriptions().fetchall()

        # Search for all videos of designated type in the source directory
        for video_path in glob.glob(f"{self.sources_path}/*.{self.source_format}"):

            # Will need absolute paths for uniqueness in DB
            full_video_path = os.path.abspath(video_path)

            # Avoid processing anything already transcripted in the DB
            if (full_video_path,) not in transcriptions:

                # Get audio split from the video
                temp_audio_path = self.create_audio_path(video_path)
                self.extract_audio(video_path, temp_audio_path)
                
                # Use a dummy transcription instead of API call
                if (self.__TRANSCRIPT_OFF__):
                    transcript = "DEBUG TRANSCRIPTION"
                # Perform transcription
                else:
                    transcript = self.speech_to_text(temp_audio_path)

                # Remove the temporary audio file we just needed for the API call
                if os.path.isfile(temp_audio_path):
                    os.remove(temp_audio_path)

                # Add the resulting transcription to DB so we don't need to perform it anymore
                self.insert_transcription(full_video_path, transcript)


    # Clean up resources in destructor
    def __del__(self):
        if (self.conn):
            self.conn.close()


def main():
    print("VideoTranscriber")


if __name__ == "__main__":
    main()
