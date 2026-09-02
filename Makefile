.PHONY: agbcc c-match diff disasm scan-libc libc-align dashboard-watch prepare-rom verify-rom doctor progress dashboard-data dashboard-dev dashboard-build analyze sync-functions bootstrap-match intr-match init-interrupts-match game-init-match vblank-match irq-helpers-match reset-display-match init-save-system-match read-eeprom-match write-eeprom-match save-slots-match save-wrappers-match save-manager-match read-eeprom-range-match write-eeprom-range-match save-helpers-match menu-layout-match draw-menu-match init-menu-screen-match menu-helpers-match menu-graphics-match matching

ROM_ZIP ?=

prepare-rom:
	@test -n "$(ROM_ZIP)" || (echo 'ROM_ZIP yolunu belirtin.' >&2; exit 2)
	@./tools/prepare_rom.sh "$(ROM_ZIP)"

verify-rom:
	@./tools/prepare_rom.sh --verify

doctor:
	@./tools/doctor.sh

progress:
	@python3 tools/progress.py data/functions.csv

agbcc:
	@tools/setup_agbcc.sh $(if $(FORCE),--force,)

# Bir C dosyasindaki her fonksiyonu agbcc ile derleyip ROM ile karsilastirir.
# Ornek: make c-match FILE=src/save/save_helpers.c
c-match: verify-rom
	@python3 tools/verify_c_function.py $(FILE)

# Tek fonksiyonun ROM halini derlenmis haliyle yan yana gosterir.
# Ornek: make diff FILE=src/save/save_helpers.c FUNC=WriteU16LE
diff: verify-rom
	@python3 tools/diff_function.py $(FILE) $(FUNC)

# ROM'daki bir fonksiyonun disassembly'sini uretir (kaynak: ROM, depo degil).
# Ornek: make disasm FUNC=EraseSaveSlot
disasm: verify-rom
	@python3 tools/disasm_function.py $(FUNC)

# agbcc libc.a fonksiyonlarini ROM icinde arar (--csv override satiri uretir).
scan-libc: verify-rom
	@python3 tools/scan_libc.py $(ARGS)

# Bir libc nesnesini bilinen capadan hizalayip fonksiyon fonksiyon karsilastirir.
# Ornek: make libc-align FUNC=remap_handle ADDR=0x0807180C
libc-align: verify-rom
	@python3 tools/locate_libc_objects.py $(FUNC) $(ADDR)

# Veri dosyalarini izler, degisince dashboard JSON'unu yeniler.
# dashboard-dev ile birlikte calistir: harita anlik guncellenir.
dashboard-watch:
	@python3 tools/watch_dashboard.py

dashboard-data:
	@python3 tools/generate_dashboard_data.py

dashboard-dev: dashboard-data
	@cd dashboard && npm run dev

dashboard-build: dashboard-data
	@cd dashboard && npm run build

analyze:
	@./tools/run_initial_analysis.sh

sync-functions:
	@python3 tools/sync_function_map.py

build/bootstrap/agb_main.o: src/bootstrap/agb_main.s
	@mkdir -p build/bootstrap
	@arm-none-eabi-as -mcpu=arm7tdmi -o $@ $<

build/bootstrap/agb_main.elf: build/bootstrap/agb_main.o config/bootstrap.ld
	@arm-none-eabi-ld -T config/bootstrap.ld -o $@ $<

build/bootstrap/agb_main.bin: build/bootstrap/agb_main.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.boot $< $@

bootstrap-match: verify-rom build/bootstrap/agb_main.bin
	@python3 tools/compare_slice.py baserom.gba 0xc0 build/bootstrap/agb_main.bin

build/bootstrap/intr_main.o: src/bootstrap/intr_main.s
	@mkdir -p build/bootstrap
	@arm-none-eabi-as -mcpu=arm7tdmi -o $@ $<

build/bootstrap/intr_main.elf: build/bootstrap/intr_main.o config/intr_main.ld
	@arm-none-eabi-ld -T config/intr_main.ld -o $@ $<

build/bootstrap/intr_main.bin: build/bootstrap/intr_main.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.intr $< $@

intr-match: verify-rom build/bootstrap/intr_main.bin
	@python3 tools/compare_slice.py baserom.gba 0x104 build/bootstrap/intr_main.bin

build/bootstrap/init_interrupts.o: src/bootstrap/init_interrupts.s
	@mkdir -p build/bootstrap
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/bootstrap/init_interrupts.elf: build/bootstrap/init_interrupts.o config/init_interrupts.ld
	@arm-none-eabi-ld -T config/init_interrupts.ld -o $@ $<

build/bootstrap/init_interrupts.bin: build/bootstrap/init_interrupts.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.init_interrupts $< $@

init-interrupts-match: verify-rom build/bootstrap/init_interrupts.bin
	@python3 tools/compare_slice.py baserom.gba 0x38c build/bootstrap/init_interrupts.bin

build/bootstrap/game_init.o: src/bootstrap/game_init.s
	@mkdir -p build/bootstrap
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/bootstrap/game_init.elf: build/bootstrap/game_init.o config/game_init.ld
	@arm-none-eabi-ld -T config/game_init.ld -o $@ $<

build/bootstrap/game_init.bin: build/bootstrap/game_init.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.game_init $< $@

game-init-match: verify-rom build/bootstrap/game_init.bin
	@python3 tools/compare_slice.py baserom.gba 0x430 build/bootstrap/game_init.bin

build/interrupt/vblank_intr.o: src/interrupt/vblank_intr.s
	@mkdir -p build/interrupt
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/interrupt/vblank_intr.elf: build/interrupt/vblank_intr.o config/vblank_intr.ld
	@arm-none-eabi-ld -T config/vblank_intr.ld -o $@ $<

build/interrupt/vblank_intr.bin: build/interrupt/vblank_intr.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.vblank_intr $< $@

vblank-match: verify-rom build/interrupt/vblank_intr.bin
	@python3 tools/compare_slice.py baserom.gba 0x220 build/interrupt/vblank_intr.bin

build/interrupt/irq_helpers.o: src/interrupt/irq_helpers.s
	@mkdir -p build/interrupt
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/interrupt/irq_helpers.elf: build/interrupt/irq_helpers.o config/irq_helpers.ld
	@arm-none-eabi-ld -T config/irq_helpers.ld -o $@ $<

build/interrupt/irq_helpers.bin: build/interrupt/irq_helpers.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.irq_helpers $< $@

irq-helpers-match: verify-rom build/interrupt/irq_helpers.bin
	@python3 tools/compare_slice.py baserom.gba 0x730 build/interrupt/irq_helpers.bin

build/bootstrap/reset_display_interrupts.o: src/bootstrap/reset_display_interrupts.s
	@mkdir -p build/bootstrap
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/bootstrap/reset_display_interrupts.elf: build/bootstrap/reset_display_interrupts.o config/reset_display_interrupts.ld
	@arm-none-eabi-ld -T config/reset_display_interrupts.ld -o $@ $<

build/bootstrap/reset_display_interrupts.bin: build/bootstrap/reset_display_interrupts.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.reset_display_interrupts $< $@

reset-display-match: verify-rom build/bootstrap/reset_display_interrupts.bin
	@python3 tools/compare_slice.py baserom.gba 0x7b4 build/bootstrap/reset_display_interrupts.bin

build/save/init_save_system.o: src/save/init_save_system.s
	@mkdir -p build/save
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/save/init_save_system.elf: build/save/init_save_system.o config/init_save_system.ld
	@arm-none-eabi-ld -T config/init_save_system.ld -o $@ $<

build/save/init_save_system.bin: build/save/init_save_system.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.init_save_system $< $@

init-save-system-match: verify-rom build/save/init_save_system.bin
	@python3 tools/compare_slice.py baserom.gba 0x82c build/save/init_save_system.bin

build/save/read_eeprom_bytes.o: src/save/read_eeprom_bytes.s
	@mkdir -p build/save
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/save/read_eeprom_bytes.elf: build/save/read_eeprom_bytes.o config/read_eeprom_bytes.ld
	@arm-none-eabi-ld -T config/read_eeprom_bytes.ld -o $@ $<

build/save/read_eeprom_bytes.bin: build/save/read_eeprom_bytes.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.read_eeprom_bytes $< $@

read-eeprom-match: verify-rom build/save/read_eeprom_bytes.bin
	@python3 tools/compare_slice.py baserom.gba 0x91c build/save/read_eeprom_bytes.bin

build/save/write_eeprom_bytes.o: src/save/write_eeprom_bytes.s
	@mkdir -p build/save
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/save/write_eeprom_bytes.elf: build/save/write_eeprom_bytes.o config/write_eeprom_bytes.ld
	@arm-none-eabi-ld -T config/write_eeprom_bytes.ld -o $@ $<

build/save/write_eeprom_bytes.bin: build/save/write_eeprom_bytes.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.write_eeprom_bytes $< $@

write-eeprom-match: verify-rom build/save/write_eeprom_bytes.bin
	@python3 tools/compare_slice.py baserom.gba 0x9ec build/save/write_eeprom_bytes.bin

build/save/save_slots.o: src/save/save_slots.s
	@mkdir -p build/save
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/save/save_slots.elf: build/save/save_slots.o config/save_slots.ld
	@arm-none-eabi-ld -T config/save_slots.ld -o $@ $<

build/save/save_slots.bin: build/save/save_slots.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.save_slots $< $@

save-slots-match: verify-rom build/save/save_slots.bin
	@python3 tools/compare_slice.py baserom.gba 0xb00 build/save/save_slots.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/save/save_wrappers.bin: src/save/save_wrappers.c data/functions.csv data/ram_map.csv
	@mkdir -p build/save
	@python3 tools/build_c.py $< $@

save-wrappers-match: verify-rom build/save/save_wrappers.bin
	@python3 tools/compare_slice.py baserom.gba 0xbe4 build/save/save_wrappers.bin

build/save/save_manager.o: src/save/save_manager.s
	@mkdir -p build/save
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/save/save_manager.elf: build/save/save_manager.o config/save_manager.ld
	@arm-none-eabi-ld -T config/save_manager.ld -o $@ $<

build/save/save_manager.bin: build/save/save_manager.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.save_manager $< $@

save-manager-match: verify-rom build/save/save_manager.bin
	@python3 tools/compare_slice.py baserom.gba 0xc28 build/save/save_manager.bin

build/save/read_eeprom_range.o: src/save/read_eeprom_range.s
	@mkdir -p build/save
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/save/read_eeprom_range.elf: build/save/read_eeprom_range.o config/read_eeprom_range.ld
	@arm-none-eabi-ld -T config/read_eeprom_range.ld -o $@ $<

build/save/read_eeprom_range.bin: build/save/read_eeprom_range.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.read_eeprom_range $< $@

read-eeprom-range-match: verify-rom build/save/read_eeprom_range.bin
	@python3 tools/compare_slice.py baserom.gba 0xddc build/save/read_eeprom_range.bin

build/save/write_eeprom_range.o: src/save/write_eeprom_range.s
	@mkdir -p build/save
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/save/write_eeprom_range.elf: build/save/write_eeprom_range.o config/write_eeprom_range.ld
	@arm-none-eabi-ld -T config/write_eeprom_range.ld -o $@ $<

build/save/write_eeprom_range.bin: build/save/write_eeprom_range.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.write_eeprom_range $< $@

write-eeprom-range-match: verify-rom build/save/write_eeprom_range.bin
	@python3 tools/compare_slice.py baserom.gba 0xf1c build/save/write_eeprom_range.bin

build/save/save_helpers.o: src/save/save_helpers.s
	@mkdir -p build/save
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/save/save_helpers.elf: build/save/save_helpers.o config/save_helpers.ld
	@arm-none-eabi-ld -T config/save_helpers.ld -o $@ $<

build/save/save_helpers.bin: build/save/save_helpers.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.save_helpers $< $@

save-helpers-match: verify-rom build/save/save_helpers.bin
	@python3 tools/compare_slice.py baserom.gba 0x1094 build/save/save_helpers.bin

build/ui/build_active_menu_items.o: src/ui/build_active_menu_items.s
	@mkdir -p build/ui
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/ui/build_active_menu_items.elf: build/ui/build_active_menu_items.o config/build_active_menu_items.ld
	@arm-none-eabi-ld -T config/build_active_menu_items.ld -o $@ $<

build/ui/build_active_menu_items.bin: build/ui/build_active_menu_items.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.build_active_menu_items $< $@

menu-layout-match: verify-rom build/ui/build_active_menu_items.bin
	@python3 tools/compare_slice.py baserom.gba 0x114c build/ui/build_active_menu_items.bin

build/ui/draw_menu_items.o: src/ui/draw_menu_items.s
	@mkdir -p build/ui
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/ui/draw_menu_items.elf: build/ui/draw_menu_items.o config/draw_menu_items.ld
	@arm-none-eabi-ld -T config/draw_menu_items.ld -o $@ $<

build/ui/draw_menu_items.bin: build/ui/draw_menu_items.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.draw_menu_items $< $@

draw-menu-match: verify-rom build/ui/draw_menu_items.bin
	@python3 tools/compare_slice.py baserom.gba 0x11ec build/ui/draw_menu_items.bin

build/ui/init_menu_screen.o: src/ui/init_menu_screen.s
	@mkdir -p build/ui
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/ui/init_menu_screen.elf: build/ui/init_menu_screen.o config/init_menu_screen.ld
	@arm-none-eabi-ld -T config/init_menu_screen.ld -o $@ $<

build/ui/init_menu_screen.bin: build/ui/init_menu_screen.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.init_menu_screen $< $@

init-menu-screen-match: verify-rom build/ui/init_menu_screen.bin
	@python3 tools/compare_slice.py baserom.gba 0x13ac build/ui/init_menu_screen.bin

build/ui/menu_helpers.o: src/ui/menu_helpers.s
	@mkdir -p build/ui
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/ui/menu_helpers.elf: build/ui/menu_helpers.o config/menu_helpers.ld
	@arm-none-eabi-ld -T config/menu_helpers.ld -o $@ $<

build/ui/menu_helpers.bin: build/ui/menu_helpers.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.menu_helpers $< $@

menu-helpers-match: verify-rom build/ui/menu_helpers.bin
	@python3 tools/compare_slice.py baserom.gba 0x1dc0 build/ui/menu_helpers.bin

build/ui/menu_graphics.o: src/ui/menu_graphics.s
	@mkdir -p build/ui
	@arm-none-eabi-as -mcpu=arm7tdmi -mthumb -o $@ $<

build/ui/menu_graphics.elf: build/ui/menu_graphics.o config/menu_graphics.ld
	@arm-none-eabi-ld -T config/menu_graphics.ld -o $@ $<

build/ui/menu_graphics.bin: build/ui/menu_graphics.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text.menu_graphics $< $@

menu-graphics-match: verify-rom build/ui/menu_graphics.bin
	@python3 tools/compare_slice.py baserom.gba 0x1e30 build/ui/menu_graphics.bin

matching: bootstrap-match intr-match init-interrupts-match game-init-match vblank-match irq-helpers-match reset-display-match init-save-system-match read-eeprom-match write-eeprom-match save-slots-match save-wrappers-match save-manager-match read-eeprom-range-match write-eeprom-range-match save-helpers-match menu-layout-match draw-menu-match init-menu-screen-match menu-helpers-match menu-graphics-match
	@python3 tools/verify_matching_regions.py
