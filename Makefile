PICO_SDK_PATH ?= $(HOME)/sdk/pico-sdk
BUILD_DIR ?= build/firmware
RECORD_SECONDS ?= 300
WEB_HOST ?= 127.0.0.1
WEB_PORT ?= 8000
FIRMWARE_DIR := firmware
WEB_DIR := web
UF2 := $(BUILD_DIR)/keyremap.uf2
HOST_PROBE_UF2 := $(BUILD_DIR)/host_probe.uf2
WEB_PID := .web.pid
WEB_LOG := .web.log

.PHONY: all help configure build firmware flash firmware-write build-host-probe probe flash-host-probe probe-write test-firmware clean distclean doctor web-start web-stop web-restart web-status web-open record-handle summarize-handle analyze-captures validate-layout list-handle

all: build

help:
	@echo "Firmware:"
	@echo "  make build              Build normal keyremap firmware"
	@echo "  make flash              Build and copy keyremap UF2 to RPI-RP2"
	@echo "  make build-host-probe   Build host-only probe firmware"
	@echo "  make flash-host-probe   Build and copy host_probe UF2 to RPI-RP2"
	@echo "  make test-firmware      Run host-side firmware unit tests"
	@echo ""
	@echo "Web UI:"
	@echo "  make web-start          Start local static server at http://$(WEB_HOST):$(WEB_PORT)"
	@echo "  make web-stop           Stop local static server"
	@echo "  make web-restart        Restart local static server"
	@echo "  make web-status         Show local static server status"
	@echo "  make web-open           Open local Web UI in the default browser"
	@echo ""
	@echo "Diagnostics:"
	@echo "  make doctor             Check toolchain and USB devices"
	@echo "  make list-handle        List Linux input devices"
	@echo "  make record-handle      Record handle input events"
	@echo "  make validate-layout    Validate captured handle layout"

configure:
	PICO_SDK_PATH="$(PICO_SDK_PATH)" cmake -S "$(FIRMWARE_DIR)" -B "$(BUILD_DIR)"

build: configure
	cmake --build "$(BUILD_DIR)" -j$$(nproc)

firmware: build

build-host-probe: configure
	cmake --build "$(BUILD_DIR)" --target host_probe -j$$(nproc)

probe: build-host-probe

flash: build
	@mountpoint=$$(lsblk -nrpo LABEL,MOUNTPOINT | awk '$$1 == "RPI-RP2" && $$2 != "" { print $$2; exit }'); \
	if [ -z "$$mountpoint" ]; then \
		device=$$(lsblk -nrpo NAME,LABEL | awk '$$2 == "RPI-RP2" { print $$1; exit }'); \
		if [ -z "$$device" ]; then \
			echo "RPI-RP2 not found. Hold BOOTSEL, plug in the board, then run: make flash"; \
			exit 1; \
		fi; \
		mountpoint=$$(udisksctl mount -b "$$device" | sed -n 's/^Mounted .* at //p' | sed 's/\.$$//'); \
	fi; \
	if [ -z "$$mountpoint" ]; then \
		echo "Could not find or mount RPI-RP2."; \
		exit 1; \
	fi; \
	echo "Copying $(UF2) to $$mountpoint"; \
	cp "$(UF2)" "$$mountpoint/"; \
	sync

firmware-write: flash

flash-host-probe: build-host-probe
	@mountpoint=$$(lsblk -nrpo LABEL,MOUNTPOINT | awk '$$1 == "RPI-RP2" && $$2 != "" { print $$2; exit }'); \
	if [ -z "$$mountpoint" ]; then \
		device=$$(lsblk -nrpo NAME,LABEL | awk '$$2 == "RPI-RP2" { print $$1; exit }'); \
		if [ -z "$$device" ]; then \
			echo "RPI-RP2 not found. Hold BOOTSEL, plug in the board, then run: make flash-host-probe"; \
			exit 1; \
		fi; \
		mountpoint=$$(udisksctl mount -b "$$device" | sed -n 's/^Mounted .* at //p' | sed 's/\.$$//'); \
	fi; \
	if [ -z "$$mountpoint" ]; then \
		echo "Could not find or mount RPI-RP2."; \
		exit 1; \
	fi; \
	echo "Copying $(HOST_PROBE_UF2) to $$mountpoint"; \
	cp "$(HOST_PROBE_UF2)" "$$mountpoint/"; \
	sync

probe-write: flash-host-probe

test-firmware:
	@mkdir -p build/test
	gcc -Wall -Wextra -Werror -std=c11 -I$(FIRMWARE_DIR)/src $(FIRMWARE_DIR)/test/test_hid_parser.c $(FIRMWARE_DIR)/src/hid_parser.c -o build/test/test_hid_parser
	build/test/test_hid_parser

clean:
	cmake --build "$(BUILD_DIR)" --target clean

distclean:
	rm -rf build

doctor:
	@echo "PICO_SDK_PATH=$(PICO_SDK_PATH)"
	@test -f "$(PICO_SDK_PATH)/external/pico_sdk_import.cmake" || { echo "Missing Pico SDK at $(PICO_SDK_PATH)"; exit 1; }
	@cmake --version | head -1
	@arm-none-eabi-gcc --version | head -1
	@lsusb | grep -E '2e8a:0003|cafe:4020|1c4f:007c' || true

web-start:
	@if [ -f "$(WEB_PID)" ] && kill -0 "$$(cat "$(WEB_PID)")" 2>/dev/null; then \
		echo "Web UI already running at http://$(WEB_HOST):$(WEB_PORT) (pid $$(cat "$(WEB_PID)"))"; \
	else \
		rm -f "$(WEB_PID)"; \
		python3 -m http.server "$(WEB_PORT)" --bind "$(WEB_HOST)" -d "$(WEB_DIR)" >"$(WEB_LOG)" 2>&1 & \
		echo $$! > "$(WEB_PID)"; \
		sleep 0.3; \
		if kill -0 "$$(cat "$(WEB_PID)")" 2>/dev/null; then \
			echo "Web UI running at http://$(WEB_HOST):$(WEB_PORT) (pid $$(cat "$(WEB_PID)"))"; \
		else \
			echo "Failed to start Web UI. See $(WEB_LOG)"; \
			rm -f "$(WEB_PID)"; \
			exit 1; \
		fi; \
	fi

web-stop:
	@if [ -f "$(WEB_PID)" ] && kill -0 "$$(cat "$(WEB_PID)")" 2>/dev/null; then \
		kill "$$(cat "$(WEB_PID)")"; \
		rm -f "$(WEB_PID)"; \
		echo "Web UI stopped"; \
	else \
		rm -f "$(WEB_PID)"; \
		echo "Web UI is not running"; \
	fi

web-restart: web-stop web-start

web-status:
	@if [ -f "$(WEB_PID)" ] && kill -0 "$$(cat "$(WEB_PID)")" 2>/dev/null; then \
		echo "Web UI running at http://$(WEB_HOST):$(WEB_PORT) (pid $$(cat "$(WEB_PID)"))"; \
	else \
		echo "Web UI is not running"; \
	fi

web-open:
	@url="http://$(WEB_HOST):$(WEB_PORT)"; \
	if command -v xdg-open >/dev/null 2>&1; then \
		xdg-open "$$url" >/dev/null 2>&1 & \
	elif command -v open >/dev/null 2>&1; then \
		open "$$url" >/dev/null 2>&1 & \
	else \
		echo "$$url"; \
	fi

list-handle:
	tools/record_linux_input.py --list

record-handle:
	sudo tools/record_linux_input.py --seconds "$(RECORD_SECONDS)"

summarize-handle:
	sudo tools/record_linux_input.py --summarize-only

analyze-captures:
	tools/analyze_captures.py

validate-layout:
	tools/validate_layout.py
