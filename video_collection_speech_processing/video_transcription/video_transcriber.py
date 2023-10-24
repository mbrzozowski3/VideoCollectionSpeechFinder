import os
import tempfile

from moviepy.editor import VideoFileClip
import whisper

class VideoTranscriber:
    """Performs speech-to-text transcription on video sources.

    Strips audio from video sources and feeds it to an OpenAI Whisper model for transcription.
    """

    def __init__(self, speech_model="base.en"):
        """Initialize instance with the name of the model to be used

        Args:
            speech_model: String
                Model for transcriber to use in speech-to-text
        """
        self.__speech_model = whisper.load_model(speech_model)

    def transcribe_source(self, path):
        """Transcribe source by extracting its audio and feeding it into speech-to-text model

        Args:
            path: String
                Path of file to be transcribed

        Returns:
            Text transcription of the speech detected in the source file
        """
        # Split audio from video into a temporary audio file
        audio_path = tempfile.NamedTemporaryFile().name + ".mp3"
        self.__extract_audio(path, audio_path)

        # Perform speech-to-text
        transcript = self.__speech_to_text(audio_path)

        # Remove the temporary audio file used as speech to text input
        if os.path.isfile(audio_path):
            os.remove(audio_path)

        return transcript

    def __extract_audio(self, video_path, audio_path):
        """
        Demuxes audio from source video file and outputs it to the target audio file

        Args:
            video_path: String
                Path to source video file
            audio_path: String
                Path to target audio file
        """
        clip = VideoFileClip(video_path)
        clip.audio.write_audiofile(audio_path, verbose=False, logger=None)

    def __speech_to_text(self, audio_path):
        """Perform speech-to-text on an audio file using model

        Args:
            audio_path: String
                Path to source audio file

        Returns:
            Text transcription of the speech detected in the source audio file
        """
        result = self.__speech_model.transcribe(audio_path, language="english")
        return result["text"]
