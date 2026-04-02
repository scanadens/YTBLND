package main

import (
	"database/sql"
	"errors"
	"fmt"
	"log"
	"os"
	"path/filepath"

	_ "modernc.org/sqlite"
)

// MigrateLegacyDevDB moves missing data from a legacy dev.db into target DB.
func MigrateLegacyDevDB(legacyPath, targetPath string) error {
	legacyPath = filepath.Clean(legacyPath)
	targetPath = filepath.Clean(targetPath)

	if legacyPath == targetPath {
		return nil
	}

	if _, err := os.Stat(legacyPath); errors.Is(err, os.ErrNotExist) {
		// Nothing to migrate when legacy DB does not exist.
		return nil
	} else if err != nil {
		return fmt.Errorf("stat legacy db: %w", err)
	}

	if _, err := os.Stat(targetPath); errors.Is(err, os.ErrNotExist) {
		// Fast path: if target is absent, copy the legacy DB as-is.
		if err := copyFile(legacyPath, targetPath); err != nil {
			return fmt.Errorf("bootstrap target db from legacy: %w", err)
		}
		log.Printf("migrated legacy db by copying %s -> %s", legacyPath, targetPath)
		return nil
	} else if err != nil {
		return fmt.Errorf("stat target db: %w", err)
	}

	db, err := sql.Open("sqlite", targetPath)
	if err != nil {
		return fmt.Errorf("open target db: %w", err)
	}
	defer db.Close()

	if err := ensureMigrationSchema(db); err != nil {
		return err
	}

	// Transaction keeps attach/select/insert operations all-or-nothing.
	tx, err := db.Begin()
	if err != nil {
		return fmt.Errorf("begin migration tx: %w", err)
	}
	defer tx.Rollback()

	if _, err := tx.Exec("ATTACH DATABASE ? AS legacy", legacyPath); err != nil {
		return fmt.Errorf("attach legacy db: %w", err)
	}
	defer tx.Exec("DETACH DATABASE legacy")

	stmts := []struct {
		table string
		sql   string
	}{
		{
			table: "users",
			sql: `INSERT OR IGNORE INTO users (user_id, username, email, password)
				  SELECT user_id, username, email, password FROM legacy.users;`,
		},
		{
			table: "user_watch_later",
			sql: `INSERT OR IGNORE INTO user_watch_later (
					user_id, video_id, title, channel_id, channel_name, thumbnail_url, channel_logo_url, position
				  )
				  SELECT user_id, video_id, title, channel_id, channel_name, thumbnail_url, channel_logo_url, position
				  FROM legacy.user_watch_later;`,
		},
		{
			table: "blends",
			sql: `INSERT OR IGNORE INTO blends (blend_id, creator_id, algorithm, created_at)
				  SELECT blend_id, creator_id, algorithm, created_at FROM legacy.blends;`,
		},
		{
			table: "blend_participants",
			sql: `INSERT OR IGNORE INTO blend_participants (blend_id, user_id)
				  SELECT blend_id, user_id FROM legacy.blend_participants;`,
		},
		{
			table: "chat_rooms",
			sql: `INSERT OR IGNORE INTO chat_rooms (chat_room_id, blend_id, created_at)
				  SELECT chat_room_id, blend_id, created_at FROM legacy.chat_rooms;`,
		},
		{
			table: "chat_room_members",
			sql: `INSERT OR IGNORE INTO chat_room_members (chat_room_id, user_id)
				  SELECT chat_room_id, user_id FROM legacy.chat_room_members;`,
		},
		{
			table: "chat_messages",
			// chat_messages uses surrogate PK, so dedupe by natural message fields.
			sql: `INSERT INTO chat_messages (chat_room_id, sender_id, content, sent_at)
				  SELECT l.chat_room_id, l.sender_id, l.content, l.sent_at
				  FROM legacy.chat_messages l
				  WHERE NOT EXISTS (
					  SELECT 1
					  FROM chat_messages t
					  WHERE t.chat_room_id = l.chat_room_id
						AND t.sender_id = l.sender_id
						AND t.content = l.content
						AND t.sent_at = l.sent_at
				  );`,
		},
	}

	for _, stmt := range stmts {
		exists, err := legacyTableExists(tx, stmt.table)
		if err != nil {
			return err
		}
		if !exists {
			// Legacy DB may predate newer tables; skip safely.
			continue
		}

		// Merge only missing rows to preserve target DB as source of truth.
		if _, err := tx.Exec(stmt.sql); err != nil {
			return fmt.Errorf("migrate table %s: %w", stmt.table, err)
		}
	}

	if err := tx.Commit(); err != nil {
		return fmt.Errorf("commit migration tx: %w", err)
	}

	log.Printf("migrated missing legacy data from %s into %s", legacyPath, targetPath)
	return nil
}

func ensureMigrationSchema(db *sql.DB) error {
	// Schema mirrors the runtime SQLite manager so merge statements are valid.
	const schema = `
CREATE TABLE IF NOT EXISTS users (
	user_id TEXT PRIMARY KEY,
	username TEXT NOT NULL,
	email TEXT,
	password TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS user_watch_later (
	user_id TEXT NOT NULL,
	video_id TEXT NOT NULL,
	title TEXT,
	channel_id TEXT,
	channel_name TEXT,
	thumbnail_url TEXT,
	channel_logo_url TEXT,
	position INTEGER NOT NULL DEFAULT 0,
	PRIMARY KEY (user_id, video_id)
);

CREATE TABLE IF NOT EXISTS blends (
	blend_id TEXT PRIMARY KEY,
	creator_id TEXT NOT NULL,
	algorithm TEXT NOT NULL,
	created_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS blend_participants (
	blend_id TEXT NOT NULL,
	user_id TEXT NOT NULL,
	PRIMARY KEY (blend_id, user_id)
);

CREATE TABLE IF NOT EXISTS chat_rooms (
	chat_room_id TEXT PRIMARY KEY,
	blend_id TEXT NOT NULL UNIQUE,
	created_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS chat_room_members (
	chat_room_id TEXT NOT NULL,
	user_id TEXT NOT NULL,
	PRIMARY KEY (chat_room_id, user_id)
);

CREATE TABLE IF NOT EXISTS chat_messages (
	message_id INTEGER PRIMARY KEY AUTOINCREMENT,
	chat_room_id TEXT NOT NULL,
	sender_id TEXT NOT NULL,
	content TEXT NOT NULL,
	sent_at TEXT NOT NULL
);
`

	if _, err := db.Exec(schema); err != nil {
		return fmt.Errorf("ensure target migration schema: %w", err)
	}
	return nil
}

func legacyTableExists(tx *sql.Tx, tableName string) (bool, error) {
	const query = `SELECT COUNT(1) FROM legacy.sqlite_master WHERE type = 'table' AND name = ?;`
	var count int
	if err := tx.QueryRow(query, tableName).Scan(&count); err != nil {
		return false, fmt.Errorf("check legacy table %s: %w", tableName, err)
	}
	// Older legacy DBs can legitimately miss newer tables.
	return count > 0, nil
}
