.PHONY: check rom agbcc c-match c-status c-review diff disasm scan-libc libc-align libc-verify dashboard-watch prepare-rom verify-rom doctor progress dashboard-data dashboard-dev dashboard-build analyze sync-functions bootstrap-match intr-match init-interrupts-match game-init-match vblank-match irq-helpers-match reset-display-match init-save-system-match read-eeprom-match write-eeprom-match save-slots-match save-wrappers-match save-manager-match read-eeprom-range-match write-eeprom-range-match save-helpers-match menu-layout-match draw-menu-match init-menu-screen-match menu-helpers-match menu-graphics-match matching

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

# Commit oncesi tek komut: her seyi dogrular.
check: rom
	@python3 tools/scan_c_sources.py
	@python3 tools/review_c_source.py
	@python3 tools/progress.py
	@python3 tools/generate_dashboard_data.py

# Tam ROM'u yeniden uretir: dogrulanmis bolgeler kendi kaynagimizdan,
# kalani baserom.gba'dan. Sonucun SHA-1'i orijinalle ayni olmali.
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

matching: libc-verify bootstrap-match intr-match init-interrupts-match game-init-match vblank-match irq-helpers-match reset-display-match init-save-system-match read-eeprom-match write-eeprom-match save-slots-match save-wrappers-match save-manager-match read-eeprom-range-match write-eeprom-range-match save-helpers-match menu-layout-match draw-menu-match init-menu-screen-match menu-helpers-match menu-graphics-match world-entity-accessors-match misc-state-getters-match misc-table-lookup-match misc-record-table-match misc-session-reset-match text-draw-text-match world-entity-flags-match misc-session-node-match ui-menu-loop-match world-map-tiles-match bios-match misc-coord-accessors-match world-object-helpers-match core-linked-list-match world-object-state-match world-actor-states-match world-slot-config-match world-slot-table-match world-stat-counters-match world-slot-query-match world-node-search-match world-actor-control-match world-area-flags-match world-object-value-match world-table-entries-match world-pause-helpers-match world-slot-selectors-match world-list-head-match
	@python3 tools/verify_matching_regions.py
