PIO_DIR    = .pio/build/genericSTM32F103C8
OBJCOPY   ?= arm-none-eabi-objcopy

ORIENT    ?= UI_PORTRAIT
THEME     ?= CATPPUCCIN_FRAPPE
BATTERY   ?= 1

BUILD_FLAGS = -D$(ORIENT) -D$(THEME)
ifeq ($(BATTERY),0)
BUILD_FLAGS += -DNO_BATTERY
endif

.PHONY: all hex clean releases

all:
	PLATFORMIO_BUILD_FLAGS="$(BUILD_FLAGS)" pio run

hex: all
	$(OBJCOPY) -O ihex $(PIO_DIR)/firmware.elf openpm01.hex

clean:
	pio run --target clean

releases:
	@mkdir -p releases
	@for orient in UI_PORTRAIT UI_PORTRAIT_FLIP UI_LANDSCAPE UI_LANDSCAPE_FLIP; do \
	  for theme in CATPPUCCIN_MOCHA CATPPUCCIN_MACCHIATO CATPPUCCIN_FRAPPE CATPPUCCIN_LATTE; do \
	    printf "  %-20s %-22s ... " "$$orient" "$$theme"; \
	    PLATFORMIO_BUILD_FLAGS="-D$$orient -D$$theme" pio run >/dev/null 2>&1 || exit 1; \
	    oname=$$(echo "$$orient" | tr '[:upper:]_' '[:lower:]-' | sed 's/^ui-//'); \
	    tname=$$(echo "$$theme" | tr '[:upper:]_' '[:lower:]-' | sed 's/^catppuccin-//'); \
	    $(OBJCOPY) -O ihex $(PIO_DIR)/firmware.elf "releases/openpm01-$$oname-$$tname.hex"; \
	    echo "ok"; \
	  done; \
	done
	@echo "Done — $$(ls releases/*.hex | wc -l) hex files in releases/"
