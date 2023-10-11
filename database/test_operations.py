import os
import sqlite3

def main():
    conn = sqlite3.connect("application.db")
    cur = conn.cursor()
    data = (os.getcwd(), "this is a test of a transcription", "{\"this\": 1, \"is\": 1 \"a\": 2, \"test\": 1, \"of\": 1, \"transcription\": 1}", 6)
    cur.execute("INSERT INTO documents VALUES (?, ?, ?, ?)", data)
    conn.commit()
    result = cur.execute("SELECT * FROM documents")
    for entry in result.fetchall():
        print(entry)
    cur.execute("DELETE FROM documents")
    conn.commit()
    data = ("term", "[\"path/document1\", \"other/path/document2\"]")
    cur.execute("INSERT INTO terms VALUES (?, ?)", data)
    conn.commit()
    result = cur.execute("SELECT * FROM terms")
    for entry in result.fetchall():
        print(entry)
    cur.execute("DELETE FROM terms")
    conn.commit()

    conn.close()


if __name__ == "__main__":
    main()
