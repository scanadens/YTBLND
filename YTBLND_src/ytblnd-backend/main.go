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
	requestLogPath := os.Getenv("YTBLND_REQUEST_LOG_PATH")
	if requestLogPath == "" {
		requestLogPath = "./data/requests.log.txt"
	}

	eventLogPath := os.Getenv("YTBLND_EVENT_LOG_PATH")
	if eventLogPath == "" {
		eventLogPath = "./data/events.log.txt"
	}

	if err := os.MkdirAll(filepath.Dir(requestLogPath), 0o755); err != nil {
		log.Fatalf("failed to create log directory: %v", err)
	}
	if err := os.MkdirAll(filepath.Dir(eventLogPath), 0o755); err != nil {
		log.Fatalf("failed to create event log directory: %v", err)
	}

	serverLoggers, err := routelayer.NewServerLoggers(requestLogPath, eventLogPath)
	if err != nil {
		log.Fatalf("failed to initialize loggers: %v", err)
	}
	defer func() {
		if err := serverLoggers.Close(); err != nil {
			log.Printf("logger close error: %v", err)
		}
	}()

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
	serverLoggers.Events.LogEvent("server_start", "db_path=%s request_log_path=%s event_log_path=%s started_at=%s", dbPath, requestLogPath, eventLogPath, time.Now().UTC().Format(time.RFC3339))

	// Start the in-memory chat fanout loop used by websocket clients.
	hub := routelayer.NewChatHub(dbManager, serverLoggers.Events)
	go hub.Run()

	r := gin.Default()
	r.Use(serverLoggers.Requests.RequestMiddleware())
	r.GET("/ping", func(c *gin.Context) {
		c.JSON(200, gin.H{"message": "pong"})
	})

	// Group all product API endpoints under a versioned prefix.
	api := r.Group("/api/v1")
	routelayer.RegisterAuthRoutes(api, dbManager, serverLoggers.Events)
	routelayer.RegisterBlendRoutes(api, dbManager, serverLoggers.Events)
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
	receivedSignal := <-quit
	serverLoggers.Events.LogEvent("server_signal_received", "signal=%s received_at=%s", receivedSignal.String(), time.Now().UTC().Format(time.RFC3339))

	// Gracefully drain requests before process exit.
	shutdownCtx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	if err := server.Shutdown(shutdownCtx); err != nil {
		log.Printf("server shutdown error: %v", err)
		serverLoggers.Events.LogEvent("server_shutdown_error", "signal=%s error=%v", receivedSignal.String(), err)
	}
	serverLoggers.Events.LogEvent("server_shutdown", "signal=%s graceful_shutdown=true stopped_at=%s", receivedSignal.String(), time.Now().UTC().Format(time.RFC3339))
}
