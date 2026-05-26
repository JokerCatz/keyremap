PICO_SDK_PATH ?= $(HOME)/sdk/pico-sdk
BUILD_DIR ?= build/firmware
RECORD_SECONDS ?= 300
FIRMWARE_DIR := firmware
UF2 := $(BUILD_DIR)/keyremap.uf2
HOST_PROBE_UF2 := $(BUILD_DIR)/host_probe.uf2

.PHONY: all configure build flash build-host-probe flash-host-probe clean distclean doctor record-handle summarize-handle analyze-captures validate-layout list-handle

all: build

configure:
	PICO_SDK_PATH="$(PICO_SDK_PATH)" cmake -S "$(FIRMWARE_DIR)" -B "$(BUILD_DIR)"

build: configure
	cmake --build "$(BUILD_DIR)" -j$$(nproc)

build-host-probe: configure
	cmake --build "$(BUILD_DIR)" --target host_probe -j$$(nproc)

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
