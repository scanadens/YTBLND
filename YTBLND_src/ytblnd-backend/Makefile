# Low-memory defaults for constrained cloud instances.
# server is hosted on a cloud-instance from vultr. Only has a 1GB of memory so
# workload and implementation needs to be designed in mind that processing 
# data may take some time. pinging the server may be slow becuase of this restraint.
# starting up server procedures may also take some time as well. though the goal
# is that once the server is going as a systemd service than it should be 
# smooth sailing
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
