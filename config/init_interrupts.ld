SECTIONS
{
    . = 0x0800038c;
    .text.init_interrupts : { *(.text.init_interrupts) }
}

