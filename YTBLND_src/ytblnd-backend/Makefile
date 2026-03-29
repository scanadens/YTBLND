# Low-memory defaults for constrained cloud instances.
GOMAXPROCS ?= 1
GOMEMLIMIT ?= 700MiB
GO_TEST_ENV := GOMAXPROCS=$(GOMAXPROCS) GOMEMLIMIT=$(GOMEMLIMIT)

.PHONY: test-db-quick test-db-user test-db-watchlater test-db-blend

# Runs all fast SQLite manager tests with conservative memory settings.
test-db-quick:
	$(GO_TEST_ENV) go test ./database_layer -run 'TestSqliteDataManager_(UserFlow|WatchLaterReplaceAndOrder|BlendFlow)' -count=1 -vet=off -v

# Runs only user CRUD/auth flow test.
test-db-user:
	$(GO_TEST_ENV) go test ./database_layer -run TestSqliteDataManager_UserFlow -count=1 -vet=off -v

# Runs only watch-later replacement/order flow test.
test-db-watchlater:
	$(GO_TEST_ENV) go test ./database_layer -run TestSqliteDataManager_WatchLaterReplaceAndOrder -count=1 -vet=off -v

# Runs only blend save/find/participants flow test.
test-db-blend:
	$(GO_TEST_ENV) go test ./database_layer -run TestSqliteDataManager_BlendFlow -count=1 -vet=off -v
