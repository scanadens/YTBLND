package database_layer

import (
	"database/sql"
	"errors"
	"fmt"
	"strings"
	"time"

	_ "modernc.org/sqlite"
)

type SqliteDatabaseManager struct {
	// db is the shared SQLite handle used by all manager methods.
	db *sql.DB
}

// NewSqliteDataManager opens the SQLite connection and eagerly creates all
// required tables so the manager is ready for use immediately.
func NewSqliteDataManager(path string) (*SqliteDatabaseManager, error) {
	db, err := sql.Open("sqlite", path)
	if err != nil {
		return nil, err
	}

	mgr := &SqliteDatabaseManager{db: db}
	if err := mgr.initSchema(); err != nil {
		return nil, err
	}

	return mgr, nil
}

// initSchema applies all table definitions using CREATE TABLE IF NOT EXISTS so
// existing user data is preserved across restarts and redeploys.
func (m *SqliteDatabaseManager) initSchema() error {
	schema := `
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
	_, err := m.db.Exec(schema)
	if err != nil {
		return err
	}

	// Backfill schema for existing DBs that were created before metadata columns existed.
	if err := m.ensureUserWatchLaterMetadataColumns(); err != nil {
		return err
	}

	return nil
}

func (m *SqliteDatabaseManager) ensureUserWatchLaterMetadataColumns() error {
	type columnDef struct {
		name       string
		sqlTypeDef string
	}

	columns := []columnDef{
		{name: "channel_id", sqlTypeDef: "TEXT"},
		{name: "channel_name", sqlTypeDef: "TEXT"},
		{name: "thumbnail_url", sqlTypeDef: "TEXT"},
		{name: "channel_logo_url", sqlTypeDef: "TEXT"},
	}

	existing := make(map[string]bool)
	rows, err := m.db.Query("PRAGMA table_info(user_watch_later);")
	if err != nil {
		return fmt.Errorf("schema migration table_info(user_watch_later): %w", err)
	}
	defer rows.Close()

	for rows.Next() {
		var cid int
		var name, colType string
		var notNull, pk int
		var dflt sql.NullString
		if err := rows.Scan(&cid, &name, &colType, &notNull, &dflt, &pk); err != nil {
			return fmt.Errorf("schema migration scan table_info(user_watch_later): %w", err)
		}
		existing[strings.ToLower(name)] = true
	}
	if err := rows.Err(); err != nil {
		return fmt.Errorf("schema migration rows table_info(user_watch_later): %w", err)
	}

	for _, col := range columns {
		if existing[col.name] {
			continue
		}
		stmt := fmt.Sprintf("ALTER TABLE user_watch_later ADD COLUMN %s %s;", col.name, col.sqlTypeDef)
		if _, err := m.db.Exec(stmt); err != nil {
			return fmt.Errorf("schema migration add column %s: %w", col.name, err)
		}
	}

	return nil
}

// CreateUser inserts a new user row; SQLite returns a constraint error when
// user_id already exists, which is surfaced via the wrapped error.
func (m *SqliteDatabaseManager) CreateUser(user User) error {
	if m.db == nil {
		return errors.New("database is not initialized")
	}

	const query = `
        INSERT INTO users (user_id, username, email, password)
        VALUES (?, ?, ?, ?);
    `

	_, err := m.db.Exec(query, user.GetUserID(), user.GetUsername(), user.GetEmail(), user.GetPassword())
	if err != nil {
		return fmt.Errorf("create user: %w", err)
	}

	return nil
}

// FindUserByID returns nil,nil when the user does not exist so callers can
// distinguish "not found" from a real database failure.
func (m *SqliteDatabaseManager) FindUserByID(id string) (*User, error) {
	if m.db == nil {
		return nil, errors.New("database is not initialized")
	}

	const query = `
        SELECT user_id, username, email, password
        FROM users
        WHERE user_id = ?;
    `

	var userID, username, password string
	var email sql.NullString
	err := m.db.QueryRow(query, id).Scan(&userID, &username, &email, &password)
	if err != nil {
		if errors.Is(err, sql.ErrNoRows) {
			return nil, nil
		}
		return nil, fmt.Errorf("find user by id: %w", err)
	}

	user := NewUser(userID, username, email.String, password)
	return &user, nil
}

// ValidatePassword delegates user lookup to FindUserByID and only compares
// plaintext values; replace with hashed password verification in production.
func (m *SqliteDatabaseManager) ValidatePassword(id, password string) bool {
	user, err := m.FindUserByID(id)
	if err != nil || user == nil {
		return false
	}

	return user.GetPassword() == password
}

// SaveWatchLater replaces the full watch-later list for a user in one
// transaction so partial updates are avoided on failures.
func (m *SqliteDatabaseManager) SaveWatchLater(userID string, videos []Video) error {
	if m.db == nil {
		return errors.New("database is not initialized")
	}

	// Start a transaction so delete+insert behaves atomically.
	tx, err := m.db.Begin()
	if err != nil {
		return fmt.Errorf("save watch later begin tx: %w", err)
	}
	defer tx.Rollback()

	// Clear existing rows first to mirror C++ "re-upload replaces all" behavior.
	if _, err := tx.Exec(`DELETE FROM user_watch_later WHERE user_id = ?;`, userID); err != nil {
		return fmt.Errorf("save watch later delete existing: %w", err)
	}

	// Keep insertion order via position so reads can reconstruct playlist order.
	stmt, err := tx.Prepare(`
		INSERT OR REPLACE INTO user_watch_later (
			user_id, video_id, title, channel_id, channel_name, thumbnail_url, channel_logo_url, position
		)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    `)
	if err != nil {
		return fmt.Errorf("save watch later prepare insert: %w", err)
	}
	defer stmt.Close()

	// Reinsert each video at its current index.
	for i, video := range videos {
		if _, err := stmt.Exec(
			userID,
			video.GetVideoID(),
			video.GetTitle(),
			video.GetChannelID(),
			video.GetChannelName(),
			video.GetThumbnailURL(),
			video.GetChannelLogoURL(),
			i,
		); err != nil {
			return fmt.Errorf("save watch later insert: %w", err)
		}
	}

	// Commit finalizes both delete and inserts together.
	if err := tx.Commit(); err != nil {
		return fmt.Errorf("save watch later commit: %w", err)
	}

	return nil
}

// LoadWatchLater returns videos in saved playlist order; only the subset of
// fields persisted in user_watch_later is populated.
func (m *SqliteDatabaseManager) LoadWatchLater(userID string) ([]Video, error) {
	if m.db == nil {
		return nil, errors.New("database is not initialized")
	}

	const query = `
		SELECT video_id, title, channel_id, channel_name, thumbnail_url, channel_logo_url
        FROM user_watch_later
        WHERE user_id = ?
        ORDER BY position ASC;
    `

	rows, err := m.db.Query(query, userID)
	if err != nil {
		return nil, fmt.Errorf("load watch later query: %w", err)
	}
	defer rows.Close()

	videos := make([]Video, 0)
	for rows.Next() {
		var videoID string
		var title sql.NullString
		var channelID sql.NullString
		var channelName sql.NullString
		var thumbnailURL sql.NullString
		var channelLogoURL sql.NullString
		if err := rows.Scan(&videoID, &title, &channelID, &channelName, &thumbnailURL, &channelLogoURL); err != nil {
			return nil, fmt.Errorf("load watch later scan: %w", err)
		}
		video := NewVideo(videoID, title.String, channelID.String, channelName.String, thumbnailURL.String, channelLogoURL.String, 0, nil)
		videos = append(videos, video)
	}

	if err := rows.Err(); err != nil {
		return nil, fmt.Errorf("load watch later rows: %w", err)
	}

	return videos, nil
}

// SaveBlend upserts blend metadata and replaces participants in one
// transaction to prevent stale membership rows.
func (m *SqliteDatabaseManager) SaveBlend(blendID, creatorID, algorithm string, participants []string) error {
	if m.db == nil {
		return errors.New("database is not initialized")
	}

	// Transaction groups blend metadata and participant set updates.
	tx, err := m.db.Begin()
	if err != nil {
		return fmt.Errorf("save blend begin tx: %w", err)
	}
	defer tx.Rollback()

	// Persist UTC timestamp in an ISO-like format for stable lexical ordering.
	createdAt := time.Now().UTC().Format("2006-01-02T15:04:05")
	if _, err := tx.Exec(`
        INSERT OR REPLACE INTO blends (blend_id, creator_id, algorithm, created_at)
        VALUES (?, ?, ?, ?);
    `, blendID, creatorID, algorithm, createdAt); err != nil {
		return fmt.Errorf("save blend upsert blend: %w", err)
	}

	// Full replacement keeps DB membership equal to the passed participant list.
	if _, err := tx.Exec(`DELETE FROM blend_participants WHERE blend_id = ?;`, blendID); err != nil {
		return fmt.Errorf("save blend delete participants: %w", err)
	}

	// INSERT OR IGNORE protects against duplicate IDs in the input list.
	stmt, err := tx.Prepare(`
        INSERT OR IGNORE INTO blend_participants (blend_id, user_id)
        VALUES (?, ?);
    `)
	if err != nil {
		return fmt.Errorf("save blend prepare participants: %w", err)
	}
	defer stmt.Close()

	for _, userID := range participants {
		if _, err := stmt.Exec(blendID, userID); err != nil {
			return fmt.Errorf("save blend insert participant: %w", err)
		}
	}

	// Keep one chat room attached to each blend.
	if _, err := tx.Exec(`
        INSERT OR IGNORE INTO chat_rooms (chat_room_id, blend_id, created_at)
        VALUES (?, ?, ?);
    `, blendID, blendID, createdAt); err != nil {
		return fmt.Errorf("save blend upsert chat room: %w", err)
	}

	if _, err := tx.Exec(`DELETE FROM chat_room_members WHERE chat_room_id = ?;`, blendID); err != nil {
		return fmt.Errorf("save blend delete chat room members: %w", err)
	}

	membersStmt, err := tx.Prepare(`
        INSERT OR IGNORE INTO chat_room_members (chat_room_id, user_id)
        VALUES (?, ?);
    `)
	if err != nil {
		return fmt.Errorf("save blend prepare chat room members: %w", err)
	}
	defer membersStmt.Close()

	for _, userID := range participants {
		if _, err := membersStmt.Exec(blendID, userID); err != nil {
			return fmt.Errorf("save blend insert chat room member: %w", err)
		}
	}

	if err := tx.Commit(); err != nil {
		return fmt.Errorf("save blend commit: %w", err)
	}

	return nil
}

// SaveBlendFromModel is a model-first helper that accepts a Blend value,
// extracts participant IDs through model accessors, and delegates persistence
// to SaveBlend. This lets callers stay at domain-model level when possible.
func (m *SqliteDatabaseManager) SaveBlendFromModel(creatorID string, blend Blend) error {
	participants := blend.GetParticipants()
	participantIDs := make([]string, 0, len(participants))
	for _, p := range participants {
		participantIDs = append(participantIDs, p.GetUserID())
	}

	return m.SaveBlend(
		blend.GetBlendID(),
		creatorID,
		blend.GetAlgorithmUsed(),
		participantIDs,
	)
}

// FindBlendForUser returns the most recently created blend that includes the
// given user; empty string means no blend found.
func (m *SqliteDatabaseManager) FindBlendForUser(userID string) (string, error) {
	if m.db == nil {
		return "", errors.New("database is not initialized")
	}

	const query = `
        SELECT bp.blend_id
        FROM blend_participants bp
        INNER JOIN blends b ON bp.blend_id = b.blend_id
        WHERE bp.user_id = ?
        ORDER BY b.created_at DESC
        LIMIT 1;
    `

	var blendID string
	err := m.db.QueryRow(query, userID).Scan(&blendID)
	if err != nil {
		// No rows is a valid state: user has not participated in a blend yet.
		if errors.Is(err, sql.ErrNoRows) {
			return "", nil
		}
		return "", fmt.Errorf("find blend for user: %w", err)
	}

	return blendID, nil
}

// LoadBlendParticipants returns all user IDs linked to a blend.
func (m *SqliteDatabaseManager) LoadBlendParticipants(blendID string) ([]string, error) {
	if m.db == nil {
		return nil, errors.New("database is not initialized")
	}

	const query = `
        SELECT user_id
        FROM blend_participants
        WHERE blend_id = ?;
    `

	rows, err := m.db.Query(query, blendID)
	if err != nil {
		return nil, fmt.Errorf("load blend participants query: %w", err)
	}
	defer rows.Close()

	participants := make([]string, 0)
	for rows.Next() {
		var userID string
		if err := rows.Scan(&userID); err != nil {
			return nil, fmt.Errorf("load blend participants scan: %w", err)
		}
		participants = append(participants, userID)
	}

	if err := rows.Err(); err != nil {
		return nil, fmt.Errorf("load blend participants rows: %w", err)
	}

	return participants, nil
}

// GetChatRoomForBlend returns the chat room ID attached to a blend.
func (m *SqliteDatabaseManager) GetChatRoomForBlend(blendID string) (string, error) {
	if m.db == nil {
		return "", errors.New("database is not initialized")
	}

	const query = `
        SELECT chat_room_id
        FROM chat_rooms
        WHERE blend_id = ?
        LIMIT 1;
    `

	var chatRoomID string
	err := m.db.QueryRow(query, blendID).Scan(&chatRoomID)
	if err != nil {
		if errors.Is(err, sql.ErrNoRows) {
			return "", nil
		}
		return "", fmt.Errorf("get chat room for blend: %w", err)
	}

	return chatRoomID, nil
}

// IsChatRoomMember reports whether a user belongs to a chat room.
func (m *SqliteDatabaseManager) IsChatRoomMember(chatRoomID, userID string) (bool, error) {
	if m.db == nil {
		return false, errors.New("database is not initialized")
	}

	const query = `
        SELECT 1
        FROM chat_room_members
        WHERE chat_room_id = ? AND user_id = ?
        LIMIT 1;
    `

	var exists int
	err := m.db.QueryRow(query, chatRoomID, userID).Scan(&exists)
	if err != nil {
		if errors.Is(err, sql.ErrNoRows) {
			return false, nil
		}
		return false, fmt.Errorf("is chat room member: %w", err)
	}

	return true, nil
}

// LoadChatRoomMembers returns all user IDs assigned to a chat room.
func (m *SqliteDatabaseManager) LoadChatRoomMembers(chatRoomID string) ([]string, error) {
	if m.db == nil {
		return nil, errors.New("database is not initialized")
	}

	const query = `
        SELECT user_id
        FROM chat_room_members
        WHERE chat_room_id = ?;
    `

	rows, err := m.db.Query(query, chatRoomID)
	if err != nil {
		return nil, fmt.Errorf("load chat room members query: %w", err)
	}
	defer rows.Close()

	members := make([]string, 0)
	for rows.Next() {
		var userID string
		if err := rows.Scan(&userID); err != nil {
			return nil, fmt.Errorf("load chat room members scan: %w", err)
		}
		members = append(members, userID)
	}

	if err := rows.Err(); err != nil {
		return nil, fmt.Errorf("load chat room members rows: %w", err)
	}

	return members, nil
}

// SaveChatMessage appends a chat message to room history for later replay.
func (m *SqliteDatabaseManager) SaveChatMessage(chatRoomID string, message ChatMessageRecord) error {
	if m.db == nil {
		return errors.New("database is not initialized")
	}

	const query = `
        INSERT INTO chat_messages (chat_room_id, sender_id, content, sent_at)
        VALUES (?, ?, ?, ?);
    `

	_, err := m.db.Exec(query, chatRoomID, message.GetSenderID(), message.GetContent(), message.GetSentAt())
	if err != nil {
		return fmt.Errorf("save chat message: %w", err)
	}

	return nil
}

// LoadChatMessages returns stored room history ordered from oldest to newest.
func (m *SqliteDatabaseManager) LoadChatMessages(chatRoomID string, limit int) ([]ChatMessageRecord, error) {
	if m.db == nil {
		return nil, errors.New("database is not initialized")
	}
	if limit <= 0 {
		limit = 100
	}

	const query = `
        SELECT sender_id, content, sent_at
        FROM (
            SELECT sender_id, content, sent_at
            FROM chat_messages
            WHERE chat_room_id = ?
            ORDER BY sent_at DESC, message_id DESC
            LIMIT ?
        ) recent
        ORDER BY sent_at ASC;
    `

	rows, err := m.db.Query(query, chatRoomID, limit)
	if err != nil {
		return nil, fmt.Errorf("load chat messages query: %w", err)
	}
	defer rows.Close()

	messages := make([]ChatMessageRecord, 0)
	for rows.Next() {
		var senderID string
		var content string
		var sentAt string
		if err := rows.Scan(&senderID, &content, &sentAt); err != nil {
			return nil, fmt.Errorf("load chat messages scan: %w", err)
		}
		messages = append(messages, NewChatMessageRecord(chatRoomID, senderID, content, sentAt))
	}

	if err := rows.Err(); err != nil {
		return nil, fmt.Errorf("load chat messages rows: %w", err)
	}

	return messages, nil
}
