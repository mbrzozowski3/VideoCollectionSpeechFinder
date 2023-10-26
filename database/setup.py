import argparse
import sqlite3

# Define the CLI for this program
def parse_input():
    parser = argparse.ArgumentParser(
        prog='setup.py',
        usage='%(prog)s [database_path]'
    )
    parser.add_argument('database_path', nargs='?', default='application.db')
    return parser.parse_args()


def main():
    args = parse_input()
    conn = sqlite3.connect(args.database_path)
    cur = conn.cursor()
    cur.execute("""
        CREATE TABLE IF NOT EXISTS documents (
            file varchar(255),
            transcription text,
            termFrequencies text,
            numTerms unsigned integer
        )
    """)
    cur.execute("""
        CREATE TABLE IF NOT EXISTS terms (
            term varchar(255),
            documents text
        )
    """)
    conn.close()


if __name__ == "__main__":
    main()
    