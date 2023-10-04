import os
import sqlite3

def main():
    conn = sqlite3.connect("application.db")
    cur = conn.cursor()
    data = (os.getcwd(), "This is an example of a transcription that goes on and on and on and ends.")
    cur.execute("INSERT INTO transcriptions VALUES (?, ?)", data)
    conn.commit()
    result = cur.execute("SELECT * FROM transcriptions")
    for entry in result.fetchall():
        print(entry)
    cur.execute("DELETE FROM transcriptions")
    conn.commit()
    conn.close()


if __name__ == "__main__":
    main()
