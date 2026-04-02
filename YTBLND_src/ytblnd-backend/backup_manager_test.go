package main

import (
	"os"
	"path/filepath"
	"testing"
	"time"
)

func TestDataBackupManager_RestoreOnLoss(t *testing.T) {
	// Create a baseline file, snapshot it, then simulate loss and verify restore.
	root := t.TempDir()
	dataDir := filepath.Join(root, "data")
	if err := os.MkdirAll(dataDir, 0o755); err != nil {
		t.Fatalf("mkdir data dir: %v", err)
	}

	dbPath := filepath.Join(dataDir, "dev.db")
	initial := []byte("sqlite-content")
	if err := os.WriteFile(dbPath, initial, 0o644); err != nil {
		t.Fatalf("write db file: %v", err)
	}

	manager := NewDataBackupManager(dataDir, 48*time.Hour)
	if err := manager.createSnapshotIfNeeded(); err != nil {
		t.Fatalf("create snapshot: %v", err)
	}

	if err := os.Remove(dbPath); err != nil {
		t.Fatalf("remove db file: %v", err)
	}

	if err := manager.EnsureHealthyOrRestore([]string{"dev.db"}); err != nil {
		t.Fatalf("restore from snapshot: %v", err)
	}

	restored, err := os.ReadFile(dbPath)
	if err != nil {
		t.Fatalf("read restored db file: %v", err)
	}
	if string(restored) != string(initial) {
		t.Fatalf("restored content mismatch: got=%q want=%q", restored, initial)
	}
}

func TestDataBackupManager_NoBackupFailsRestore(t *testing.T) {
	// Without snapshots, loss detection should return a clear failure.
	root := t.TempDir()
	dataDir := filepath.Join(root, "data")
	manager := NewDataBackupManager(dataDir, 48*time.Hour)

	if err := manager.EnsureHealthyOrRestore([]string{"dev.db"}); err == nil {
		t.Fatal("expected error when data loss is detected and no backup exists")
	}
}
