# VideoCollectionSpeechFinder

The `VideoCollectionSpeechFinder` is built to search an unlabeled collection of videos containing speech for some known search phrase / set of words. For instance, someone has a library of hundreds of videos and would like to identify the one which contains a conversation about ice hockey. Search terms such as "ice", "hockey", "puck", "stick", "score", etc. will likely help identify the most relevant candidates.

## Technical Overview ##
The tool uses the [`OpenAI Whisper Model`](https://github.com/openai/whisper) to perfom speech-to-text on the video collection using the model size of user's choice (larger models are more accurate, run slower and use more memory).
To search the collection of transcripts, various search algorithms can be implemented - the current base implementation uses `TF-IDF` (Term Frequency - Inverse Document Frequency).

Preprocessing can be performed at transcription-time to generate values which are search-independent (for instance, maintaining a reverse-index from terms to their containing documents for `TF-IDF`) to improve search-time performance.

Transcription and preprocessing is handled by the [`video_collection_speech_processing`](#video_collection_speech_processing) Python module, and search is handled by the [`transcript_searcher`](#transcript_seacher) C++ module.

The two modules interact with an intermediate `SQLite` [`database`](#database), which also maintains offline state avoiding the expensive retranscription cost of already processed videos.

## Usage ##
### Dependencies ###

```bash
sudo apt install ffmpeg
sudo apt install nvidia-cuda-toolkit

pip install openai-whisper
pip install nltk
pip install moviepy
```

### `database` ###

To get started, set up the database -
```bash
cd VideoCollectionSpeechFinder/database
python3 setup.py
```

### `video_collection_speech_processing` ###

To transcribe a directory of sources and add its results to the database's collection, use the module's `main` program -
```bash
cd VideoCollectionSpeechFinder/video_collection_speech_processing
python3 main.py <source_directory>
```

### `transcript_searcher` ###

First, build the transcript searcher -
```bash
cd VideoCollectionSpeechFinder/transcript_searcher
mkdir build
git submodule init
git submodule update --recursive
cmake -Bbuild
cd build
make
```

Now, we can run the transcript searcher to search our collection of existing transcripts -
```bash
./bin/main
```



