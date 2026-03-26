package main

import (
	"context"
	"log"
	"net/http"
	"os"
	"os/signal"
	"path/filepath"
	"syscall"
	"time"

	"github.com/gin-gonic/gin"

	database_layer "ytblnd-backend/database_layer"
	routelayer "ytblnd-backend/route_layer"
)

func main() {
	// Use an env override for DB path so local/dev/prod can vary storage.
	dbPath := os.Getenv("YTBLND_DB_PATH")
	if dbPath == "" {
		dbPath = "./data/dev.db"
	}

	// Ensure DB parent directory exists before SQLite tries to open the file.
	if err := os.MkdirAll(filepath.Dir(dbPath), 0o755); err != nil {
		log.Fatalf("failed to create data directory: %v", err)
	}

	// Initialize persistence dependencies once at process startup.
	dbManager, err := database_layer.NewSqliteDataManager(dbPath)
	if err != nil {
		log.Fatalf("failed to initialize sqlite manager: %v", err)
	}

	// Start the in-memory chat fanout loop used by websocket clients.
	hub := routelayer.NewChatHub()
	go hub.Run()

	r := gin.Default()
	r.GET("/ping", func(c *gin.Context) {
		c.JSON(200, gin.H{"message": "pong"})
	})

	// Group all product API endpoints under a versioned prefix.
	api := r.Group("/api/v1")
	routelayer.RegisterAuthRoutes(api, dbManager)
	routelayer.RegisterBlendRoutes(api, dbManager)
	routelayer.RegisterChatRoutes(api, dbManager, hub)

	server := &http.Server{
		Addr:    "0.0.0.0:8080",
		Handler: r,
	}

	// Run the HTTP server in a goroutine so we can listen for OS signals.
	go func() {
		if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatalf("server failed: %v", err)
		}
	}()

	// Block until termination signals arrive.
	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
	<-quit

	// Gracefully drain requests before process exit.
	shutdownCtx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	if err := server.Shutdown(shutdownCtx); err != nil {
		log.Printf("server shutdown error: %v", err)
	}
}
