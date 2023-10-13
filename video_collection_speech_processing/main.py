import argparse
import json
from pathlib import Path

from video_collection_speech_processor import VideoCollectionSpeechProcessor


# Define the CLI for this program
def parse_input():
    parser = argparse.ArgumentParser(
        prog='main.py',
        usage='%(prog)s source_path [--config_file] [--source_format] [--search_algorithm] [--speech_model]'
    )
    parser.add_argument('source_path')
    parser.add_argument('--config_file', nargs='?', default='config.json')
    parser.add_argument('--source_format', nargs='?', default='mp4')
    parser.add_argument('--search_algorithm', nargs='?', default='tf-idf')
    parser.add_argument('--speech_model', nargs='?', default='base.en')
    return parser.parse_args()


def main():
    # Parse arguments from command line
    args = parse_input()

    # Determine root project dir for use in relative path resolution
    current_dir = Path(__file__)
    project_dir = [p for p in current_dir.parents
                   if p.parts[-1]=='VideoCollectionSpeechFinder'][0]
    project_root_path = str(project_dir) + "/"

    with open(project_root_path + args.config_file, encoding="utf-8") as config_file:
        config_dict = json.load(config_file)
        db_path = project_root_path + config_dict["Paths"]["database"]

    video_collection_speech_processor = VideoCollectionSpeechProcessor(
        db_path,
        args.speech_model,
        args.search_algorithm
    )
    video_collection_speech_processor.process_sources(args.source_path, args.source_format)


if __name__ == "__main__":
    main()
