import sqlite3

def main():
    conn = sqlite3.connect("application.db")
    cur = conn.cursor()
    cur.execute("""
        CREATE TABLE IF NOT EXISTS transcriptions (
            file varchar(255),
            transcription text
        )
    """)
    conn.close()


if __name__ == "__main__":
    main()
    