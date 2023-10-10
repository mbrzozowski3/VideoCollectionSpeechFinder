import sqlite3

def main():
    conn = sqlite3.connect("application.db")
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
            documents text,
            globalFrequency unsigned integer
        )
    """)

    conn.close()


if __name__ == "__main__":
    main()
    