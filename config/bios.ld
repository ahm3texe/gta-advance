SECTIONS
{
    . = 0x0806b84c;
    .text : SUBALIGN(1) { *(.text) }
}
