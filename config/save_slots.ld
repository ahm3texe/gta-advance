SECTIONS
{
    . = 0x08000b00;
    .text.save_slots : { *(.text.save_slots) }
}

