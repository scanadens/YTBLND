package main

import (
	"context"
	"errors"
	"fmt"
	"io"
	"io/fs"
	"log"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"
)

// DataBackupManager stores rolling snapshots for a data directory.
type DataBackupManager struct {
	dataDir    string
	backupRoot string
	interval   time.Duration
}

func NewDataBackupManager(dataDir string, interval time.Duration) *DataBackupManager {
	cleanDataDir := filepath.Clean(dataDir)
	return &DataBackupManager{
		dataDir: cleanDataDir,
		// Keep backups beside the data directory for predictable ops paths.
		backupRoot: filepath.Join(filepath.Dir(cleanDataDir), filepath.Base(cleanDataDir)+"_backups"),
		interval:   interval,
	}
}

func collectRequiredFilesForDataDir(dataDir string, absolutePaths ...string) []string {
	required := make([]string, 0, len(absolutePaths))
	for _, absolutePath := range absolutePaths {
		// Ignore paths outside dataDir so checks do not trip on unrelated files.
		relPath, err := filepath.Rel(dataDir, absolutePath)
		if err != nil {
			continue
		}
		if strings.HasPrefix(relPath, "..") {
			continue
		}
		relPath = filepath.Clean(relPath)
		if relPath == "." {
			continue
		}
		required = append(required, relPath)
	}
	return required
}

func (m *DataBackupManager) Run(ctx context.Context) {
	// Snapshot at startup so a fresh restore point exists before long uptime.
	if err := m.createSnapshotIfNeeded(); err != nil {
		log.Printf("backup manager initial snapshot failed: %v", err)
	}

	ticker := time.NewTicker(m.interval)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			if err := m.createSnapshotIfNeeded(); err != nil {
				log.Printf("backup manager snapshot failed: %v", err)
			}
		}
	}
}

func (m *DataBackupManager) EnsureHealthyOrRestore(requiredFiles []string) error {
	lost, err := m.isDataLossDetected(requiredFiles)
	if err != nil {
		return err
	}
	if !lost {
		return nil
	}

	// Restore from the newest snapshot when critical files are missing/empty.
	latest, err := m.latestSnapshotPath()
	if err != nil {
		return fmt.Errorf("data loss detected but no backup available: %w", err)
	}

	if err := m.restoreFromSnapshot(latest); err != nil {
		return fmt.Errorf("restore from snapshot %s failed: %w", latest, err)
	}

	log.Printf("data loss detected in %s; restored from %s", m.dataDir, latest)
	return nil
}

func (m *DataBackupManager) isDataLossDetected(requiredFiles []string) (bool, error) {
	if len(requiredFiles) == 0 {
		return false, nil
	}

	for _, relPath := range requiredFiles {
		absPath := filepath.Join(m.dataDir, relPath)
		info, err := os.Stat(absPath)
		// Missing required file is treated as data loss.
		if errors.Is(err, os.ErrNotExist) {
			return true, nil
		}
		if err != nil {
			return false, fmt.Errorf("stat required file %s: %w", absPath, err)
		}
		if info.IsDir() {
			continue
		}
		// Empty required file is also treated as loss/corruption signal.
		if info.Size() == 0 {
			return true, nil
		}
	}

	return false, nil
}

func (m *DataBackupManager) createSnapshotIfNeeded() error {
	lastSnapshot, err := m.latestSnapshotPath()
	if err == nil {
		lastSnapshotInfo, statErr := os.Stat(lastSnapshot)
		// Skip redundant copies until the configured interval has elapsed.
		if statErr == nil && time.Since(lastSnapshotInfo.ModTime()) < m.interval {
			return nil
		}
	}

	if err := os.MkdirAll(m.backupRoot, 0o755); err != nil {
		return fmt.Errorf("create backup root: %w", err)
	}

	if err := os.MkdirAll(m.dataDir, 0o755); err != nil {
		return fmt.Errorf("create data dir: %w", err)
	}

	snapshotName := time.Now().UTC().Format("20060102-150405")
	snapshotPath := filepath.Join(m.backupRoot, "snapshot-"+snapshotName)
	if err := copyDirectory(m.dataDir, snapshotPath); err != nil {
		return fmt.Errorf("copy data dir for snapshot: %w", err)
	}

	log.Printf("created data snapshot: %s", snapshotPath)
	return nil
}

func (m *DataBackupManager) latestSnapshotPath() (string, error) {
	entries, err := os.ReadDir(m.backupRoot)
	if err != nil {
		if errors.Is(err, os.ErrNotExist) {
			return "", os.ErrNotExist
		}
		return "", fmt.Errorf("read backup root: %w", err)
	}

	type candidate struct {
		path    string
		modTime time.Time
	}

	candidates := make([]candidate, 0, len(entries))
	for _, entry := range entries {
		if !entry.IsDir() {
			continue
		}
		info, infoErr := entry.Info()
		if infoErr != nil {
			continue
		}
		candidates = append(candidates, candidate{
			path:    filepath.Join(m.backupRoot, entry.Name()),
			modTime: info.ModTime(),
		})
	}

	if len(candidates) == 0 {
		return "", os.ErrNotExist
	}

	// Pick newest snapshot by mtime; snapshot name already contains UTC timestamp.
	sort.Slice(candidates, func(i, j int) bool {
		return candidates[i].modTime.After(candidates[j].modTime)
	})

	return candidates[0].path, nil
}

func (m *DataBackupManager) restoreFromSnapshot(snapshotPath string) error {
	if _, err := os.Stat(snapshotPath); err != nil {
		return fmt.Errorf("snapshot does not exist: %w", err)
	}

	tmpRestorePath := m.dataDir + ".restore-tmp"
	_ = os.RemoveAll(tmpRestorePath)

	// Restore into a temp directory first, then swap atomically.
	if err := copyDirectory(snapshotPath, tmpRestorePath); err != nil {
		return fmt.Errorf("copy snapshot to temp: %w", err)
	}

	if err := os.RemoveAll(m.dataDir); err != nil {
		return fmt.Errorf("remove damaged data directory: %w", err)
	}

	if err := os.Rename(tmpRestorePath, m.dataDir); err != nil {
		return fmt.Errorf("swap restored data directory: %w", err)
	}

	return nil
}

func copyDirectory(srcDir, dstDir string) error {
	// Rebuild destination each time to ensure snapshot mirrors source exactly.
	if err := os.RemoveAll(dstDir); err != nil {
		return err
	}

	return filepath.WalkDir(srcDir, func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			return err
		}

		relPath, err := filepath.Rel(srcDir, path)
		if err != nil {
			return err
		}

		targetPath := filepath.Join(dstDir, relPath)
		if d.IsDir() {
			info, infoErr := d.Info()
			if infoErr != nil {
				return infoErr
			}
			// Preserve source permissions for directories.
			return os.MkdirAll(targetPath, info.Mode().Perm())
		}

		return copyFile(path, targetPath)
	})
}

func copyFile(srcFile, dstFile string) error {
	if err := os.MkdirAll(filepath.Dir(dstFile), 0o755); err != nil {
		return err
	}

	src, err := os.Open(srcFile)
	if err != nil {
		return err
	}
	defer src.Close()

	info, err := src.Stat()
	if err != nil {
		return err
	}

	dst, err := os.OpenFile(dstFile, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, info.Mode().Perm())
	if err != nil {
		return err
	}
	defer dst.Close()

	// Stream copy keeps memory use small for larger DB and log files.
	if _, err := io.Copy(dst, src); err != nil {
		return err
	}

	return nil
}
