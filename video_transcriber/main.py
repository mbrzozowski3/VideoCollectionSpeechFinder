import argparse
from pathlib import Path
from video_transcriber import VideoTranscriber


# Define the CLI for this program
def parse_input():
    parser = argparse.ArgumentParser(
        prog='main.py',
        usage='%(prog)s source_content_path [--source_format] [--config_file] [--TRANSCRIPT_OFF]'
    )
    parser.add_argument('source_content_path')
    parser.add_argument('--source_format', nargs='?', default='mp4')
    parser.add_argument('--config_file', nargs='?', default='config.json')
    parser.add_argument('--TRANSCRIPT_OFF', dest='TRANSCRIPT_OFF', action='store_true')
    return parser.parse_args()


def main():
    # Parse arguments from command line
    args = parse_input()

    # Add root project dir for use in relative path resolution
    current_dir = Path(__file__)
    project_dir = [p for p in current_dir.parents
                   if p.parts[-1]=='VideoCollectionSpeechFinder'][0]
    args.project_root_path = str(project_dir) + "/"

    # Create a video transcriber and run
    videoTranscriber = VideoTranscriber(args)
    videoTranscriber.transcribe_sources()


if __name__ == "__main__":
    main()
