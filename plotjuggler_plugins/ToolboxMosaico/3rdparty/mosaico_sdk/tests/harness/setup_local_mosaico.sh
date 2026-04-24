#!/usr/bin/env bash
# Set up a local Mosaico server with TLS + API keys for testing.
#
# Usage:
#   setup_local_mosaico.sh [options]
#   setup_local_mosaico.sh cleanup
#
# Options:
#   --port PORT           Port to expose on host (default: 6726)
#   --certs-dir DIR       Where to write certs (default: /tmp/mosaico-certs)
#   --compose-dir DIR     Where to write compose.yml (default: /tmp/mosaico-test)
#   --image TAG           mosaicod image tag (default: latest)
#   --keep-certs          Don't regenerate certs if they exist
#   --reuse-stack         Don't recreate containers if already running
#   --verbose, -v         Print every command as it runs
#   --help, -h            Show this help
#
# Subcommand:
#   cleanup               Stop containers, remove volumes, delete certs
#
# Env vars override defaults:
#   MOSAICO_PORT, MOSAICO_CERTS_DIR, MOSAICO_COMPOSE_DIR, MOSAICO_IMAGE

set -euo pipefail

# ---------------------------------------------------------------------------
# Colors / logging
# ---------------------------------------------------------------------------
if [ -t 1 ]; then
  BLUE='\033[1;34m'; YELLOW='\033[1;33m'; RED='\033[1;31m'; GREEN='\033[1;32m'; DIM='\033[2m'; RESET='\033[0m'
else
  BLUE=''; YELLOW=''; RED=''; GREEN=''; DIM=''; RESET=''
fi

log()   { printf "${BLUE}[mosaico]${RESET} %s\n" "$*"; }
warn()  { printf "${YELLOW}[warn]${RESET} %s\n" "$*" >&2; }
fail()  { printf "${RED}[error]${RESET} %s\n" "$*" >&2; exit 1; }
ok()    { printf "${GREEN}[ok]${RESET} %s\n" "$*"; }
debug() { [ -n "${VERBOSE:-}" ] && printf "${DIM}  > %s${RESET}\n" "$*" || true; }

# ---------------------------------------------------------------------------
# Defaults (overridable via env or CLI flags)
# ---------------------------------------------------------------------------
PORT="${MOSAICO_PORT:-6726}"
CERTS_DIR="${MOSAICO_CERTS_DIR:-/tmp/mosaico-certs}"
COMPOSE_DIR="${MOSAICO_COMPOSE_DIR:-/tmp/mosaico-test}"
IMAGE_TAG="${MOSAICO_IMAGE:-latest}"
KEEP_CERTS=""
REUSE_STACK=""
VERBOSE=""
MODE="setup"

# ---------------------------------------------------------------------------
# Parse args
# ---------------------------------------------------------------------------
show_help() {
  grep -E '^# ' "$0" | head -30 | sed 's/^# \?//'
  exit 0
}

while [ $# -gt 0 ]; do
  case "$1" in
    --port)         PORT="$2"; shift 2;;
    --certs-dir)    CERTS_DIR="$2"; shift 2;;
    --compose-dir)  COMPOSE_DIR="$2"; shift 2;;
    --image)        IMAGE_TAG="$2"; shift 2;;
    --keep-certs)   KEEP_CERTS=1; shift;;
    --reuse-stack)  REUSE_STACK=1; shift;;
    --verbose|-v)   VERBOSE=1; shift;;
    --help|-h)      show_help;;
    cleanup)        MODE="cleanup"; shift;;
    *)              fail "unknown argument: $1 (use --help)";;
  esac
done

# ---------------------------------------------------------------------------
# Preflight checks
# ---------------------------------------------------------------------------
check_preconditions() {
  debug "checking preconditions"
  command -v docker  >/dev/null || fail "docker not installed. Install Docker Engine or Desktop."
  command -v openssl >/dev/null || fail "openssl not installed"
  docker compose version >/dev/null 2>&1 || fail "docker compose v2 not available. Run 'docker compose version' to check."
  docker info >/dev/null 2>&1 || fail "docker daemon not reachable. Is Docker running? Try: sudo systemctl start docker"
  ok "docker $(docker --version | awk '{print $3}' | tr -d ,) + openssl $(openssl version | awk '{print $2}')"
}

check_port_free() {
  debug "checking if port $PORT is free"
  if command -v ss >/dev/null && ss -ltn "sport = :$PORT" 2>/dev/null | grep -q ":$PORT "; then
    # Is it our own mosaicod?
    if docker compose -f "$COMPOSE_DIR/compose.yml" ps 2>/dev/null | grep -q mosaicod; then
      warn "port $PORT is bound by a previous mosaico-test stack. Use --reuse-stack to keep it, or cleanup to remove it."
    else
      fail "port $PORT is already in use by another process (not ours). Use --port to choose another, or free the port."
    fi
  fi
}

# ---------------------------------------------------------------------------
# Cleanup
# ---------------------------------------------------------------------------
do_cleanup() {
  log "Cleaning up..."
  if [ -f "$COMPOSE_DIR/compose.yml" ]; then
    debug "docker compose down -v in $COMPOSE_DIR"
    (cd "$COMPOSE_DIR" && docker compose down -v 2>&1 | while read -r l; do debug "$l"; done) || warn "docker compose down exited non-zero"
  else
    debug "no compose file at $COMPOSE_DIR, skipping docker down"
  fi
  debug "removing $CERTS_DIR and $COMPOSE_DIR"
  rm -rf "$CERTS_DIR" "$COMPOSE_DIR"
  ok "cleanup complete"
}

if [ "$MODE" = "cleanup" ]; then
  check_preconditions
  do_cleanup
  exit 0
fi

# ---------------------------------------------------------------------------
# Show config
# ---------------------------------------------------------------------------
cat <<EOF
Configuration:
  Port:         $PORT
  Certs dir:    $CERTS_DIR
  Compose dir:  $COMPOSE_DIR
  Image tag:    ghcr.io/mosaico-labs/mosaicod:$IMAGE_TAG
  Keep certs:   $([ -n "$KEEP_CERTS" ] && echo yes || echo no)
  Reuse stack:  $([ -n "$REUSE_STACK" ] && echo yes || echo no)
  Verbose:      $([ -n "$VERBOSE" ] && echo yes || echo no)

EOF

[ -n "$VERBOSE" ] && set -x || true

# ---------------------------------------------------------------------------
# Preconditions
# ---------------------------------------------------------------------------
check_preconditions

# ---------------------------------------------------------------------------
# Handle existing stack
# ---------------------------------------------------------------------------
STACK_RUNNING=""
if [ -f "$COMPOSE_DIR/compose.yml" ]; then
  if docker compose -f "$COMPOSE_DIR/compose.yml" ps --services --filter "status=running" 2>/dev/null | grep -q mosaicod; then
    STACK_RUNNING=1
  fi
fi

if [ -n "$STACK_RUNNING" ]; then
  if [ -n "$REUSE_STACK" ]; then
    log "Existing stack is running, --reuse-stack specified — keeping it."
  else
    warn "Existing mosaico-test stack is running. Stopping and removing it (use --reuse-stack to keep)."
    (cd "$COMPOSE_DIR" && docker compose down -v 2>&1 | while read -r l; do debug "$l"; done) || true
    STACK_RUNNING=""
  fi
fi

check_port_free

# ---------------------------------------------------------------------------
# Generate certs (or reuse)
# ---------------------------------------------------------------------------
CERTS_READY=""
if [ -n "$KEEP_CERTS" ] && [ -f "$CERTS_DIR/ca.pem" ] && [ -f "$CERTS_DIR/cert.pem" ] && [ -f "$CERTS_DIR/key.pem" ]; then
  log "Reusing existing certs at $CERTS_DIR (--keep-certs)"
  CERTS_READY=1
fi

if [ -z "$CERTS_READY" ]; then
  log "Generating CA and server certificates..."
  rm -rf "$CERTS_DIR"
  mkdir -p "$CERTS_DIR"
  cd "$CERTS_DIR"

  cat > ca.conf <<'EOF'
[req]
distinguished_name = req_distinguished_name
x509_extensions = v3_ca
prompt = no

[req_distinguished_name]
CN = MyTestCA

[v3_ca]
basicConstraints = critical, CA:TRUE
keyUsage = critical, keyCertSign, cRLSign
subjectKeyIdentifier = hash
EOF

  cat > server.ext <<'EOF'
basicConstraints = CA:FALSE
keyUsage = digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = DNS:localhost, IP:127.0.0.1
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid,issuer
EOF

  debug "generating CA private key"
  openssl genrsa -out ca.key 4096 >/dev/null 2>&1
  debug "self-signing CA certificate"
  openssl req -x509 -new -nodes -key ca.key -sha256 -days 365 \
    -config ca.conf -out ca.pem 2>/dev/null

  debug "generating server private key"
  openssl genrsa -out key.pem 4096 >/dev/null 2>&1
  debug "creating server CSR"
  openssl req -new -key key.pem -out server.csr -subj "/CN=localhost" 2>/dev/null
  debug "signing server certificate with CA"
  openssl x509 -req -in server.csr -CA ca.pem -CAkey ca.key -CAcreateserial \
    -out cert.pem -days 365 -sha256 -extfile server.ext 2>/dev/null

  log "Verifying certificate chain..."
  openssl verify -CAfile ca.pem cert.pem >/dev/null || fail "cert verification failed"
  openssl x509 -in ca.pem -text -noout | grep -q "CA:TRUE" || fail "CA missing CA:TRUE"
  openssl x509 -in ca.pem -text -noout | grep -q "Certificate Sign" || fail "CA missing keyCertSign (needed for gRPC/BoringSSL)"
  openssl x509 -in cert.pem -text -noout | grep -q "DNS:localhost" || fail "server cert missing DNS:localhost SAN"
  openssl x509 -in cert.pem -text -noout | grep -q "IP Address:127.0.0.1" || fail "server cert missing IP:127.0.0.1 SAN"
  ok "certificates valid"
fi

# ---------------------------------------------------------------------------
# Write / update compose file
# ---------------------------------------------------------------------------
if [ -z "$STACK_RUNNING" ]; then
  log "Writing docker compose file..."
  mkdir -p "$COMPOSE_DIR"
  cat > "$COMPOSE_DIR/compose.yml" <<EOF
name: mosaico-test
services:
  db:
    image: postgres:18
    environment:
      POSTGRES_USER: postgres
      POSTGRES_PASSWORD: password
      POSTGRES_DB: mosaico
    networks: [mosaico]
    volumes: [pg-data:/var/lib/postgresql]
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U postgres"]
      interval: 5s
      retries: 5

  mosaicod:
    image: ghcr.io/mosaico-labs/mosaicod:$IMAGE_TAG
    environment:
      MOSAICOD_DB_URL: postgresql://postgres:password@db:5432/mosaico
      MOSAICOD_TLS_CERT_FILE: /certs/cert.pem
      MOSAICOD_TLS_PRIVATE_KEY_FILE: /certs/key.pem
    networks: [mosaico]
    volumes:
      - $CERTS_DIR:/certs:ro
      - mosaico-data:/data
    command: run --host 0.0.0.0 --port 6726 --tls --api-key --log-level info --local-store /data
    depends_on:
      db:
        condition: service_healthy
    ports:
      - "127.0.0.1:$PORT:6726"

volumes:
  pg-data:
  mosaico-data:

networks:
  mosaico:
EOF
  debug "compose file: $COMPOSE_DIR/compose.yml"
fi

# ---------------------------------------------------------------------------
# Start stack
# ---------------------------------------------------------------------------
cd "$COMPOSE_DIR"

if [ -z "$STACK_RUNNING" ]; then
  log "Pulling images..."
  docker compose pull --quiet 2>&1 | while read -r l; do debug "$l"; done

  log "Starting stack..."
  docker compose up -d 2>&1 | while read -r l; do debug "$l"; done

  log "Waiting for mosaicod to be ready..."
  READY=""
  READY_TIMEOUT="${MOSAICO_READY_TIMEOUT:-180}"
  for i in $(seq 1 "$READY_TIMEOUT"); do
    STATUS=$(docker compose ps --format json mosaicod 2>/dev/null || echo "")
    if echo "$STATUS" | grep -q '"State":"exited"'; then
      warn "mosaicod container exited. Logs:"
      docker compose logs mosaicod
      fail "mosaicod crashed on startup"
    fi
    # Port probe first (network-level readiness).
    if docker compose exec -T mosaicod sh -c 'exec 3<>/dev/tcp/127.0.0.1/6726' 2>/dev/null; then
      READY=1
      break
    fi
    # Log-pattern fallback.
    if docker compose logs mosaicod 2>/dev/null | grep -qE "ready in|listening|Listening|Serving"; then
      READY=1
      break
    fi
    debug "waiting... ($i/$READY_TIMEOUT)"
    sleep 1
  done

  if [ -z "$READY" ]; then
    warn "mosaicod did not signal readiness in ${READY_TIMEOUT}s. Recent logs:"
    docker compose logs --tail=30 mosaicod
    fail "startup timed out"
  fi
  ok "mosaicod is up"
else
  ok "reusing running stack"
fi

# ---------------------------------------------------------------------------
# Create admin API key
# ---------------------------------------------------------------------------
log "Creating admin API key (manage permission)..."
set +e  # need to inspect exit codes manually here, disable pipefail traps
ADMIN_KEY_OUTPUT=$(docker compose exec -T mosaicod mosaicod api-key create --permissions manage -d "admin-test-key" 2>&1)
ADMIN_KEY_EXIT=$?
set -e

if [ $ADMIN_KEY_EXIT -ne 0 ]; then
  warn "api-key create exited with code $ADMIN_KEY_EXIT. Output:"
  echo "$ADMIN_KEY_OUTPUT" | sed 's/^/  /'
  fail "key creation failed — see output above"
fi

ADMIN_KEY=$(echo "$ADMIN_KEY_OUTPUT" | grep -oE 'msco_[a-z0-9]{32}_[0-9a-f]{8}' | head -1 || true)

if [ -z "$ADMIN_KEY" ]; then
  warn "api-key create succeeded but no msco_ token found in output. Full output:"
  echo "$ADMIN_KEY_OUTPUT" | sed 's/^/  /'
  fail "could not parse admin API key"
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
[ -n "$VERBOSE" ] && set +x || true

cat <<EOF

${GREEN}================================================================${RESET}
 ${GREEN}Mosaico local test server is running.${RESET}
${GREEN}================================================================${RESET}

  Server URI:      grpc+tls://localhost:$PORT
  Client CA cert:  $CERTS_DIR/ca.pem
  Admin API key:   $ADMIN_KEY

 In the plugin dialog, paste:
   Certificate:  $CERTS_DIR/ca.pem
   API Key:      $ADMIN_KEY

 Manage:
   cleanup:        $0 cleanup
   view logs:      docker compose -f $COMPOSE_DIR/compose.yml logs -f mosaicod
   restart:        $0 --reuse-stack      (re-issues admin key)
   stop only:      docker compose -f $COMPOSE_DIR/compose.yml down
   create more:    docker compose -f $COMPOSE_DIR/compose.yml exec mosaicod \\
                     mosaicod api-key create --permissions [read|write|delete|manage]

${GREEN}================================================================${RESET}
EOF
