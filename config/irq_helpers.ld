SECTIONS
{
    . = 0x08000730;
    .text.irq_helpers : { *(.text.irq_helpers) }
}

