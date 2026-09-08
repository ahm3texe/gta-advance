.PHONY: check check-full status status-update status-check queue-check sibling-check boundary-check boundary-baseline toolchain-check toolchain-corpus consistency rom agbcc c-match c-status c-review diff disasm scan-libc libc-align libc-verify dashboard-watch prepare-rom verify-rom doctor progress dashboard-data dashboard-dev dashboard-build dashboard-lint analyze sync-functions bootstrap-match intr-match init-interrupts-match game-init-match vblank-match irq-helpers-match reset-display-match init-save-system-match read-eeprom-match write-eeprom-match save-slots-match save-wrappers-match save-manager-match read-eeprom-range-match write-eeprom-range-match save-helpers-match menu-layout-match draw-menu-match init-menu-screen-match menu-helpers-match menu-graphics-match matching

ROM_ZIP ?=

# c_sources.csv, hangi C ceviri birimlerinin matching build hedefi oldugunun
# tek kaynagidir. Ortak header/build araci/toolchain kilidi degisince bu
# hedeflerin tamami yeniden uretilir; bayat .bin kullanilamaz.
MATCHING_C_SOURCES := $(shell python3 -c 'import csv; print(" ".join(sorted({r["source"] for r in csv.DictReader(open("data/c_sources.csv")) if r["matching"] == "yes"})))')
MATCHING_C_BINS := $(sort $(patsubst src/%.c,build/%.bin,$(MATCHING_C_SOURCES)))
C_BUILD_DEPS := $(wildcard include/*.h) tools/build_c.py tools/agbcc_build.py config/toolchain.lock.json
$(MATCHING_C_BINS): $(C_BUILD_DEPS)

prepare-rom:
	@test -n "$(ROM_ZIP)" || (echo 'ROM_ZIP yolunu belirtin.' >&2; exit 2)
	@./tools/prepare_rom.sh "$(ROM_ZIP)"

verify-rom:
	@./tools/prepare_rom.sh --verify

doctor:
	@./tools/doctor.sh

progress:
	@python3 tools/progress.py data/functions.csv

# Canli ve tekil durum gorunumu. Sabit sayilar README/PLAN'e yazilmaz.
status:
	@python3 tools/project_status.py

status-update: c-status
	@python3 tools/project_status.py --write

status-check:
	@python3 tools/project_status.py --check

queue-check:
	@python3 tools/check_work_queue.py

sibling-check:
	@python3 tools/check_sibling_band.py

boundary-check:
	@python3 tools/audit_boundaries.py --check-baseline

# Yalnizca bulgular tek tek incelendikten sonra bilincli olarak calistirilir.
boundary-baseline:
	@python3 tools/audit_boundaries.py --write-baseline

toolchain-check:
	@python3 tools/verify_toolchain.py

toolchain-corpus:
	@python3 tools/verify_toolchain.py --corpus

agbcc:
	@tools/setup_agbcc.sh $(if $(FORCE),--force,)

# Bir C dosyasindaki her fonksiyonu agbcc ile derleyip ROM ile karsilastirir.
# Ornek: make c-match FILE=src/save/save_helpers.c
c-match: verify-rom
	@python3 tools/verify_c_function.py $(FILE)

# src/ altindaki tum C kaynaklarini derleyip ROM ile karsilastirir ve
# data/c_sources.csv'yi uretir. progress ve dashboard bunu okur.
c-status: verify-rom
	@python3 tools/scan_c_sources.py

# C kaynaklarini okunabilirlik acisindan denetler (byte eslesmesi yetmez).
c-review:
	@python3 tools/review_c_source.py $(FILE)

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

# Commit oncesi kapilar: bilinen borca izin verir, yeni regresyonu reddeder.
check: toolchain-check rom
	@python3 tools/scan_c_sources.py
	@python3 tools/check_consistency.py
	@python3 tools/check_work_queue.py
	@python3 tools/check_sibling_band.py
	@python3 tools/audit_boundaries.py --check-baseline
	@python3 tools/review_c_source.py
	@python3 tools/progress.py
	@python3 tools/generate_dashboard_data.py
	@python3 tools/check_generated_views.py
	@python3 tools/project_status.py --check

# Kilometre tasi/merge kapisi: tum matching hedefleri cache'siz uretilir,
# C corpus parmak izi ve dashboard urun kaynaklari da dogrulanir.
check-full:
	@$(MAKE) -B check
	@python3 tools/verify_toolchain.py --corpus
	@$(MAKE) dashboard-lint dashboard-build

# Hibrit ROM sinamasi: dogrulanmis bolgeler kendi kaynagimizdan, kalani
# baserom.gba'dan gelir. Hash kaynak bolgelerinin yerlesimini denetler.
rom: matching
	@python3 tools/build_rom.py

# ROM'daki standart kutuphane bolgelerini agbcc libc.a'sina karsi dogrular.
libc-verify: verify-rom
	@python3 tools/verify_libc_regions.py

# Bir libc nesnesini bilinen capadan hizalayip fonksiyon fonksiyon karsilastirir.
# Ornek: make libc-align FUNC=remap_handle ADDR=0x0807180C
libc-align: verify-rom
	@python3 tools/locate_libc_objects.py $(FUNC) $(ADDR)

# Veri dosyalarini izler, degisince dashboard JSON'unu yeniler.
# dashboard-dev ile birlikte calistir: harita anlik guncellenir.
dashboard-watch:
	@python3 tools/watch_dashboard.py

# Veri dosyalarinin kendi icinde, birbiriyle ve kaynakla tutarliligi.
consistency:
	@python3 tools/check_consistency.py

dashboard-data:
	@python3 tools/generate_dashboard_data.py

dashboard-dev: dashboard-data
	@cd dashboard && npm run dev

dashboard-build: dashboard-data
	@cd dashboard && npm run build

dashboard-lint:
	@cd dashboard && npm run lint

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

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/bootstrap/init_interrupts.bin: src/bootstrap/init_interrupts.c data/functions.csv data/ram_map.csv
	@mkdir -p build/bootstrap
	@python3 tools/build_c.py $< $@

init-interrupts-match: verify-rom build/bootstrap/init_interrupts.bin
	@python3 tools/compare_slice.py baserom.gba 0x38c build/bootstrap/init_interrupts.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/bootstrap/game_init.bin: src/bootstrap/game_init.c data/functions.csv data/ram_map.csv
	@mkdir -p build/bootstrap
	@python3 tools/build_c.py $< $@

game-init-match: verify-rom build/bootstrap/game_init.bin
	@python3 tools/compare_slice.py baserom.gba 0x430 build/bootstrap/game_init.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/interrupt/vblank_intr.bin: src/interrupt/vblank_intr.c data/functions.csv data/ram_map.csv
	@mkdir -p build/interrupt
	@python3 tools/build_c.py $< $@

vblank-match: verify-rom build/interrupt/vblank_intr.bin
	@python3 tools/compare_slice.py baserom.gba 0x220 build/interrupt/vblank_intr.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/interrupt/irq_helpers.bin: src/interrupt/irq_helpers.c data/functions.csv data/ram_map.csv
	@mkdir -p build/interrupt
	@python3 tools/build_c.py $< $@

irq-helpers-match: verify-rom build/interrupt/irq_helpers.bin
	@python3 tools/compare_slice.py baserom.gba 0x730 build/interrupt/irq_helpers.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/bootstrap/reset_display_interrupts.bin: src/bootstrap/reset_display_interrupts.c data/functions.csv data/ram_map.csv
	@mkdir -p build/bootstrap
	@python3 tools/build_c.py $< $@

reset-display-match: verify-rom build/bootstrap/reset_display_interrupts.bin
	@python3 tools/compare_slice.py baserom.gba 0x7b4 build/bootstrap/reset_display_interrupts.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/save/init_save_system.bin: src/save/init_save_system.c data/functions.csv data/ram_map.csv
	@mkdir -p build/save
	@python3 tools/build_c.py $< $@

init-save-system-match: verify-rom build/save/init_save_system.bin
	@python3 tools/compare_slice.py baserom.gba 0x82c build/save/init_save_system.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/save/read_eeprom_bytes.bin: src/save/read_eeprom_bytes.c data/functions.csv data/ram_map.csv
	@mkdir -p build/save
	@python3 tools/build_c.py $< $@

read-eeprom-match: verify-rom build/save/read_eeprom_bytes.bin
	@python3 tools/compare_slice.py baserom.gba 0x91c build/save/read_eeprom_bytes.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/save/write_eeprom_bytes.bin: src/save/write_eeprom_bytes.c data/functions.csv data/ram_map.csv
	@mkdir -p build/save
	@python3 tools/build_c.py $< $@

write-eeprom-match: verify-rom build/save/write_eeprom_bytes.bin
	@python3 tools/compare_slice.py baserom.gba 0x9ec build/save/write_eeprom_bytes.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/save/save_slots.bin: src/save/save_slots.c data/functions.csv data/ram_map.csv
	@mkdir -p build/save
	@python3 tools/build_c.py $< $@

save-slots-match: verify-rom build/save/save_slots.bin
	@python3 tools/compare_slice.py baserom.gba 0xb00 build/save/save_slots.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/save/save_wrappers.bin: src/save/save_wrappers.c data/functions.csv data/ram_map.csv
	@mkdir -p build/save
	@python3 tools/build_c.py $< $@

save-wrappers-match: verify-rom build/save/save_wrappers.bin
	@python3 tools/compare_slice.py baserom.gba 0xbe4 build/save/save_wrappers.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/save/save_manager.bin: src/save/save_manager.c data/functions.csv data/ram_map.csv
	@mkdir -p build/save
	@python3 tools/build_c.py $< $@

save-manager-match: verify-rom build/save/save_manager.bin
	@python3 tools/compare_slice.py baserom.gba 0xc28 build/save/save_manager.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/save/read_eeprom_range.bin: src/save/read_eeprom_range.c data/functions.csv data/ram_map.csv
	@mkdir -p build/save
	@python3 tools/build_c.py $< $@

read-eeprom-range-match: verify-rom build/save/read_eeprom_range.bin
	@python3 tools/compare_slice.py baserom.gba 0xddc build/save/read_eeprom_range.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/save/write_eeprom_range.bin: src/save/write_eeprom_range.c data/functions.csv data/ram_map.csv
	@mkdir -p build/save
	@python3 tools/build_c.py $< $@

write-eeprom-range-match: verify-rom build/save/write_eeprom_range.bin
	@python3 tools/compare_slice.py baserom.gba 0xf1c build/save/write_eeprom_range.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/save/save_helpers.bin: src/save/save_helpers.c data/functions.csv data/ram_map.csv
	@mkdir -p build/save
	@python3 tools/build_c.py $< $@

save-helpers-match: verify-rom build/save/save_helpers.bin
	@python3 tools/compare_slice.py baserom.gba 0x1094 build/save/save_helpers.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/ui/build_active_menu_items.bin: src/ui/build_active_menu_items.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

menu-layout-match: verify-rom build/ui/build_active_menu_items.bin
	@python3 tools/compare_slice.py baserom.gba 0x114c build/ui/build_active_menu_items.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/ui/draw_menu_items.bin: src/ui/draw_menu_items.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

draw-menu-match: verify-rom build/ui/draw_menu_items.bin
	@python3 tools/compare_slice.py baserom.gba 0x11ec build/ui/draw_menu_items.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/ui/init_menu_screen.bin: src/ui/init_menu_screen.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

init-menu-screen-match: verify-rom build/ui/init_menu_screen.bin
	@python3 tools/compare_slice.py baserom.gba 0x13ac build/ui/init_menu_screen.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/ui/menu_helpers.bin: src/ui/menu_helpers.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

menu-helpers-match: verify-rom build/ui/menu_helpers.bin
	@python3 tools/compare_slice.py baserom.gba 0x1dc0 build/ui/menu_helpers.bin

# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.
build/ui/menu_graphics.bin: src/ui/menu_graphics.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

menu-graphics-match: verify-rom build/ui/menu_graphics.bin
	@python3 tools/compare_slice.py baserom.gba 0x1e30 build/ui/menu_graphics.bin

build/world/entity_accessors.bin: src/world/entity_accessors.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-entity-accessors-match: verify-rom build/world/entity_accessors.bin
	@python3 tools/compare_slice.py baserom.gba 0x32090 build/world/entity_accessors.bin

build/misc/state_getters.bin: src/misc/state_getters.c data/functions.csv data/ram_map.csv
	@mkdir -p build/misc
	@python3 tools/build_c.py $< $@

misc-state-getters-match: verify-rom build/misc/state_getters.bin
	@python3 tools/compare_slice.py baserom.gba 0x37f1c build/misc/state_getters.bin

build/misc/table_lookup.bin: src/misc/table_lookup.c data/functions.csv data/ram_map.csv
	@mkdir -p build/misc
	@python3 tools/build_c.py $< $@

misc-table-lookup-match: verify-rom build/misc/table_lookup.bin
	@python3 tools/compare_slice.py baserom.gba 0x5e6c4 build/misc/table_lookup.bin

build/misc/record_table.bin: src/misc/record_table.c data/functions.csv data/ram_map.csv
	@mkdir -p build/misc
	@python3 tools/build_c.py $< $@

misc-record-table-match: verify-rom build/misc/record_table.bin
	@python3 tools/compare_slice.py baserom.gba 0x514c8 build/misc/record_table.bin

build/misc/session_reset.bin: src/misc/session_reset.c data/functions.csv data/ram_map.csv
	@mkdir -p build/misc
	@python3 tools/build_c.py $< $@

misc-session-reset-match: verify-rom build/misc/session_reset.bin
	@python3 tools/compare_slice.py baserom.gba 0x3c77c build/misc/session_reset.bin

build/text/draw_text.bin: src/text/draw_text.c data/functions.csv data/ram_map.csv
	@mkdir -p build/text
	@python3 tools/build_c.py $< $@

text-draw-text-match: verify-rom build/text/draw_text.bin
	@python3 tools/compare_slice.py baserom.gba 0x6434c build/text/draw_text.bin

build/world/entity_flags.bin: src/world/entity_flags.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-entity-flags-match: verify-rom build/world/entity_flags.bin
	@python3 tools/compare_slice.py baserom.gba 0x32058 build/world/entity_flags.bin

build/misc/session_node.bin: src/misc/session_node.c data/functions.csv data/ram_map.csv
	@mkdir -p build/misc
	@python3 tools/build_c.py $< $@

misc-session-node-match: verify-rom build/misc/session_node.bin
	@python3 tools/compare_slice.py baserom.gba 0x3c798 build/misc/session_node.bin

build/ui/menu_loop.bin: src/ui/menu_loop.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

ui-menu-loop-match: verify-rom build/ui/menu_loop.bin
	@python3 tools/compare_slice.py baserom.gba 0x1a00 build/ui/menu_loop.bin

build/world/map_tiles.bin: src/world/map_tiles.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-map-tiles-match: verify-rom build/world/map_tiles.bin
	@python3 tools/compare_slice.py baserom.gba 0x42434 build/world/map_tiles.bin

build/bios/syscalls.o: src/bios/syscalls.s
	@mkdir -p build/bios
	@arm-none-eabi-as -mcpu=arm7tdmi -o $@ $<

build/bios/syscalls.elf: build/bios/syscalls.o config/bios.ld
	@arm-none-eabi-ld -T config/bios.ld -o $@ $<

build/bios/syscalls.bin: build/bios/syscalls.elf
	@arm-none-eabi-objcopy -O binary --only-section=.text $< $@

bios-match: verify-rom build/bios/syscalls.bin
	@python3 tools/compare_slice.py baserom.gba 0x6b84c build/bios/syscalls.bin


build/misc/coord_accessors.bin: src/misc/coord_accessors.c data/functions.csv data/ram_map.csv
	@mkdir -p build/misc
	@python3 tools/build_c.py $< $@

misc-coord-accessors-match: verify-rom build/misc/coord_accessors.bin
	@python3 tools/compare_slice.py baserom.gba 0xa94c build/misc/coord_accessors.bin

build/world/object_helpers.bin: src/world/object_helpers.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-object-helpers-match: verify-rom build/world/object_helpers.bin
	@python3 tools/compare_slice.py baserom.gba 0x38358 build/world/object_helpers.bin

build/core/linked_list.bin: src/core/linked_list.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-linked-list-match: verify-rom build/core/linked_list.bin
	@python3 tools/compare_slice.py baserom.gba 0x12898 build/core/linked_list.bin

build/world/object_state.bin: src/world/object_state.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-object-state-match: verify-rom build/world/object_state.bin
	@python3 tools/compare_slice.py baserom.gba 0x19670 build/world/object_state.bin

build/world/actor_states.bin: src/world/actor_states.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-actor-states-match: verify-rom build/world/actor_states.bin
	@python3 tools/compare_slice.py baserom.gba 0x3f6f8 build/world/actor_states.bin

build/world/slot_config.bin: src/world/slot_config.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-slot-config-match: verify-rom build/world/slot_config.bin
	@python3 tools/compare_slice.py baserom.gba 0x3c050 build/world/slot_config.bin

build/world/slot_table.bin: src/world/slot_table.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-slot-table-match: verify-rom build/world/slot_table.bin
	@python3 tools/compare_slice.py baserom.gba 0x55d08 build/world/slot_table.bin

build/world/stat_counters.bin: src/world/stat_counters.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-stat-counters-match: verify-rom build/world/stat_counters.bin
	@python3 tools/compare_slice.py baserom.gba 0x67228 build/world/stat_counters.bin

build/world/slot_query.bin: src/world/slot_query.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-slot-query-match: verify-rom build/world/slot_query.bin
	@python3 tools/compare_slice.py baserom.gba 0x3c53c build/world/slot_query.bin

build/world/node_search.bin: src/world/node_search.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-node-search-match: verify-rom build/world/node_search.bin
	@python3 tools/compare_slice.py baserom.gba 0x55a68 build/world/node_search.bin

build/world/actor_control.bin: src/world/actor_control.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-actor-control-match: verify-rom build/world/actor_control.bin
	@python3 tools/compare_slice.py baserom.gba 0x19450 build/world/actor_control.bin

build/world/area_flags.bin: src/world/area_flags.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-area-flags-match: verify-rom build/world/area_flags.bin
	@python3 tools/compare_slice.py baserom.gba 0x30c4c build/world/area_flags.bin

build/world/object_value.bin: src/world/object_value.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-object-value-match: verify-rom build/world/object_value.bin
	@python3 tools/compare_slice.py baserom.gba 0x38260 build/world/object_value.bin

build/world/table_entries.bin: src/world/table_entries.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-table-entries-match: verify-rom build/world/table_entries.bin
	@python3 tools/compare_slice.py baserom.gba 0x28c48 build/world/table_entries.bin

build/world/pause_helpers.bin: src/world/pause_helpers.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-pause-helpers-match: verify-rom build/world/pause_helpers.bin
	@python3 tools/compare_slice.py baserom.gba 0x664a4 build/world/pause_helpers.bin

build/world/slot_selectors.bin: src/world/slot_selectors.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-slot-selectors-match: verify-rom build/world/slot_selectors.bin
	@python3 tools/compare_slice.py baserom.gba 0x3c49c build/world/slot_selectors.bin

build/world/list_head.bin: src/world/list_head.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-list-head-match: verify-rom build/world/list_head.bin
	@python3 tools/compare_slice.py baserom.gba 0x13974 build/world/list_head.bin

build/world/more_counters.bin: src/world/more_counters.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-more-counters-match: verify-rom build/world/more_counters.bin
	@python3 tools/compare_slice.py baserom.gba 0x671b8 build/world/more_counters.bin

build/world/pool_gets.bin: src/world/pool_gets.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-pool-gets-match: verify-rom build/world/pool_gets.bin
	@python3 tools/compare_slice.py baserom.gba 0x37fac build/world/pool_gets.bin

build/world/threshold.bin: src/world/threshold.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-threshold-match: verify-rom build/world/threshold.bin
	@python3 tools/compare_slice.py baserom.gba 0x23a0c build/world/threshold.bin

build/world/state_init.bin: src/world/state_init.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-state-init-match: verify-rom build/world/state_init.bin
	@python3 tools/compare_slice.py baserom.gba 0x3f630 build/world/state_init.bin

build/world/gRam02030330_gets.bin: src/world/gRam02030330_gets.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-gRam02030330-gets-match: verify-rom build/world/gRam02030330_gets.bin
	@python3 tools/compare_slice.py baserom.gba 0x509dc build/world/gRam02030330_gets.bin

build/world/slot_scan.bin: src/world/slot_scan.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-slot-scan-match: verify-rom build/world/slot_scan.bin
	@python3 tools/compare_slice.py baserom.gba 0x55c8c build/world/slot_scan.bin

build/world/list_ops.bin: src/world/list_ops.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-list-ops-match: verify-rom build/world/list_ops.bin
	@python3 tools/compare_slice.py baserom.gba 0xdbe8 build/world/list_ops.bin

build/world/map_tile_fields.bin: src/world/map_tile_fields.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-map-tile-fields-match: verify-rom build/world/map_tile_fields.bin
	@python3 tools/compare_slice.py baserom.gba 0x42058 build/world/map_tile_fields.bin

build/world/comm_flag.bin: src/world/comm_flag.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-comm-flag-match: verify-rom build/world/comm_flag.bin
	@python3 tools/compare_slice.py baserom.gba 0x66a40 build/world/comm_flag.bin

build/world/pair_lookup.bin: src/world/pair_lookup.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-pair-lookup-match: verify-rom build/world/pair_lookup.bin
	@python3 tools/compare_slice.py baserom.gba 0x2392c build/world/pair_lookup.bin

build/world/tile_and_map.bin: src/world/tile_and_map.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-tile-and-map-match: verify-rom build/world/tile_and_map.bin
	@python3 tools/compare_slice.py baserom.gba 0x424a8 build/world/tile_and_map.bin

build/world/anchor_reset.bin: src/world/anchor_reset.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-anchor-reset-match: verify-rom build/world/anchor_reset.bin
	@python3 tools/compare_slice.py baserom.gba 0x50918 build/world/anchor_reset.bin

build/world/slot_range.bin: src/world/slot_range.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-slot-range-match: verify-rom build/world/slot_range.bin
	@python3 tools/compare_slice.py baserom.gba 0x3c178 build/world/slot_range.bin

build/world/word_compare.bin: src/world/word_compare.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-word-compare-match: verify-rom build/world/word_compare.bin
	@python3 tools/compare_slice.py baserom.gba 0x50190 build/world/word_compare.bin

build/world/ram_flags.bin: src/world/ram_flags.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-ram-flags-match: verify-rom build/world/ram_flags.bin
	@python3 tools/compare_slice.py baserom.gba 0x62514 build/world/ram_flags.bin

build/world/more_counters_2.bin: src/world/more_counters_2.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-more-counters-2-match: verify-rom build/world/more_counters_2.bin
	@python3 tools/compare_slice.py baserom.gba 0x67374 build/world/more_counters_2.bin

build/world/more_counters_3.bin: src/world/more_counters_3.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-more-counters-3-match: verify-rom build/world/more_counters_3.bin
	@python3 tools/compare_slice.py baserom.gba 0x673c4 build/world/more_counters_3.bin

build/core/list_ops2.bin: src/core/list_ops2.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-list-ops2-match: verify-rom build/core/list_ops2.bin
	@python3 tools/compare_slice.py baserom.gba 0x127fc build/core/list_ops2.bin

build/world/ram_state.bin: src/world/ram_state.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-ram-state-match: verify-rom build/world/ram_state.bin
	@python3 tools/compare_slice.py baserom.gba 0x62294 build/world/ram_state.bin

build/world/window_config.bin: src/world/window_config.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-window-config-match: verify-rom build/world/window_config.bin
	@python3 tools/compare_slice.py baserom.gba 0x1d7dc build/world/window_config.bin

build/misc/coord_more.bin: src/misc/coord_more.c data/functions.csv data/ram_map.csv
	@mkdir -p build/misc
	@python3 tools/build_c.py $< $@

misc-coord-more-match: verify-rom build/misc/coord_more.bin
	@python3 tools/compare_slice.py baserom.gba 0xaad0 build/misc/coord_more.bin

build/world/id_verify.bin: src/world/id_verify.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-id-verify-match: verify-rom build/world/id_verify.bin
	@python3 tools/compare_slice.py baserom.gba 0x336ec build/world/id_verify.bin

build/world/flag_arrays.bin: src/world/flag_arrays.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-flag-arrays-match: verify-rom build/world/flag_arrays.bin
	@python3 tools/compare_slice.py baserom.gba 0x31d24 build/world/flag_arrays.bin

build/world/scan_active.bin: src/world/scan_active.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-scan-active-match: verify-rom build/world/scan_active.bin
	@python3 tools/compare_slice.py baserom.gba 0x28ffc build/world/scan_active.bin

build/world/actor_check.bin: src/world/actor_check.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-actor-check-match: verify-rom build/world/actor_check.bin
	@python3 tools/compare_slice.py baserom.gba 0x19b08 build/world/actor_check.bin

build/world/frame_chain.bin: src/world/frame_chain.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-frame-chain-match: verify-rom build/world/frame_chain.bin
	@python3 tools/compare_slice.py baserom.gba 0x28e6c build/world/frame_chain.bin

build/world/dma_flush.bin: src/world/dma_flush.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-dma-flush-match: verify-rom build/world/dma_flush.bin
	@python3 tools/compare_slice.py baserom.gba 0x13a28 build/world/dma_flush.bin

build/world/slot_release.bin: src/world/slot_release.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-slot-release-match: verify-rom build/world/slot_release.bin
	@python3 tools/compare_slice.py baserom.gba 0x308a0 build/world/slot_release.bin

build/world/counter_saturate.bin: src/world/counter_saturate.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-counter-saturate-match: verify-rom build/world/counter_saturate.bin
	@python3 tools/compare_slice.py baserom.gba 0x5ac44 build/world/counter_saturate.bin

build/world/set_index.bin: src/world/set_index.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-set-index-match: verify-rom build/world/set_index.bin
	@python3 tools/compare_slice.py baserom.gba 0x8094 build/world/set_index.bin

build/world/submit_object.bin: src/world/submit_object.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-submit-object-match: verify-rom build/world/submit_object.bin
	@python3 tools/compare_slice.py baserom.gba 0x38234 build/world/submit_object.bin

build/world/scan_all.bin: src/world/scan_all.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-scan-all-match: verify-rom build/world/scan_all.bin
	@python3 tools/compare_slice.py baserom.gba 0x29014 build/world/scan_all.bin

build/world/maybe_advance.bin: src/world/maybe_advance.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-maybe-advance-match: verify-rom build/world/maybe_advance.bin
	@python3 tools/compare_slice.py baserom.gba 0x664f0 build/world/maybe_advance.bin

build/world/entity_query.bin: src/world/entity_query.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-entity-query-match: verify-rom build/world/entity_query.bin
	@python3 tools/compare_slice.py baserom.gba 0x55af8 build/world/entity_query.bin

build/world/object_query.bin: src/world/object_query.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-object-query-match: verify-rom build/world/object_query.bin
	@python3 tools/compare_slice.py baserom.gba 0x381f8 build/world/object_query.bin

build/world/get_inner_id.bin: src/world/get_inner_id.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-get-inner-id-match: verify-rom build/world/get_inner_id.bin
	@python3 tools/compare_slice.py baserom.gba 0x3824c build/world/get_inner_id.bin

build/world/is_ram_mode.bin: src/world/is_ram_mode.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-is-ram-mode-match: verify-rom build/world/is_ram_mode.bin
	@python3 tools/compare_slice.py baserom.gba 0x62530 build/world/is_ram_mode.bin

build/world/offset_helpers.bin: src/world/offset_helpers.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-offset-helpers-match: verify-rom build/world/offset_helpers.bin
	@python3 tools/compare_slice.py baserom.gba 0x509c4 build/world/offset_helpers.bin

build/world/actor_init.bin: src/world/actor_init.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-actor-init-match: verify-rom build/world/actor_init.bin
	@python3 tools/compare_slice.py baserom.gba 0x154d8 build/world/actor_init.bin

build/world/kind_scan.bin: src/world/kind_scan.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-kind-scan-match: verify-rom build/world/kind_scan.bin
	@python3 tools/compare_slice.py baserom.gba 0x28e3c build/world/kind_scan.bin

build/world/history_push.bin: src/world/history_push.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-history-push-match: verify-rom build/world/history_push.bin
	@python3 tools/compare_slice.py baserom.gba 0x8064 build/world/history_push.bin

build/world/distance_accum.bin: src/world/distance_accum.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-distance-accum-match: verify-rom build/world/distance_accum.bin
	@python3 tools/compare_slice.py baserom.gba 0x67274 build/world/distance_accum.bin

build/world/bump_or_reset.bin: src/world/bump_or_reset.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-bump-or-reset-match: verify-rom build/world/bump_or_reset.bin
	@python3 tools/compare_slice.py baserom.gba 0x5ac50 build/world/bump_or_reset.bin

build/world/release_slot.bin: src/world/release_slot.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-release-slot-match: verify-rom build/world/release_slot.bin
	@python3 tools/compare_slice.py baserom.gba 0x308ac build/world/release_slot.bin

build/world/pool_first.bin: src/world/pool_first.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-pool-first-match: verify-rom build/world/pool_first.bin
	@python3 tools/compare_slice.py baserom.gba 0x37f70 build/world/pool_first.bin

build/world/copy_flag_byte.bin: src/world/copy_flag_byte.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-copy-flag-byte-match: verify-rom build/world/copy_flag_byte.bin
	@python3 tools/compare_slice.py baserom.gba 0x320c8 build/world/copy_flag_byte.bin

build/world/get_slot_unk10.bin: src/world/get_slot_unk10.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-get-slot-unk10-match: verify-rom build/world/get_slot_unk10.bin
	@python3 tools/compare_slice.py baserom.gba 0x50994 build/world/get_slot_unk10.bin

build/world/is_mode_two.bin: src/world/is_mode_two.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-is-mode-two-match: verify-rom build/world/is_mode_two.bin
	@python3 tools/compare_slice.py baserom.gba 0x6233c build/world/is_mode_two.bin

build/world/submit_pack.bin: src/world/submit_pack.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-submit-pack-match: verify-rom build/world/submit_pack.bin
	@python3 tools/compare_slice.py baserom.gba 0x3c7d4 build/world/submit_pack.bin

build/core/read_triple.bin: src/core/read_triple.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-read-triple-match: verify-rom build/core/read_triple.bin
	@python3 tools/compare_slice.py baserom.gba 0xa930 build/core/read_triple.bin

build/world/notify_if_ready.bin: src/world/notify_if_ready.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-notify-if-ready-match: verify-rom build/world/notify_if_ready.bin
	@python3 tools/compare_slice.py baserom.gba 0x30884 build/world/notify_if_ready.bin

build/world/bump_save_counter.bin: src/world/bump_save_counter.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-bump-save-counter-match: verify-rom build/world/bump_save_counter.bin
	@python3 tools/compare_slice.py baserom.gba 0x6720c build/world/bump_save_counter.bin

build/world/masked_compare.bin: src/world/masked_compare.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-masked-compare-match: verify-rom build/world/masked_compare.bin
	@python3 tools/compare_slice.py baserom.gba 0x23a80 build/world/masked_compare.bin

build/world/get_anchor_unk34.bin: src/world/get_anchor_unk34.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-get-anchor-unk34-match: verify-rom build/world/get_anchor_unk34.bin
	@python3 tools/compare_slice.py baserom.gba 0x50a00 build/world/get_anchor_unk34.bin

build/world/forward_with_zero.bin: src/world/forward_with_zero.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-forward-with-zero-match: verify-rom build/world/forward_with_zero.bin
	@python3 tools/compare_slice.py baserom.gba 0x30874 build/world/forward_with_zero.bin

build/world/zero_three_flags.bin: src/world/zero_three_flags.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-zero-three-flags-match: verify-rom build/world/zero_three_flags.bin
	@python3 tools/compare_slice.py baserom.gba 0x3376c build/world/zero_three_flags.bin

build/world/clear_bg1_enable.bin: src/world/clear_bg1_enable.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-clear-bg1-enable-match: verify-rom build/world/clear_bg1_enable.bin
	@python3 tools/compare_slice.py baserom.gba 0x30860 build/world/clear_bg1_enable.bin

build/world/clear_flag_notify.bin: src/world/clear_flag_notify.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-clear-flag-notify-match: verify-rom build/world/clear_flag_notify.bin
	@python3 tools/compare_slice.py baserom.gba 0x38000 build/world/clear_flag_notify.bin

build/world/select_word_source.bin: src/world/select_word_source.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-select-word-source-match: verify-rom build/world/select_word_source.bin
	@python3 tools/compare_slice.py baserom.gba 0x3c45c build/world/select_word_source.bin

build/world/zero_two_blocks.bin: src/world/zero_two_blocks.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-zero-two-blocks-match: verify-rom build/world/zero_two_blocks.bin
	@python3 tools/compare_slice.py baserom.gba 0x30c28 build/world/zero_two_blocks.bin

build/world/init_handler_pack.bin: src/world/init_handler_pack.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-init-handler-pack-match: verify-rom build/world/init_handler_pack.bin
	@python3 tools/compare_slice.py baserom.gba 0x3f67c build/world/init_handler_pack.bin

build/world/retarget_if_kind4.bin: src/world/retarget_if_kind4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-retarget-if-kind4-match: verify-rom build/world/retarget_if_kind4.bin
	@python3 tools/compare_slice.py baserom.gba 0x3f6d8 build/world/retarget_if_kind4.bin

build/world/step_then_check.bin: src/world/step_then_check.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-step-then-check-match: verify-rom build/world/step_then_check.bin
	@python3 tools/compare_slice.py baserom.gba 0x31db8 build/world/step_then_check.bin

build/world/drain_two_chains.bin: src/world/drain_two_chains.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-drain-two-chains-match: verify-rom build/world/drain_two_chains.bin
	@python3 tools/compare_slice.py baserom.gba 0x5151c build/world/drain_two_chains.bin

build/world/forward_zero_arg4.bin: src/world/forward_zero_arg4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-forward-zero-arg4-match: verify-rom build/world/forward_zero_arg4.bin
	@python3 tools/compare_slice.py baserom.gba 0x38020 build/world/forward_zero_arg4.bin

build/world/forward_zero_arg2.bin: src/world/forward_zero_arg2.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-forward-zero-arg2-match: verify-rom build/world/forward_zero_arg2.bin
	@python3 tools/compare_slice.py baserom.gba 0x55bf8 build/world/forward_zero_arg2.bin

build/world/lookup_then_call.bin: src/world/lookup_then_call.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-lookup-then-call-match: verify-rom build/world/lookup_then_call.bin
	@python3 tools/compare_slice.py baserom.gba 0x30c0c build/world/lookup_then_call.bin

build/world/reset_session_flags.bin: src/world/reset_session_flags.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-reset-session-flags-match: verify-rom build/world/reset_session_flags.bin
	@python3 tools/compare_slice.py baserom.gba 0x66540 build/world/reset_session_flags.bin

build/core/zero_history.bin: src/core/zero_history.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-zero-history-match: verify-rom build/core/zero_history.bin
	@python3 tools/compare_slice.py baserom.gba 0x8108 build/core/zero_history.bin

build/core/clear_coord_byte71.bin: src/core/clear_coord_byte71.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-clear-coord-byte71-match: verify-rom build/core/clear_coord_byte71.bin
	@python3 tools/compare_slice.py baserom.gba 0xab30 build/core/clear_coord_byte71.bin

build/world/wrap_08005fc4.bin: src/world/wrap_08005fc4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-08005fc4-match: verify-rom build/world/wrap_08005fc4.bin
	@python3 tools/compare_slice.py baserom.gba 0x5fc4 build/world/wrap_08005fc4.bin

build/world/wrap_080101d8.bin: src/world/wrap_080101d8.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-080101d8-match: verify-rom build/world/wrap_080101d8.bin
	@python3 tools/compare_slice.py baserom.gba 0x101d8 build/world/wrap_080101d8.bin

build/world/wrap_08012a98.bin: src/world/wrap_08012a98.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-08012a98-match: verify-rom build/world/wrap_08012a98.bin
	@python3 tools/compare_slice.py baserom.gba 0x12a98 build/world/wrap_08012a98.bin

build/world/wrap_080138e8.bin: src/world/wrap_080138e8.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-080138e8-match: verify-rom build/world/wrap_080138e8.bin
	@python3 tools/compare_slice.py baserom.gba 0x138e8 build/world/wrap_080138e8.bin

build/world/wrap_080138f4.bin: src/world/wrap_080138f4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-080138f4-match: verify-rom build/world/wrap_080138f4.bin
	@python3 tools/compare_slice.py baserom.gba 0x138f4 build/world/wrap_080138f4.bin

build/world/wrap_08014fa0.bin: src/world/wrap_08014fa0.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-08014fa0-match: verify-rom build/world/wrap_08014fa0.bin
	@python3 tools/compare_slice.py baserom.gba 0x14fa0 build/world/wrap_08014fa0.bin

build/world/wrap_0803378c.bin: src/world/wrap_0803378c.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-0803378c-match: verify-rom build/world/wrap_0803378c.bin
	@python3 tools/compare_slice.py baserom.gba 0x3378c build/world/wrap_0803378c.bin

build/world/wrap_0803379c.bin: src/world/wrap_0803379c.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-0803379c-match: verify-rom build/world/wrap_0803379c.bin
	@python3 tools/compare_slice.py baserom.gba 0x3379c build/world/wrap_0803379c.bin

build/world/wrap_08035dd8.bin: src/world/wrap_08035dd8.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-08035dd8-match: verify-rom build/world/wrap_08035dd8.bin
	@python3 tools/compare_slice.py baserom.gba 0x35dd8 build/world/wrap_08035dd8.bin

build/world/wrap_08042784.bin: src/world/wrap_08042784.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-08042784-match: verify-rom build/world/wrap_08042784.bin
	@python3 tools/compare_slice.py baserom.gba 0x42784 build/world/wrap_08042784.bin

build/world/wrap_080428ac.bin: src/world/wrap_080428ac.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-080428ac-match: verify-rom build/world/wrap_080428ac.bin
	@python3 tools/compare_slice.py baserom.gba 0x428ac build/world/wrap_080428ac.bin

build/world/wrap_0804fb4c.bin: src/world/wrap_0804fb4c.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-0804fb4c-match: verify-rom build/world/wrap_0804fb4c.bin
	@python3 tools/compare_slice.py baserom.gba 0x4fb4c build/world/wrap_0804fb4c.bin

build/world/get_field_100.bin: src/world/get_field_100.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-get-field-100-match: verify-rom build/world/get_field_100.bin
	@python3 tools/compare_slice.py baserom.gba 0x1d8b0 build/world/get_field_100.bin

build/world/spin_delay.bin: src/world/spin_delay.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-spin-delay-match: verify-rom build/world/spin_delay.bin
	@python3 tools/compare_slice.py baserom.gba 0x30ea0 build/world/spin_delay.bin

build/world/wrap_0804ff98.bin: src/world/wrap_0804ff98.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-0804ff98-match: verify-rom build/world/wrap_0804ff98.bin
	@python3 tools/compare_slice.py baserom.gba 0x4ff98 build/world/wrap_0804ff98.bin

build/world/wrap_0804ffb4.bin: src/world/wrap_0804ffb4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-0804ffb4-match: verify-rom build/world/wrap_0804ffb4.bin
	@python3 tools/compare_slice.py baserom.gba 0x4ffb4 build/world/wrap_0804ffb4.bin

build/world/wrap_080500d0.bin: src/world/wrap_080500d0.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-080500d0-match: verify-rom build/world/wrap_080500d0.bin
	@python3 tools/compare_slice.py baserom.gba 0x500d0 build/world/wrap_080500d0.bin

build/world/wrap_08051464.bin: src/world/wrap_08051464.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-08051464-match: verify-rom build/world/wrap_08051464.bin
	@python3 tools/compare_slice.py baserom.gba 0x51464 build/world/wrap_08051464.bin

build/world/wrap_0805a4f4.bin: src/world/wrap_0805a4f4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-0805a4f4-match: verify-rom build/world/wrap_0805a4f4.bin
	@python3 tools/compare_slice.py baserom.gba 0x5a4f4 build/world/wrap_0805a4f4.bin

build/world/wrap_08063bcc.bin: src/world/wrap_08063bcc.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-08063bcc-match: verify-rom build/world/wrap_08063bcc.bin
	@python3 tools/compare_slice.py baserom.gba 0x63bcc build/world/wrap_08063bcc.bin

build/world/get_anchor_unk04.bin: src/world/get_anchor_unk04.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-get-anchor-unk04-match: verify-rom build/world/get_anchor_unk04.bin
	@python3 tools/compare_slice.py baserom.gba 0x50988 build/world/get_anchor_unk04.bin

build/world/get_anchor_unk30.bin: src/world/get_anchor_unk30.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-get-anchor-unk30-match: verify-rom build/world/get_anchor_unk30.bin
	@python3 tools/compare_slice.py baserom.gba 0x509f4 build/world/get_anchor_unk30.bin

build/world/wrap_08051480.bin: src/world/wrap_08051480.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-08051480-match: verify-rom build/world/wrap_08051480.bin
	@python3 tools/compare_slice.py baserom.gba 0x51480 build/world/wrap_08051480.bin

build/world/wrap_08059d7c.bin: src/world/wrap_08059d7c.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-wrap-08059d7c-match: verify-rom build/world/wrap_08059d7c.bin
	@python3 tools/compare_slice.py baserom.gba 0x59d7c build/world/wrap_08059d7c.bin

build/world/get_record_index.bin: src/world/get_record_index.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-get-record-index-match: verify-rom build/world/get_record_index.bin
	@python3 tools/compare_slice.py baserom.gba 0x512b0 build/world/get_record_index.bin

build/world/get_ram_word16.bin: src/world/get_ram_word16.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-get-ram-word16-match: verify-rom build/world/get_ram_word16.bin
	@python3 tools/compare_slice.py baserom.gba 0x62654 build/world/get_ram_word16.bin

build/world/is_state_two.bin: src/world/is_state_two.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-is-state-two-match: verify-rom build/world/is_state_two.bin
	@python3 tools/compare_slice.py baserom.gba 0x5148c build/world/is_state_two.bin

build/world/is_anchor_small.bin: src/world/is_anchor_small.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-is-anchor-small-match: verify-rom build/world/is_anchor_small.bin
	@python3 tools/compare_slice.py baserom.gba 0x50a34 build/world/is_anchor_small.bin

build/world/rearm_slot.bin: src/world/rearm_slot.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-rearm-slot-match: verify-rom build/world/rearm_slot.bin
	@python3 tools/compare_slice.py baserom.gba 0x3f69c build/world/rearm_slot.bin

build/world/accumulate_distance.bin: src/world/accumulate_distance.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-accumulate-distance-match: verify-rom build/world/accumulate_distance.bin
	@python3 tools/compare_slice.py baserom.gba 0x672b8 build/world/accumulate_distance.bin

build/core/clip_bounds.bin: src/core/clip_bounds.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-clip-bounds-match: verify-rom build/core/clip_bounds.bin
	@python3 tools/compare_slice.py baserom.gba 0xa980 build/core/clip_bounds.bin

build/world/engage_actor.bin: src/world/engage_actor.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-engage-actor-match: verify-rom build/world/engage_actor.bin
	@python3 tools/compare_slice.py baserom.gba 0x55b34 build/world/engage_actor.bin

build/core/insert_sorted.bin: src/core/insert_sorted.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-insert-sorted-match: verify-rom build/core/insert_sorted.bin
	@python3 tools/compare_slice.py baserom.gba 0x1282c build/core/insert_sorted.bin

build/world/refresh_then_notify.bin: src/world/refresh_then_notify.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-refresh-then-notify-match: verify-rom build/world/refresh_then_notify.bin
	@python3 tools/compare_slice.py baserom.gba 0x55bbc build/world/refresh_then_notify.bin

build/world/check_current_entity.bin: src/world/check_current_entity.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-check-current-entity-match: verify-rom build/world/check_current_entity.bin
	@python3 tools/compare_slice.py baserom.gba 0x50a0c build/world/check_current_entity.bin

build/core/for_each_node.bin: src/core/for_each_node.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-for-each-node-match: verify-rom build/core/for_each_node.bin
	@python3 tools/compare_slice.py baserom.gba 0x12944 build/core/for_each_node.bin

build/world/set_bg1_enable.bin: src/world/set_bg1_enable.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-set-bg1-enable-match: verify-rom build/world/set_bg1_enable.bin
	@python3 tools/compare_slice.py baserom.gba 0x3084c build/world/set_bg1_enable.bin

build/world/slot_range_or_field.bin: src/world/slot_range_or_field.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-slot-range-or-field-match: verify-rom build/world/slot_range_or_field.bin
	@python3 tools/compare_slice.py baserom.gba 0x38044 build/world/slot_range_or_field.bin

build/world/init_and_mirror.bin: src/world/init_and_mirror.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-init-and-mirror-match: verify-rom build/world/init_and_mirror.bin
	@python3 tools/compare_slice.py baserom.gba 0x1d848 build/world/init_and_mirror.bin

build/world/bump_or_trigger.bin: src/world/bump_or_trigger.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-bump-or-trigger-match: verify-rom build/world/bump_or_trigger.bin
	@python3 tools/compare_slice.py baserom.gba 0x50108 build/world/bump_or_trigger.bin

build/world/apply_two_levels.bin: src/world/apply_two_levels.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-apply-two-levels-match: verify-rom build/world/apply_two_levels.bin
	@python3 tools/compare_slice.py baserom.gba 0x3c0e4 build/world/apply_two_levels.bin

build/core/sort_sprite_list.bin: src/core/sort_sprite_list.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-sort-sprite-list-match: verify-rom build/core/sort_sprite_list.bin
	@python3 tools/compare_slice.py baserom.gba 0x12a00 build/core/sort_sprite_list.bin

build/core/init_sprite_pool.bin: src/core/init_sprite_pool.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-init-sprite-pool-match: verify-rom build/core/init_sprite_pool.bin
	@python3 tools/compare_slice.py baserom.gba 0x12b20 build/core/init_sprite_pool.bin

build/core/flush_sprite_list.bin: src/core/flush_sprite_list.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-flush-sprite-list-match: verify-rom build/core/flush_sprite_list.bin
	@python3 tools/compare_slice.py baserom.gba 0x12b9c build/core/flush_sprite_list.bin

build/core/alloc_node.bin: src/core/alloc_node.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-alloc-node-match: verify-rom build/core/alloc_node.bin
	@python3 tools/compare_slice.py baserom.gba 0x12c0c build/core/alloc_node.bin

build/core/sort_active_sprites.bin: src/core/sort_active_sprites.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-sort-active-sprites-match: verify-rom build/core/sort_active_sprites.bin
	@python3 tools/compare_slice.py baserom.gba 0x12c54 build/core/sort_active_sprites.bin

build/core/insert_sprite_sorted.bin: src/core/insert_sprite_sorted.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-insert-sprite-sorted-match: verify-rom build/core/insert_sprite_sorted.bin
	@python3 tools/compare_slice.py baserom.gba 0x12c74 build/core/insert_sprite_sorted.bin

build/core/memory_init.bin: src/core/memory_init.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-memory-init-match: verify-rom build/core/memory_init.bin
	@python3 tools/compare_slice.py baserom.gba 0x100d0 build/core/memory_init.bin

build/world/link_objects.bin: src/world/link_objects.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-link-objects-match: verify-rom build/world/link_objects.bin
	@python3 tools/compare_slice.py baserom.gba 0x5ac94 build/world/link_objects.bin

build/world/nibble_replace.bin: src/world/nibble_replace.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-nibble-replace-match: verify-rom build/world/nibble_replace.bin
	@python3 tools/compare_slice.py baserom.gba 0x31c10 build/world/nibble_replace.bin

build/world/palette_lerp.bin: src/world/palette_lerp.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-palette-lerp-match: verify-rom build/world/palette_lerp.bin
	@python3 tools/compare_slice.py baserom.gba 0x199bc build/world/palette_lerp.bin

build/core/reset_runtime_globals.bin: src/core/reset_runtime_globals.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-reset-runtime-globals-match: verify-rom build/core/reset_runtime_globals.bin
	@python3 tools/compare_slice.py baserom.gba 0x5643c build/core/reset_runtime_globals.bin

build/core/adjust_area_position.bin: src/core/adjust_area_position.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-adjust-area-position-match: verify-rom build/core/adjust_area_position.bin
	@python3 tools/compare_slice.py baserom.gba 0x51540 build/core/adjust_area_position.bin

build/core/frame_dispatch.bin: src/core/frame_dispatch.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-frame-dispatch-match: verify-rom build/core/frame_dispatch.bin
	@python3 tools/compare_slice.py baserom.gba 0x5135c build/core/frame_dispatch.bin

build/core/probe_nearby_position.bin: src/core/probe_nearby_position.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-probe-nearby-position-match: verify-rom build/core/probe_nearby_position.bin
	@python3 tools/compare_slice.py baserom.gba 0x55970 build/core/probe_nearby_position.bin

build/world/player_slot_query.bin: src/world/player_slot_query.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-player-slot-query-match: verify-rom build/world/player_slot_query.bin
	@python3 tools/compare_slice.py baserom.gba 0x6543c build/world/player_slot_query.bin

build/world/slot_range_query.bin: src/world/slot_range_query.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-slot-range-query-match: verify-rom build/world/slot_range_query.bin
	@python3 tools/compare_slice.py baserom.gba 0x65518 build/world/slot_range_query.bin

build/world/bump_count_82.bin: src/world/bump_count_82.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-bump-count-82-match: verify-rom build/world/bump_count_82.bin
	@python3 tools/compare_slice.py baserom.gba 0x67184 build/world/bump_count_82.bin

build/misc/format_decimal.bin: src/misc/format_decimal.c data/functions.csv data/ram_map.csv
	@mkdir -p build/misc
	@python3 tools/build_c.py $< $@

misc-format-decimal-match: verify-rom build/misc/format_decimal.bin
	@python3 tools/compare_slice.py baserom.gba 0x672fc build/misc/format_decimal.bin

build/world/link_session.bin: src/world/link_session.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-link-session-match: verify-rom build/world/link_session.bin
	@python3 tools/compare_slice.py baserom.gba 0x663e8 build/world/link_session.bin

build/world/link_hw_reset.bin: src/world/link_hw_reset.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-link-hw-reset-match: verify-rom build/world/link_hw_reset.bin
	@python3 tools/compare_slice.py baserom.gba 0x66a54 build/world/link_hw_reset.bin

build/world/link_report.bin: src/world/link_report.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-link-report-match: verify-rom build/world/link_report.bin
	@python3 tools/compare_slice.py baserom.gba 0x66aa8 build/world/link_report.bin

build/world/actor_tracking.bin: src/world/actor_tracking.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-actor-tracking-match: verify-rom build/world/actor_tracking.bin
	@python3 tools/compare_slice.py baserom.gba 0x65378 build/world/actor_tracking.bin

build/world/scale_by_band.bin: src/world/scale_by_band.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-scale-by-band-match: verify-rom build/world/scale_by_band.bin
	@python3 tools/compare_slice.py baserom.gba 0x6549c build/world/scale_by_band.bin

build/world/link_init.bin: src/world/link_init.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-link-init-match: verify-rom build/world/link_init.bin
	@python3 tools/compare_slice.py baserom.gba 0x66568 build/world/link_init.bin

build/world/link_session_reset.bin: src/world/link_session_reset.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-link-session-reset-match: verify-rom build/world/link_session_reset.bin
	@python3 tools/compare_slice.py baserom.gba 0x66144 build/world/link_session_reset.bin

build/world/spawn_slot_effect.bin: src/world/spawn_slot_effect.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-spawn-slot-effect-match: verify-rom build/world/spawn_slot_effect.bin
	@python3 tools/compare_slice.py baserom.gba 0x65130 build/world/spawn_slot_effect.bin

build/world/bump_rank_counter.bin: src/world/bump_rank_counter.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-bump-rank-counter-match: verify-rom build/world/bump_rank_counter.bin
	@python3 tools/compare_slice.py baserom.gba 0x66c94 build/world/bump_rank_counter.bin

build/world/bitmap_asset.bin: src/world/bitmap_asset.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-bitmap-asset-match: verify-rom build/world/bitmap_asset.bin
	@python3 tools/compare_slice.py baserom.gba 0x65574 build/world/bitmap_asset.bin

build/world/link_state_step.bin: src/world/link_state_step.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-link-state-step-match: verify-rom build/world/link_state_step.bin
	@python3 tools/compare_slice.py baserom.gba 0x66b40 build/world/link_state_step.bin

build/world/link_dispatch.bin: src/world/link_dispatch.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-link-dispatch-match: verify-rom build/world/link_dispatch.bin
	@python3 tools/compare_slice.py baserom.gba 0x66904 build/world/link_dispatch.bin

build/world/link_frame_step.bin: src/world/link_frame_step.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-link-frame-step-match: verify-rom build/world/link_frame_step.bin
	@python3 tools/compare_slice.py baserom.gba 0x6660c build/world/link_frame_step.bin

build/world/link_menu_step.bin: src/world/link_menu_step.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-link-menu-step-match: verify-rom build/world/link_menu_step.bin
	@python3 tools/compare_slice.py baserom.gba 0x66ed0 build/world/link_menu_step.bin

build/world/rank_query.bin: src/world/rank_query.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-rank-query-match: verify-rom build/world/rank_query.bin
	@python3 tools/compare_slice.py baserom.gba 0x66d54 build/world/rank_query.bin

build/world/slot_probe.bin: src/world/slot_probe.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-slot-probe-match: verify-rom build/world/slot_probe.bin
	@python3 tools/compare_slice.py baserom.gba 0x651e0 build/world/slot_probe.bin

build/world/shutdown_reset.bin: src/world/shutdown_reset.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-shutdown-reset-match: verify-rom build/world/shutdown_reset.bin
	@python3 tools/compare_slice.py baserom.gba 0x65650 build/world/shutdown_reset.bin

build/world/band_0800b16c.bin: src/world/band_0800b16c.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-band-0800b16c-match: verify-rom build/world/band_0800b16c.bin
	@python3 tools/compare_slice.py baserom.gba 0xb16c build/world/band_0800b16c.bin

build/core/band_08051154.bin: src/core/band_08051154.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-band-08051154-match: verify-rom build/core/band_08051154.bin
	@python3 tools/compare_slice.py baserom.gba 0x51154 build/core/band_08051154.bin

build/world/band_080534a8.bin: src/world/band_080534a8.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-band-080534a8-match: verify-rom build/world/band_080534a8.bin
	@python3 tools/compare_slice.py baserom.gba 0x534a8 build/world/band_080534a8.bin

build/world/band_0803afbc.bin: src/world/band_0803afbc.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-band-0803afbc-match: verify-rom build/world/band_0803afbc.bin
	@python3 tools/compare_slice.py baserom.gba 0x3afbc build/world/band_0803afbc.bin

build/misc/format_decimal_dup.bin: src/misc/format_decimal_dup.c data/functions.csv data/ram_map.csv
	@mkdir -p build/misc
	@python3 tools/build_c.py $< $@

misc-format-decimal-dup-match: verify-rom build/misc/format_decimal_dup.bin
	@python3 tools/compare_slice.py baserom.gba 0x674b0 build/misc/format_decimal_dup.bin

build/world/band_0804aeac.bin: src/world/band_0804aeac.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-band-0804aeac-match: verify-rom build/world/band_0804aeac.bin
	@python3 tools/compare_slice.py baserom.gba 0x4aeac build/world/band_0804aeac.bin

build/world/band_08064f24.bin: src/world/band_08064f24.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-band-08064f24-match: verify-rom build/world/band_08064f24.bin
	@python3 tools/compare_slice.py baserom.gba 0x64f24 build/world/band_08064f24.bin

build/world/band_a_315f8.bin: src/world/band_a_315f8.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-band-a-315f8-match: verify-rom build/world/band_a_315f8.bin
	@python3 tools/compare_slice.py baserom.gba 0x315f8 build/world/band_a_315f8.bin

build/world/band_a_31bf4.bin: src/world/band_a_31bf4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-band-a-31bf4-match: verify-rom build/world/band_a_31bf4.bin
	@python3 tools/compare_slice.py baserom.gba 0x31bf4 build/world/band_a_31bf4.bin

build/world/band_a_30b50.bin: src/world/band_a_30b50.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-band-a-30b50-match: verify-rom build/world/band_a_30b50.bin
	@python3 tools/compare_slice.py baserom.gba 0x30b50 build/world/band_a_30b50.bin

build/world/band_a_30f78.bin: src/world/band_a_30f78.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-band-a-30f78-match: verify-rom build/world/band_a_30f78.bin
	@python3 tools/compare_slice.py baserom.gba 0x30f78 build/world/band_a_30f78.bin

build/world/band_a_31534.bin: src/world/band_a_31534.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-band-a-31534-match: verify-rom build/world/band_a_31534.bin
	@python3 tools/compare_slice.py baserom.gba 0x31534 build/world/band_a_31534.bin

build/world/band_a_317f0.bin: src/world/band_a_317f0.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-band-a-317f0-match: verify-rom build/world/band_a_317f0.bin
	@python3 tools/compare_slice.py baserom.gba 0x317f0 build/world/band_a_317f0.bin

build/world/actor_desc_resolve.bin: src/world/actor_desc_resolve.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-actor-desc-resolve-match: verify-rom build/world/actor_desc_resolve.bin
	@python3 tools/compare_slice.py baserom.gba 0x1952c build/world/actor_desc_resolve.bin

build/world/actor_id_matches.bin: src/world/actor_id_matches.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-actor-id-matches-match: verify-rom build/world/actor_id_matches.bin
	@python3 tools/compare_slice.py baserom.gba 0x19620 build/world/actor_id_matches.bin

build/world/release_entry_bc.bin: src/world/release_entry_bc.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-release-entry-bc-match: verify-rom build/world/release_entry_bc.bin
	@python3 tools/compare_slice.py baserom.gba 0x28b2c build/world/release_entry_bc.bin

build/world/release_entry_d.bin: src/world/release_entry_d.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-release-entry-d-match: verify-rom build/world/release_entry_d.bin
	@python3 tools/compare_slice.py baserom.gba 0x28dc4 build/world/release_entry_d.bin

build/world/get_record16.bin: src/world/get_record16.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-get-record16-match: verify-rom build/world/get_record16.bin
	@python3 tools/compare_slice.py baserom.gba 0xdb60 build/world/get_record16.bin

build/video/setup_bg0_bg1.bin: src/video/setup_bg0_bg1.c data/functions.csv data/ram_map.csv
	@mkdir -p build/video
	@python3 tools/build_c.py $< $@

video-setup-bg0-bg1-match: verify-rom build/video/setup_bg0_bg1.bin
	@python3 tools/compare_slice.py baserom.gba 0x127a8 build/video/setup_bg0_bg1.bin

build/world/stop_audio_dma.bin: src/world/stop_audio_dma.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-stop-audio-dma-match: verify-rom build/world/stop_audio_dma.bin
	@python3 tools/compare_slice.py baserom.gba 0x337a8 build/world/stop_audio_dma.bin

build/world/send_text_mode1.bin: src/world/send_text_mode1.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-send-text-mode1-match: verify-rom build/world/send_text_mode1.bin
	@python3 tools/compare_slice.py baserom.gba 0x30b34 build/world/send_text_mode1.bin

build/ui/send_text_by_id.bin: src/ui/send_text_by_id.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

ui-send-text-by-id-match: verify-rom build/ui/send_text_by_id.bin
	@python3 tools/compare_slice.py baserom.gba 0x30b40 build/ui/send_text_by_id.bin

build/ui/load_hud_palettes.bin: src/ui/load_hud_palettes.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

ui-load-hud-palettes-match: verify-rom build/ui/load_hud_palettes.bin
	@python3 tools/compare_slice.py baserom.gba 0x30eac build/ui/load_hud_palettes.bin

build/ui/clear_hud_field_c.bin: src/ui/clear_hud_field_c.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

ui-clear-hud-field-c-match: verify-rom build/ui/clear_hud_field_c.bin
	@python3 tools/compare_slice.py baserom.gba 0x30f50 build/ui/clear_hud_field_c.bin

build/world/trigger_event39.bin: src/world/trigger_event39.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-trigger-event39-match: verify-rom build/world/trigger_event39.bin
	@python3 tools/compare_slice.py baserom.gba 0x30f84 build/world/trigger_event39.bin

build/ui/clear_map_window.bin: src/ui/clear_map_window.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

ui-clear-map-window-match: verify-rom build/ui/clear_map_window.bin
	@python3 tools/compare_slice.py baserom.gba 0x311dc build/ui/clear_map_window.bin

build/ui/clear_map_window_dma.bin: src/ui/clear_map_window_dma.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

ui-clear-map-window-dma-match: verify-rom build/ui/clear_map_window_dma.bin
	@python3 tools/compare_slice.py baserom.gba 0x31328 build/ui/clear_map_window_dma.bin

build/world/scale_magnitude.bin: src/world/scale_magnitude.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-scale-magnitude-match: verify-rom build/world/scale_magnitude.bin
	@python3 tools/compare_slice.py baserom.gba 0x31414 build/world/scale_magnitude.bin

build/ui/draw_two_digits.bin: src/ui/draw_two_digits.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

ui-draw-two-digits-match: verify-rom build/ui/draw_two_digits.bin
	@python3 tools/compare_slice.py baserom.gba 0x31498 build/ui/draw_two_digits.bin

build/ui/clear_hud_rows_ab.bin: src/ui/clear_hud_rows_ab.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

ui-clear-hud-rows-ab-match: verify-rom build/ui/clear_hud_rows_ab.bin
	@python3 tools/compare_slice.py baserom.gba 0x31578 build/ui/clear_hud_rows_ab.bin

build/ui/send_text_mode2.bin: src/ui/send_text_mode2.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

ui-send-text-mode2-match: verify-rom build/ui/send_text_mode2.bin
	@python3 tools/compare_slice.py baserom.gba 0x315c0 build/ui/send_text_mode2.bin

build/world/release_actor_and_slot.bin: src/world/release_actor_and_slot.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-release-actor-and-slot-match: verify-rom build/world/release_actor_and_slot.bin
	@python3 tools/compare_slice.py baserom.gba 0x31618 build/world/release_actor_and_slot.bin

build/world/get_record_node_by_id.bin: src/world/get_record_node_by_id.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-get-record-node-by-id-match: verify-rom build/world/get_record_node_by_id.bin
	@python3 tools/compare_slice.py baserom.gba 0x31318 build/world/get_record_node_by_id.bin

build/video/blit_strip_4bpp.bin: src/video/blit_strip_4bpp.c data/functions.csv data/ram_map.csv
	@mkdir -p build/video
	@python3 tools/build_c.py $< $@

video-blit-strip-4bpp-match: verify-rom build/video/blit_strip_4bpp.bin
	@python3 tools/compare_slice.py baserom.gba 0x31a1c build/video/blit_strip_4bpp.bin

build/world/push_slot_queue_entry.bin: src/world/push_slot_queue_entry.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-push-slot-queue-entry-match: verify-rom build/world/push_slot_queue_entry.bin
	@python3 tools/compare_slice.py baserom.gba 0x31c98 build/world/push_slot_queue_entry.bin

build/video/blit_strip_plain.bin: src/video/blit_strip_plain.c data/functions.csv data/ram_map.csv
	@mkdir -p build/video
	@python3 tools/build_c.py $< $@

video-blit-strip-plain-match: verify-rom build/video/blit_strip_plain.bin
	@python3 tools/compare_slice.py baserom.gba 0x31684 build/video/blit_strip_plain.bin

build/video/blit_strip_clip_left.bin: src/video/blit_strip_clip_left.c data/functions.csv data/ram_map.csv
	@mkdir -p build/video
	@python3 tools/build_c.py $< $@

video-blit-strip-clip-left-match: verify-rom build/video/blit_strip_clip_left.bin
	@python3 tools/compare_slice.py baserom.gba 0x31844 build/video/blit_strip_clip_left.bin

build/world/release_entry_e.bin: src/world/release_entry_e.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-release-entry-e-match: verify-rom build/world/release_entry_e.bin
	@python3 tools/compare_slice.py baserom.gba 0x294b4 build/world/release_entry_e.bin

build/world/release_slot_held.bin: src/world/release_slot_held.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-release-slot-held-match: verify-rom build/world/release_slot_held.bin
	@python3 tools/compare_slice.py baserom.gba 0x31658 build/world/release_slot_held.bin

build/core/find_free_node.bin: src/core/find_free_node.c data/functions.csv data/ram_map.csv
	@mkdir -p build/core
	@python3 tools/build_c.py $< $@

core-find-free-node-match: verify-rom build/core/find_free_node.bin
	@python3 tools/compare_slice.py baserom.gba 0x55954 build/core/find_free_node.bin

build/world/actor_behavior_steps.bin: src/world/actor_behavior_steps.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-actor-behavior-steps-match: verify-rom build/world/actor_behavior_steps.bin
	@python3 tools/compare_slice.py baserom.gba 0x17e3c build/world/actor_behavior_steps.bin

build/world/facing_target.bin: src/world/facing_target.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-facing-target-match: verify-rom build/world/facing_target.bin
	@python3 tools/compare_slice.py baserom.gba 0x17500 build/world/facing_target.bin

build/world/queue_draw_request.bin: src/world/queue_draw_request.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-queue-draw-request-match: verify-rom build/world/queue_draw_request.bin
	@python3 tools/compare_slice.py baserom.gba 0x12f88 build/world/queue_draw_request.bin

build/ui/draw_eight_digits.bin: src/ui/draw_eight_digits.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

ui-draw-eight-digits-match: verify-rom build/ui/draw_eight_digits.bin
	@python3 tools/compare_slice.py baserom.gba 0x2a610 build/ui/draw_eight_digits.bin

build/world/mark_distant_actors.bin: src/world/mark_distant_actors.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-mark-distant-actors-match: verify-rom build/world/mark_distant_actors.bin
	@python3 tools/compare_slice.py baserom.gba 0x611cc build/world/mark_distant_actors.bin

build/world/queue_draw_request_direct.bin: src/world/queue_draw_request_direct.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-queue-draw-request-direct-match: verify-rom build/world/queue_draw_request_direct.bin
	@python3 tools/compare_slice.py baserom.gba 0x12e18 build/world/queue_draw_request_direct.bin

build/world/init_session_attr.bin: src/world/init_session_attr.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-init-session-attr-match: verify-rom build/world/init_session_attr.bin
	@python3 tools/compare_slice.py baserom.gba 0x31388 build/world/init_session_attr.bin

build/video/setup_bg0_bg1_menu.bin: src/video/setup_bg0_bg1_menu.c data/functions.csv data/ram_map.csv
	@mkdir -p build/video
	@python3 tools/build_c.py $< $@

video-setup-bg0-bg1-menu-match: verify-rom build/video/setup_bg0_bg1_menu.bin
	@python3 tools/compare_slice.py baserom.gba 0x12750 build/video/setup_bg0_bg1_menu.bin

build/world/random_below_count.bin: src/world/random_below_count.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-random-below-count-match: verify-rom build/world/random_below_count.bin
	@python3 tools/compare_slice.py baserom.gba 0x646ac build/world/random_below_count.bin

build/world/reset_track_heading.bin: src/world/reset_track_heading.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-reset-track-heading-match: verify-rom build/world/reset_track_heading.bin
	@python3 tools/compare_slice.py baserom.gba 0xaaf4 build/world/reset_track_heading.bin

build/world/add_to_counter.bin: src/world/add_to_counter.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-add-to-counter-match: verify-rom build/world/add_to_counter.bin
	@python3 tools/compare_slice.py baserom.gba 0x33824 build/world/add_to_counter.bin

build/world/spawn_at_slot.bin: src/world/spawn_at_slot.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-spawn-at-slot-match: verify-rom build/world/spawn_at_slot.bin
	@python3 tools/compare_slice.py baserom.gba 0x28ce8 build/world/spawn_at_slot.bin

build/world/ping_other_actors.bin: src/world/ping_other_actors.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-ping-other-actors-match: verify-rom build/world/ping_other_actors.bin
	@python3 tools/compare_slice.py baserom.gba 0x19710 build/world/ping_other_actors.bin

build/world/flags_to_angle.bin: src/world/flags_to_angle.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-flags-to-angle-match: verify-rom build/world/flags_to_angle.bin
	@python3 tools/compare_slice.py baserom.gba 0x17d78 build/world/flags_to_angle.bin

build/world/spawn_selected_for_players.bin: src/world/spawn_selected_for_players.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-spawn-selected-for-players-match: verify-rom build/world/spawn_selected_for_players.bin
	@python3 tools/compare_slice.py baserom.gba 0x54e40 build/world/spawn_selected_for_players.bin

build/world/is_kind20.bin: src/world/is_kind20.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-is-kind20-match: verify-rom build/world/is_kind20.bin
	@python3 tools/compare_slice.py baserom.gba 0x67404 build/world/is_kind20.bin

build/world/alloc_from_counter.bin: src/world/alloc_from_counter.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-alloc-from-counter-match: verify-rom build/world/alloc_from_counter.bin
	@python3 tools/compare_slice.py baserom.gba 0x3385c build/world/alloc_from_counter.bin

build/world/finish_actor_state.bin: src/world/finish_actor_state.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-finish-actor-state-match: verify-rom build/world/finish_actor_state.bin
	@python3 tools/compare_slice.py baserom.gba 0x1979c build/world/finish_actor_state.bin

build/world/refresh_score_digits.bin: src/world/refresh_score_digits.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-refresh-score-digits-match: verify-rom build/world/refresh_score_digits.bin
	@python3 tools/compare_slice.py baserom.gba 0x31294 build/world/refresh_score_digits.bin

build/world/get_actor_route_entry.bin: src/world/get_actor_route_entry.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-get-actor-route-entry-match: verify-rom build/world/get_actor_route_entry.bin
	@python3 tools/compare_slice.py baserom.gba 0x193c4 build/world/get_actor_route_entry.bin

build/world/release_object.bin: src/world/release_object.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-release-object-match: verify-rom build/world/release_object.bin
	@python3 tools/compare_slice.py baserom.gba 0x13abc build/world/release_object.bin

build/world/find_random_tile_of_kind.bin: src/world/find_random_tile_of_kind.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-find-random-tile-of-kind-match: verify-rom build/world/find_random_tile_of_kind.bin
	@python3 tools/compare_slice.py baserom.gba 0x424ec build/world/find_random_tile_of_kind.bin

build/world/alloc_draw_entry.bin: src/world/alloc_draw_entry.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-alloc-draw-entry-match: verify-rom build/world/alloc_draw_entry.bin
	@python3 tools/compare_slice.py baserom.gba 0x12e78 build/world/alloc_draw_entry.bin

build/world/reset_map_view.bin: src/world/reset_map_view.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-reset-map-view-match: verify-rom build/world/reset_map_view.bin
	@python3 tools/compare_slice.py baserom.gba 0x30f90 build/world/reset_map_view.bin

build/world/try_launch_actor.bin: src/world/try_launch_actor.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-try-launch-actor-match: verify-rom build/world/try_launch_actor.bin
	@python3 tools/compare_slice.py baserom.gba 0x157b8 build/world/try_launch_actor.bin

build/world/release_actor_entries.bin: src/world/release_actor_entries.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-release-actor-entries-match: verify-rom build/world/release_actor_entries.bin
	@python3 tools/compare_slice.py baserom.gba 0x16808 build/world/release_actor_entries.bin

build/world/is_player_actor_ready.bin: src/world/is_player_actor_ready.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-is-player-actor-ready-match: verify-rom build/world/is_player_actor_ready.bin
	@python3 tools/compare_slice.py baserom.gba 0x3c62c build/world/is_player_actor_ready.bin

build/ui/init_menu_session.bin: src/ui/init_menu_session.c data/functions.csv data/ram_map.csv
	@mkdir -p build/ui
	@python3 tools/build_c.py $< $@

ui-init-menu-session-match: verify-rom build/ui/init_menu_session.bin
	@python3 tools/compare_slice.py baserom.gba 0x320e0 build/ui/init_menu_session.bin

build/world/is_near_any_actor.bin: src/world/is_near_any_actor.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-is-near-any-actor-match: verify-rom build/world/is_near_any_actor.bin
	@python3 tools/compare_slice.py baserom.gba 0x54f1c build/world/is_near_any_actor.bin

build/world/project_actor_offset.bin: src/world/project_actor_offset.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-project-actor-offset-match: verify-rom build/world/project_actor_offset.bin
	@python3 tools/compare_slice.py baserom.gba 0x23868 build/world/project_actor_offset.bin

build/world/update_actor_lane.bin: src/world/update_actor_lane.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-update-actor-lane-match: verify-rom build/world/update_actor_lane.bin
	@python3 tools/compare_slice.py baserom.gba 0x159a0 build/world/update_actor_lane.bin

build/world/p1_23778.bin: src/world/p1_23778.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-23778-match: verify-rom build/world/p1_23778.bin
	@python3 tools/compare_slice.py baserom.gba 0x23778 build/world/p1_23778.bin

build/world/p1_23794.bin: src/world/p1_23794.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-23794-match: verify-rom build/world/p1_23794.bin
	@python3 tools/compare_slice.py baserom.gba 0x23794 build/world/p1_23794.bin

build/world/p1_5b2a4.bin: src/world/p1_5b2a4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-5b2a4-match: verify-rom build/world/p1_5b2a4.bin
	@python3 tools/compare_slice.py baserom.gba 0x5b2a4 build/world/p1_5b2a4.bin

build/world/p1_5b854.bin: src/world/p1_5b854.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-5b854-match: verify-rom build/world/p1_5b854.bin
	@python3 tools/compare_slice.py baserom.gba 0x5b854 build/world/p1_5b854.bin

build/world/p1_63c88.bin: src/world/p1_63c88.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-63c88-match: verify-rom build/world/p1_63c88.bin
	@python3 tools/compare_slice.py baserom.gba 0x63c88 build/world/p1_63c88.bin

build/world/p1_63d3c.bin: src/world/p1_63d3c.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-63d3c-match: verify-rom build/world/p1_63d3c.bin
	@python3 tools/compare_slice.py baserom.gba 0x63d3c build/world/p1_63d3c.bin

build/world/p1_4fee4.bin: src/world/p1_4fee4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-4fee4-match: verify-rom build/world/p1_4fee4.bin
	@python3 tools/compare_slice.py baserom.gba 0x4fee4 build/world/p1_4fee4.bin

build/world/p1_500dc.bin: src/world/p1_500dc.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-500dc-match: verify-rom build/world/p1_500dc.bin
	@python3 tools/compare_slice.py baserom.gba 0x500dc build/world/p1_500dc.bin

build/world/p1_3f7f8.bin: src/world/p1_3f7f8.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-3f7f8-match: verify-rom build/world/p1_3f7f8.bin
	@python3 tools/compare_slice.py baserom.gba 0x3f7f8 build/world/p1_3f7f8.bin

build/world/p1_3f85c.bin: src/world/p1_3f85c.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-3f85c-match: verify-rom build/world/p1_3f85c.bin
	@python3 tools/compare_slice.py baserom.gba 0x3f85c build/world/p1_3f85c.bin

build/world/p1_35d60.bin: src/world/p1_35d60.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-35d60-match: verify-rom build/world/p1_35d60.bin
	@python3 tools/compare_slice.py baserom.gba 0x35d60 build/world/p1_35d60.bin

build/world/p1_35d9c.bin: src/world/p1_35d9c.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-35d9c-match: verify-rom build/world/p1_35d9c.bin
	@python3 tools/compare_slice.py baserom.gba 0x35d9c build/world/p1_35d9c.bin

build/world/p1_32548.bin: src/world/p1_32548.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-32548-match: verify-rom build/world/p1_32548.bin
	@python3 tools/compare_slice.py baserom.gba 0x32548 build/world/p1_32548.bin

build/world/p1_3258c.bin: src/world/p1_3258c.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-3258c-match: verify-rom build/world/p1_3258c.bin
	@python3 tools/compare_slice.py baserom.gba 0x3258c build/world/p1_3258c.bin

build/world/p1_35168.bin: src/world/p1_35168.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-35168-match: verify-rom build/world/p1_35168.bin
	@python3 tools/compare_slice.py baserom.gba 0x35168 build/world/p1_35168.bin

build/world/p1_351ac.bin: src/world/p1_351ac.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-351ac-match: verify-rom build/world/p1_351ac.bin
	@python3 tools/compare_slice.py baserom.gba 0x351ac build/world/p1_351ac.bin

build/world/p1_3f8dc.bin: src/world/p1_3f8dc.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-3f8dc-match: verify-rom build/world/p1_3f8dc.bin
	@python3 tools/compare_slice.py baserom.gba 0x3f8dc build/world/p1_3f8dc.bin

build/world/p1_3f910.bin: src/world/p1_3f910.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-3f910-match: verify-rom build/world/p1_3f910.bin
	@python3 tools/compare_slice.py baserom.gba 0x3f910 build/world/p1_3f910.bin

build/world/p1_59fe8.bin: src/world/p1_59fe8.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-59fe8-match: verify-rom build/world/p1_59fe8.bin
	@python3 tools/compare_slice.py baserom.gba 0x59fe8 build/world/p1_59fe8.bin

build/world/p1_5a070.bin: src/world/p1_5a070.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-5a070-match: verify-rom build/world/p1_5a070.bin
	@python3 tools/compare_slice.py baserom.gba 0x5a070 build/world/p1_5a070.bin

build/world/p1_5abdc.bin: src/world/p1_5abdc.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-5abdc-match: verify-rom build/world/p1_5abdc.bin
	@python3 tools/compare_slice.py baserom.gba 0x5abdc build/world/p1_5abdc.bin

build/world/p1_5ac10.bin: src/world/p1_5ac10.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-5ac10-match: verify-rom build/world/p1_5ac10.bin
	@python3 tools/compare_slice.py baserom.gba 0x5ac10 build/world/p1_5ac10.bin

build/world/p1_0dd50.bin: src/world/p1_0dd50.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-0dd50-match: verify-rom build/world/p1_0dd50.bin
	@python3 tools/compare_slice.py baserom.gba 0xdd50 build/world/p1_0dd50.bin

build/world/p1_357cc.bin: src/world/p1_357cc.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-357cc-match: verify-rom build/world/p1_357cc.bin
	@python3 tools/compare_slice.py baserom.gba 0x357cc build/world/p1_357cc.bin

build/world/p1_5ab90.bin: src/world/p1_5ab90.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-5ab90-match: verify-rom build/world/p1_5ab90.bin
	@python3 tools/compare_slice.py baserom.gba 0x5ab90 build/world/p1_5ab90.bin

build/world/p1_5aefc.bin: src/world/p1_5aefc.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-5aefc-match: verify-rom build/world/p1_5aefc.bin
	@python3 tools/compare_slice.py baserom.gba 0x5aefc build/world/p1_5aefc.bin

build/world/p1_35de4.bin: src/world/p1_35de4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-35de4-match: verify-rom build/world/p1_35de4.bin
	@python3 tools/compare_slice.py baserom.gba 0x35de4 build/world/p1_35de4.bin

build/world/p1_35e38.bin: src/world/p1_35e38.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-35e38-match: verify-rom build/world/p1_35e38.bin
	@python3 tools/compare_slice.py baserom.gba 0x35e38 build/world/p1_35e38.bin

build/world/p1_13450.bin: src/world/p1_13450.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-13450-match: verify-rom build/world/p1_13450.bin
	@python3 tools/compare_slice.py baserom.gba 0x13450 build/world/p1_13450.bin

build/world/p1_134a4.bin: src/world/p1_134a4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-134a4-match: verify-rom build/world/p1_134a4.bin
	@python3 tools/compare_slice.py baserom.gba 0x134a4 build/world/p1_134a4.bin

build/world/p1_6245c.bin: src/world/p1_6245c.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-6245c-match: verify-rom build/world/p1_6245c.bin
	@python3 tools/compare_slice.py baserom.gba 0x6245c build/world/p1_6245c.bin

build/world/p1_624b8.bin: src/world/p1_624b8.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-624b8-match: verify-rom build/world/p1_624b8.bin
	@python3 tools/compare_slice.py baserom.gba 0x624b8 build/world/p1_624b8.bin

build/world/p1_350a8.bin: src/world/p1_350a8.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-350a8-match: verify-rom build/world/p1_350a8.bin
	@python3 tools/compare_slice.py baserom.gba 0x350a8 build/world/p1_350a8.bin

build/world/p1_35230.bin: src/world/p1_35230.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-35230-match: verify-rom build/world/p1_35230.bin
	@python3 tools/compare_slice.py baserom.gba 0x35230 build/world/p1_35230.bin

build/world/p1_5ad10.bin: src/world/p1_5ad10.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-5ad10-match: verify-rom build/world/p1_5ad10.bin
	@python3 tools/compare_slice.py baserom.gba 0x5ad10 build/world/p1_5ad10.bin

build/world/p1_5ad68.bin: src/world/p1_5ad68.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-5ad68-match: verify-rom build/world/p1_5ad68.bin
	@python3 tools/compare_slice.py baserom.gba 0x5ad68 build/world/p1_5ad68.bin

build/world/p1_25fc0.bin: src/world/p1_25fc0.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-25fc0-match: verify-rom build/world/p1_25fc0.bin
	@python3 tools/compare_slice.py baserom.gba 0x25fc0 build/world/p1_25fc0.bin

build/world/p1_26034.bin: src/world/p1_26034.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-26034-match: verify-rom build/world/p1_26034.bin
	@python3 tools/compare_slice.py baserom.gba 0x26034 build/world/p1_26034.bin

build/world/p1_3c268.bin: src/world/p1_3c268.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-3c268-match: verify-rom build/world/p1_3c268.bin
	@python3 tools/compare_slice.py baserom.gba 0x3c268 build/world/p1_3c268.bin

build/world/p1_3c2c4.bin: src/world/p1_3c2c4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-3c2c4-match: verify-rom build/world/p1_3c2c4.bin
	@python3 tools/compare_slice.py baserom.gba 0x3c2c4 build/world/p1_3c2c4.bin

build/world/p1_6ca24.bin: src/world/p1_6ca24.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-6ca24-match: verify-rom build/world/p1_6ca24.bin
	@python3 tools/compare_slice.py baserom.gba 0x6ca24 build/world/p1_6ca24.bin

build/world/p1_6ca70.bin: src/world/p1_6ca70.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-6ca70-match: verify-rom build/world/p1_6ca70.bin
	@python3 tools/compare_slice.py baserom.gba 0x6ca70 build/world/p1_6ca70.bin

build/world/p1_6cabc.bin: src/world/p1_6cabc.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-6cabc-match: verify-rom build/world/p1_6cabc.bin
	@python3 tools/compare_slice.py baserom.gba 0x6cabc build/world/p1_6cabc.bin

build/world/p1_6cb08.bin: src/world/p1_6cb08.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-6cb08-match: verify-rom build/world/p1_6cb08.bin
	@python3 tools/compare_slice.py baserom.gba 0x6cb08 build/world/p1_6cb08.bin

build/world/p1_6cb54.bin: src/world/p1_6cb54.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-6cb54-match: verify-rom build/world/p1_6cb54.bin
	@python3 tools/compare_slice.py baserom.gba 0x6cb54 build/world/p1_6cb54.bin

build/world/p1_6cba0.bin: src/world/p1_6cba0.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-6cba0-match: verify-rom build/world/p1_6cba0.bin
	@python3 tools/compare_slice.py baserom.gba 0x6cba0 build/world/p1_6cba0.bin

build/world/p1_6d810.bin: src/world/p1_6d810.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-6d810-match: verify-rom build/world/p1_6d810.bin
	@python3 tools/compare_slice.py baserom.gba 0x6d810 build/world/p1_6d810.bin

build/world/p1_6d860.bin: src/world/p1_6d860.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-6d860-match: verify-rom build/world/p1_6d860.bin
	@python3 tools/compare_slice.py baserom.gba 0x6d860 build/world/p1_6d860.bin

build/world/p1_6d8b0.bin: src/world/p1_6d8b0.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-6d8b0-match: verify-rom build/world/p1_6d8b0.bin
	@python3 tools/compare_slice.py baserom.gba 0x6d8b0 build/world/p1_6d8b0.bin

build/world/p1_6d900.bin: src/world/p1_6d900.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-6d900-match: verify-rom build/world/p1_6d900.bin
	@python3 tools/compare_slice.py baserom.gba 0x6d900 build/world/p1_6d900.bin

build/world/p1_6d950.bin: src/world/p1_6d950.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-6d950-match: verify-rom build/world/p1_6d950.bin
	@python3 tools/compare_slice.py baserom.gba 0x6d950 build/world/p1_6d950.bin

build/world/p1_6d9a0.bin: src/world/p1_6d9a0.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-6d9a0-match: verify-rom build/world/p1_6d9a0.bin
	@python3 tools/compare_slice.py baserom.gba 0x6d9a0 build/world/p1_6d9a0.bin

build/world/p1_716e8.bin: src/world/p1_716e8.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-716e8-match: verify-rom build/world/p1_716e8.bin
	@python3 tools/compare_slice.py baserom.gba 0x716e8 build/world/p1_716e8.bin

build/world/p1_71d20.bin: src/world/p1_71d20.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-71d20-match: verify-rom build/world/p1_71d20.bin
	@python3 tools/compare_slice.py baserom.gba 0x71d20 build/world/p1_71d20.bin

build/world/p1_71c90.bin: src/world/p1_71c90.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-71c90-match: verify-rom build/world/p1_71c90.bin
	@python3 tools/compare_slice.py baserom.gba 0x71c90 build/world/p1_71c90.bin

build/world/p1_71db4.bin: src/world/p1_71db4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-71db4-match: verify-rom build/world/p1_71db4.bin
	@python3 tools/compare_slice.py baserom.gba 0x71db4 build/world/p1_71db4.bin

build/world/p1_71de8.bin: src/world/p1_71de8.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-71de8-match: verify-rom build/world/p1_71de8.bin
	@python3 tools/compare_slice.py baserom.gba 0x71de8 build/world/p1_71de8.bin

build/world/p1_58140.bin: src/world/p1_58140.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-58140-match: verify-rom build/world/p1_58140.bin
	@python3 tools/compare_slice.py baserom.gba 0x58140 build/world/p1_58140.bin

build/world/p1_581b4.bin: src/world/p1_581b4.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-581b4-match: verify-rom build/world/p1_581b4.bin
	@python3 tools/compare_slice.py baserom.gba 0x581b4 build/world/p1_581b4.bin

build/world/p1_5b388.bin: src/world/p1_5b388.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-5b388-match: verify-rom build/world/p1_5b388.bin
	@python3 tools/compare_slice.py baserom.gba 0x5b388 build/world/p1_5b388.bin

build/world/p1_5b484.bin: src/world/p1_5b484.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-5b484-match: verify-rom build/world/p1_5b484.bin
	@python3 tools/compare_slice.py baserom.gba 0x5b484 build/world/p1_5b484.bin

build/world/p1_2a9f0.bin: src/world/p1_2a9f0.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-2a9f0-match: verify-rom build/world/p1_2a9f0.bin
	@python3 tools/compare_slice.py baserom.gba 0x2a9f0 build/world/p1_2a9f0.bin

build/world/p1_2ab18.bin: src/world/p1_2ab18.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-2ab18-match: verify-rom build/world/p1_2ab18.bin
	@python3 tools/compare_slice.py baserom.gba 0x2ab18 build/world/p1_2ab18.bin

build/world/p1_130f0.bin: src/world/p1_130f0.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-130f0-match: verify-rom build/world/p1_130f0.bin
	@python3 tools/compare_slice.py baserom.gba 0x130f0 build/world/p1_130f0.bin

build/world/p1_427bc.bin: src/world/p1_427bc.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-427bc-match: verify-rom build/world/p1_427bc.bin
	@python3 tools/compare_slice.py baserom.gba 0x427bc build/world/p1_427bc.bin

build/world/p1_51048.bin: src/world/p1_51048.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-51048-match: verify-rom build/world/p1_51048.bin
	@python3 tools/compare_slice.py baserom.gba 0x51048 build/world/p1_51048.bin

build/world/p1_519b8.bin: src/world/p1_519b8.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-519b8-match: verify-rom build/world/p1_519b8.bin
	@python3 tools/compare_slice.py baserom.gba 0x519b8 build/world/p1_519b8.bin

build/world/p1_35844.bin: src/world/p1_35844.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-35844-match: verify-rom build/world/p1_35844.bin
	@python3 tools/compare_slice.py baserom.gba 0x35844 build/world/p1_35844.bin

build/world/p1_30848.bin: src/world/p1_30848.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-30848-match: verify-rom build/world/p1_30848.bin
	@python3 tools/compare_slice.py baserom.gba 0x30848 build/world/p1_30848.bin

build/world/p1_5131c.bin: src/world/p1_5131c.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-5131c-match: verify-rom build/world/p1_5131c.bin
	@python3 tools/compare_slice.py baserom.gba 0x5131c build/world/p1_5131c.bin

build/world/p1_31204.bin: src/world/p1_31204.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-31204-match: verify-rom build/world/p1_31204.bin
	@python3 tools/compare_slice.py baserom.gba 0x31204 build/world/p1_31204.bin

build/world/p1_3c708.bin: src/world/p1_3c708.c data/functions.csv data/ram_map.csv
	@mkdir -p build/world
	@python3 tools/build_c.py $< $@

world-p1-3c708-match: verify-rom build/world/p1_3c708.bin
	@python3 tools/compare_slice.py baserom.gba 0x3c708 build/world/p1_3c708.bin

matching: libc-verify bootstrap-match intr-match init-interrupts-match game-init-match vblank-match irq-helpers-match reset-display-match init-save-system-match read-eeprom-match write-eeprom-match save-slots-match save-wrappers-match save-manager-match read-eeprom-range-match write-eeprom-range-match save-helpers-match menu-layout-match draw-menu-match init-menu-screen-match menu-helpers-match menu-graphics-match world-entity-accessors-match misc-state-getters-match misc-table-lookup-match misc-record-table-match misc-session-reset-match text-draw-text-match world-entity-flags-match misc-session-node-match ui-menu-loop-match world-map-tiles-match bios-match misc-coord-accessors-match world-object-helpers-match core-linked-list-match world-object-state-match world-actor-states-match world-slot-config-match world-slot-table-match world-stat-counters-match world-slot-query-match world-node-search-match world-actor-control-match world-area-flags-match world-object-value-match world-table-entries-match world-pause-helpers-match world-slot-selectors-match world-list-head-match world-more-counters-match world-pool-gets-match world-threshold-match world-state-init-match world-gRam02030330-gets-match world-slot-scan-match world-list-ops-match world-map-tile-fields-match world-comm-flag-match world-pair-lookup-match world-tile-and-map-match world-anchor-reset-match world-slot-range-match world-word-compare-match world-ram-flags-match world-more-counters-2-match world-more-counters-3-match core-list-ops2-match world-ram-state-match world-window-config-match misc-coord-more-match world-id-verify-match world-flag-arrays-match world-scan-active-match world-actor-check-match world-frame-chain-match world-dma-flush-match world-slot-release-match world-counter-saturate-match world-set-index-match world-submit-object-match world-scan-all-match world-maybe-advance-match world-entity-query-match world-object-query-match world-get-inner-id-match world-is-ram-mode-match world-offset-helpers-match world-actor-init-match world-kind-scan-match world-history-push-match world-distance-accum-match world-bump-or-reset-match world-release-slot-match world-pool-first-match world-copy-flag-byte-match world-get-slot-unk10-match world-is-mode-two-match world-submit-pack-match core-read-triple-match world-notify-if-ready-match world-bump-save-counter-match world-masked-compare-match world-get-anchor-unk34-match world-forward-with-zero-match world-zero-three-flags-match world-clear-bg1-enable-match world-clear-flag-notify-match world-select-word-source-match world-zero-two-blocks-match world-init-handler-pack-match world-retarget-if-kind4-match world-step-then-check-match world-drain-two-chains-match world-forward-zero-arg4-match world-forward-zero-arg2-match world-lookup-then-call-match world-reset-session-flags-match core-zero-history-match core-clear-coord-byte71-match world-wrap-08005fc4-match world-wrap-080101d8-match world-wrap-08012a98-match world-wrap-080138e8-match world-wrap-080138f4-match world-wrap-08014fa0-match world-wrap-0803378c-match world-wrap-0803379c-match world-wrap-08035dd8-match world-wrap-08042784-match world-wrap-080428ac-match world-wrap-0804fb4c-match world-get-field-100-match world-spin-delay-match world-wrap-0804ff98-match world-wrap-0804ffb4-match world-wrap-080500d0-match world-wrap-08051464-match world-wrap-0805a4f4-match world-wrap-08063bcc-match world-get-anchor-unk04-match world-get-anchor-unk30-match world-wrap-08051480-match world-wrap-08059d7c-match world-get-record-index-match world-get-ram-word16-match world-is-state-two-match world-is-anchor-small-match world-rearm-slot-match world-accumulate-distance-match core-clip-bounds-match world-engage-actor-match core-insert-sorted-match world-refresh-then-notify-match world-check-current-entity-match core-for-each-node-match world-set-bg1-enable-match world-slot-range-or-field-match world-init-and-mirror-match world-bump-or-trigger-match world-apply-two-levels-match core-sort-sprite-list-match core-init-sprite-pool-match core-flush-sprite-list-match core-alloc-node-match core-sort-active-sprites-match core-insert-sprite-sorted-match core-memory-init-match world-link-objects-match world-nibble-replace-match world-palette-lerp-match core-reset-runtime-globals-match core-adjust-area-position-match core-frame-dispatch-match core-probe-nearby-position-match world-player-slot-query-match world-slot-range-query-match world-bump-count-82-match misc-format-decimal-match world-link-session-match world-link-hw-reset-match world-link-report-match world-actor-tracking-match world-scale-by-band-match world-link-init-match world-link-session-reset-match world-spawn-slot-effect-match world-bump-rank-counter-match world-bitmap-asset-match world-link-state-step-match world-link-dispatch-match world-link-frame-step-match world-link-menu-step-match world-rank-query-match world-slot-probe-match world-shutdown-reset-match world-band-0800b16c-match core-band-08051154-match world-band-080534a8-match world-band-0803afbc-match misc-format-decimal-dup-match world-band-0804aeac-match world-band-08064f24-match world-band-a-315f8-match world-band-a-31bf4-match world-band-a-30b50-match world-band-a-30f78-match world-band-a-31534-match world-band-a-317f0-match world-actor-desc-resolve-match world-actor-id-matches-match world-release-entry-bc-match world-release-entry-d-match world-get-record16-match video-setup-bg0-bg1-match world-stop-audio-dma-match world-send-text-mode1-match ui-send-text-by-id-match ui-load-hud-palettes-match ui-clear-hud-field-c-match world-trigger-event39-match ui-clear-map-window-match ui-clear-map-window-dma-match world-scale-magnitude-match ui-draw-two-digits-match ui-clear-hud-rows-ab-match ui-send-text-mode2-match world-release-actor-and-slot-match world-get-record-node-by-id-match video-blit-strip-4bpp-match world-push-slot-queue-entry-match video-blit-strip-plain-match video-blit-strip-clip-left-match world-release-entry-e-match world-release-slot-held-match core-find-free-node-match world-actor-behavior-steps-match world-facing-target-match world-queue-draw-request-match ui-draw-eight-digits-match world-mark-distant-actors-match world-queue-draw-request-direct-match world-init-session-attr-match video-setup-bg0-bg1-menu-match world-random-below-count-match world-reset-track-heading-match world-add-to-counter-match world-spawn-at-slot-match world-ping-other-actors-match world-flags-to-angle-match world-spawn-selected-for-players-match world-is-kind20-match world-alloc-from-counter-match world-finish-actor-state-match world-refresh-score-digits-match world-get-actor-route-entry-match world-release-object-match world-find-random-tile-of-kind-match world-alloc-draw-entry-match world-reset-map-view-match world-try-launch-actor-match world-release-actor-entries-match world-is-player-actor-ready-match ui-init-menu-session-match world-is-near-any-actor-match world-project-actor-offset-match world-update-actor-lane-match world-p1-23778-match world-p1-23794-match world-p1-5b2a4-match world-p1-5b854-match world-p1-63c88-match world-p1-63d3c-match world-p1-4fee4-match world-p1-500dc-match world-p1-3f7f8-match world-p1-3f85c-match world-p1-35d60-match world-p1-35d9c-match world-p1-32548-match world-p1-3258c-match world-p1-35168-match world-p1-351ac-match world-p1-3f8dc-match world-p1-3f910-match world-p1-59fe8-match world-p1-5a070-match world-p1-5abdc-match world-p1-5ac10-match world-p1-0dd50-match world-p1-357cc-match world-p1-5ab90-match world-p1-5aefc-match world-p1-35de4-match world-p1-35e38-match world-p1-13450-match world-p1-134a4-match world-p1-6245c-match world-p1-624b8-match world-p1-350a8-match world-p1-35230-match world-p1-5ad10-match world-p1-5ad68-match world-p1-25fc0-match world-p1-26034-match world-p1-3c268-match world-p1-3c2c4-match world-p1-6ca24-match world-p1-6ca70-match world-p1-6cabc-match world-p1-6cb08-match world-p1-6cb54-match world-p1-6cba0-match world-p1-6d810-match world-p1-6d860-match world-p1-6d8b0-match world-p1-6d900-match world-p1-6d950-match world-p1-6d9a0-match world-p1-716e8-match world-p1-71d20-match world-p1-71c90-match world-p1-71db4-match world-p1-71de8-match world-p1-58140-match world-p1-581b4-match world-p1-5b388-match world-p1-5b484-match world-p1-2a9f0-match world-p1-2ab18-match world-p1-130f0-match world-p1-427bc-match world-p1-51048-match world-p1-519b8-match world-p1-35844-match world-p1-30848-match world-p1-5131c-match world-p1-31204-match world-p1-3c708-match
	@python3 tools/verify_matching_regions.py

build/misc/id_compatibility.bin: src/misc/id_compatibility.c data/functions.csv data/ram_map.csv
	@mkdir -p build/misc
	@python3 tools/build_c.py $< $@

.PHONY: misc-id-compatibility-match
misc-id-compatibility-match: verify-rom build/misc/id_compatibility.bin
	@python3 tools/compare_slice.py baserom.gba 0x5d878 build/misc/id_compatibility.bin

matching: misc-id-compatibility-match

# --- permuter kancasi -------------------------------------------------
# decomp-permuter, `make --always-make --dry-run --debug=j PERMUTER=1`
# ciktisinda kaynak dosyanin gectigi bir derleme komuti arar. Kural
# YALNIZCA PERMUTER=1 ile tanimlanir ki normal derlemeyi etkilemesin,
# ve `@` KULLANMAZ: komut kuru calistirmada gorunmeli.
# Permuter `make ... PERMUTER=1` komutunu HEDEFSIZ calistirir, yani varsayilan
# hedefin ciktisina bakar. Bu yuzden PERMUTER=1 altinda varsayilan hedef tum
# C kaynaklarini derleyen bir listeye cevrilir; boylece aranan dosyanin
# derleme komutu kuru calistirmada mutlaka gorunur.
ifdef PERMUTER
PERMUTER_SRCS := $(shell find src -name '*.c')
PERMUTER_OBJS := $(patsubst %.c,build/permuter/%.o,$(PERMUTER_SRCS))
.DEFAULT_GOAL := permuter-all
.PHONY: permuter-all
permuter-all: $(PERMUTER_OBJS)

build/permuter/%.o: %.c
	python3 tools/permuter_compile.py -o $@ $<
endif
