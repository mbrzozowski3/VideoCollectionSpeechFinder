import sqlite3

def main():
    conn = sqlite3.connect("application.db")
    cur = conn.cursor()
    cur.execute("""
        DROP TABLE documents;
    """)
    cur.execute("""
        DROP TABLE terms;
    """)
    conn.close()


if __name__ == "__main__":
    main()
    