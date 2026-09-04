.PHONY: check check-full status status-update status-check queue-check boundary-check boundary-baseline toolchain-check toolchain-corpus consistency rom agbcc c-match c-status c-review diff disasm scan-libc libc-align libc-verify dashboard-watch prepare-rom verify-rom doctor progress dashboard-data dashboard-dev dashboard-build dashboard-lint analyze sync-functions bootstrap-match intr-match init-interrupts-match game-init-match vblank-match irq-helpers-match reset-display-match init-save-system-match read-eeprom-match write-eeprom-match save-slots-match save-wrappers-match save-manager-match read-eeprom-range-match write-eeprom-range-match save-helpers-match menu-layout-match draw-menu-match init-menu-screen-match menu-helpers-match menu-graphics-match matching

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

matching: libc-verify bootstrap-match intr-match init-interrupts-match game-init-match vblank-match irq-helpers-match reset-display-match init-save-system-match read-eeprom-match write-eeprom-match save-slots-match save-wrappers-match save-manager-match read-eeprom-range-match write-eeprom-range-match save-helpers-match menu-layout-match draw-menu-match init-menu-screen-match menu-helpers-match menu-graphics-match world-entity-accessors-match misc-state-getters-match misc-table-lookup-match misc-record-table-match misc-session-reset-match text-draw-text-match world-entity-flags-match misc-session-node-match ui-menu-loop-match world-map-tiles-match bios-match misc-coord-accessors-match world-object-helpers-match core-linked-list-match world-object-state-match world-actor-states-match world-slot-config-match world-slot-table-match world-stat-counters-match world-slot-query-match world-node-search-match world-actor-control-match world-area-flags-match world-object-value-match world-table-entries-match world-pause-helpers-match world-slot-selectors-match world-list-head-match world-more-counters-match world-pool-gets-match world-threshold-match world-state-init-match world-gRam02030330-gets-match world-slot-scan-match world-list-ops-match world-map-tile-fields-match world-comm-flag-match world-pair-lookup-match world-tile-and-map-match world-anchor-reset-match world-slot-range-match world-word-compare-match world-ram-flags-match world-more-counters-2-match world-more-counters-3-match core-list-ops2-match world-ram-state-match world-window-config-match misc-coord-more-match world-id-verify-match world-flag-arrays-match world-scan-active-match world-actor-check-match world-frame-chain-match world-dma-flush-match world-slot-release-match world-counter-saturate-match world-set-index-match world-submit-object-match world-scan-all-match world-maybe-advance-match world-entity-query-match world-object-query-match world-get-inner-id-match world-is-ram-mode-match world-offset-helpers-match world-actor-init-match world-kind-scan-match world-history-push-match world-distance-accum-match world-bump-or-reset-match world-release-slot-match world-pool-first-match world-copy-flag-byte-match world-get-slot-unk10-match world-is-mode-two-match world-submit-pack-match core-read-triple-match world-notify-if-ready-match world-bump-save-counter-match world-masked-compare-match world-get-anchor-unk34-match world-forward-with-zero-match world-zero-three-flags-match
	@python3 tools/verify_matching_regions.py

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
