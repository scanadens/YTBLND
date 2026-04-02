#!/usr/bin/env bash
# clear_db_data.sh — Deletes all rows from every table while preserving schemas.
set -euo pipefail

DB_PATH="${YTBLND_DB_PATH:-./ytblnd-server_data/ytblnd.db}"

if [[ ! -f "$DB_PATH" ]]; then
  echo "Database not found: $DB_PATH"
  exit 1
fi

echo "Clearing all data from: $DB_PATH"

sqlite3 "$DB_PATH" <<'SQL'
PRAGMA foreign_keys = OFF;
DELETE FROM chat_messages;
DELETE FROM chat_room_members;
DELETE FROM chat_rooms;
DELETE FROM blend_participants;
DELETE FROM blends;
DELETE FROM user_watch_later;
DELETE FROM users;
PRAGMA foreign_keys = ON;
SQL

echo "Done. All rows deleted; schemas intact."
