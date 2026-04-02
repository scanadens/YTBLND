package main

import (
	"database/sql"
	"path/filepath"
	"testing"

	_ "modernc.org/sqlite"
)

func TestMigrateLegacyDevDB_CopiesWhenTargetMissing(t *testing.T) {
	// If ytblnd.db does not exist, migration should clone dev.db directly.
	root := t.TempDir()
	legacyPath := filepath.Join(root, "dev.db")
	targetPath := filepath.Join(root, "ytblnd.db")

	legacyDB := openSQLiteForTest(t, legacyPath)
	seedSchema(t, legacyDB)
	_, err := legacyDB.Exec(`INSERT INTO users (user_id, username, email, password) VALUES ('u1', 'legacy', 'legacy@example.com', 'pw');`)
	if err != nil {
		t.Fatalf("insert legacy user: %v", err)
	}
	legacyDB.Close()

	if err := MigrateLegacyDevDB(legacyPath, targetPath); err != nil {
		t.Fatalf("migrate legacy db: %v", err)
	}

	targetDB := openSQLiteForTest(t, targetPath)
	defer targetDB.Close()
	count := countUsers(t, targetDB)
	if count != 1 {
		t.Fatalf("unexpected user count after copy migration: got=%d want=1", count)
	}
}

func TestMigrateLegacyDevDB_MergesMissingRows(t *testing.T) {
	// If both DBs exist, migration should add rows missing from target only.
	root := t.TempDir()
	legacyPath := filepath.Join(root, "dev.db")
	targetPath := filepath.Join(root, "ytblnd.db")

	legacyDB := openSQLiteForTest(t, legacyPath)
	seedSchema(t, legacyDB)
	_, err := legacyDB.Exec(`
		INSERT INTO users (user_id, username, email, password) VALUES ('u1', 'legacy-1', 'u1@example.com', 'pw');
		INSERT INTO users (user_id, username, email, password) VALUES ('u2', 'legacy-2', 'u2@example.com', 'pw');
	`)
	if err != nil {
		t.Fatalf("seed legacy users: %v", err)
	}
	legacyDB.Close()

	targetDB := openSQLiteForTest(t, targetPath)
	seedSchema(t, targetDB)
	_, err = targetDB.Exec(`INSERT INTO users (user_id, username, email, password) VALUES ('u2', 'target-2', 'u2@target.com', 'pw');`)
	if err != nil {
		t.Fatalf("seed target user: %v", err)
	}
	targetDB.Close()

	if err := MigrateLegacyDevDB(legacyPath, targetPath); err != nil {
		t.Fatalf("migrate legacy db: %v", err)
	}

	mergedDB := openSQLiteForTest(t, targetPath)
	defer mergedDB.Close()
	count := countUsers(t, mergedDB)
	if count != 2 {
		t.Fatalf("unexpected user count after merge migration: got=%d want=2", count)
	}
}

func openSQLiteForTest(t *testing.T, path string) *sql.DB {
	t.Helper()
	db, err := sql.Open("sqlite", path)
	if err != nil {
		t.Fatalf("open sqlite: %v", err)
	}
	return db
}

func seedSchema(t *testing.T, db *sql.DB) {
	t.Helper()
	if err := ensureMigrationSchema(db); err != nil {
		t.Fatalf("ensure schema: %v", err)
	}
}

func countUsers(t *testing.T, db *sql.DB) int {
	t.Helper()
	var count int
	if err := db.QueryRow(`SELECT COUNT(1) FROM users;`).Scan(&count); err != nil {
		t.Fatalf("count users: %v", err)
	}
	return count
}
