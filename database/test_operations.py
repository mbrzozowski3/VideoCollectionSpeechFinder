import argparse
import sqlite3

# Define the CLI for this program
def parse_input():
    parser = argparse.ArgumentParser(
        prog='test_operations.py',
        usage='%(prog)s [database_path]'
    )
    parser.add_argument('database_path', nargs='?', default='application.db')
    return parser.parse_args()

def main():
    args = parse_input()
    conn = sqlite3.connect(args.database_path)

    cur = conn.cursor()
    result = cur.execute("SELECT * FROM documents")
    for entry in result.fetchall():
        print(entry)
    result = cur.execute("SELECT * FROM terms")
    for entry in result.fetchall():
        print(entry)

    conn.close()


if __name__ == "__main__":
    main()
