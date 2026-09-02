PY := python3
ifeq (, $(shell command -v python3 2>/dev/null))
  PY := python
endif

dashboard-kill:
	@echo "Killing listeners on port 8000 (if any)..."
	@pids=$$(lsof -t -nP -iTCP:8000 -sTCP:LISTEN 2>/dev/null); \
	if [ -n "$$pids" ]; then \
		echo "Found PIDs: $$pids"; \
		kill $$pids; \
		echo "Stopped port 8000 listeners."; \
	else \
		echo "No listeners on port 8000."; \
	fi

dashboard:
	@echo "Installing dashboard dependencies..."
	if ! $(PY) -m pip --version >/dev/null 2>&1; then \
		echo "pip not found, attempting to install..."; \
		$(PY) -m ensurepip --upgrade || $(PY) -m pip install --upgrade pip; \
	fi; \
	$(PY) -m pip install --upgrade pip; \
	$(PY) -m pip install -r requirements.txt
	@echo "Launching FastAPI dashboard..."
	$(PY) -m uvicorn main:app --reload --host 0.0.0.0 --port 8000
