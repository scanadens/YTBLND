package routelayer

import (
	"fmt"
	"log"
	"os"
	"path/filepath"
	"time"

	"github.com/gin-gonic/gin"
)

// EventLogger writes structured request and application events to a text file.
type EventLogger struct {
	logger *log.Logger
	file   *os.File
}

// NewEventLogger opens the log file and returns a shared logger instance.
func NewEventLogger(path string) (*EventLogger, error) {
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return nil, fmt.Errorf("create log directory: %w", err)
	}

	file, err := os.OpenFile(path, os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0o644)
	if err != nil {
		return nil, fmt.Errorf("open log file: %w", err)
	}

	return &EventLogger{
		logger: log.New(file, "", log.LstdFlags|log.Lmicroseconds|log.LUTC),
		file:   file,
	}, nil
}

// ServerLoggers keeps request and event log streams separate.
type ServerLoggers struct {
	Requests *EventLogger
	Events   *EventLogger
}

// NewServerLoggers initializes both request and event log files.
func NewServerLoggers(requestPath, eventPath string) (*ServerLoggers, error) {
	requestLogger, err := NewEventLogger(requestPath)
	if err != nil {
		return nil, fmt.Errorf("create request logger: %w", err)
	}

	eventLogger, err := NewEventLogger(eventPath)
	if err != nil {
		_ = requestLogger.Close()
		return nil, fmt.Errorf("create event logger: %w", err)
	}

	return &ServerLoggers{
		Requests: requestLogger,
		Events:   eventLogger,
	}, nil
}

// Close releases both underlying file handles.
func (l *ServerLoggers) Close() error {
	if l == nil {
		return nil
	}
	if l.Requests != nil {
		if err := l.Requests.Close(); err != nil {
			return err
		}
	}
	if l.Events != nil {
		if err := l.Events.Close(); err != nil {
			return err
		}
	}
	return nil
}

// Close releases the underlying file handle.
func (l *EventLogger) Close() error {
	if l == nil || l.file == nil {
		return nil
	}
	return l.file.Close()
}

// LogEvent records a named application event.
func (l *EventLogger) LogEvent(name, format string, args ...any) {
	if l == nil || l.logger == nil {
		return
	}
	l.logger.Printf("event=%s %s", name, fmt.Sprintf(format, args...))
}

// RequestMiddleware appends one line per HTTP request after the handler runs.
func (l *EventLogger) RequestMiddleware() gin.HandlerFunc {
	return func(c *gin.Context) {
		startedAt := time.Now().UTC()
		c.Next()

		if l == nil || l.logger == nil {
			return
		}

		latency := time.Since(startedAt)
		l.logger.Printf(
			"event=http_request method=%s path=%s raw_query=%q status=%d latency=%s client_ip=%s",
			c.Request.Method,
			c.Request.URL.Path,
			c.Request.URL.RawQuery,
			c.Writer.Status(),
			latency,
			c.ClientIP(),
		)
	}
}
