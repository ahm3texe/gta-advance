// Seed a raw GTA Advance ROM with its known ARM/Thumb entry points.
//@category GTAAdvance

import java.math.BigInteger;

import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.lang.Register;
import ghidra.program.model.lang.RegisterValue;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;

public class BootstrapGba extends GhidraScript {
    private void seedArm(long offset, String name, boolean entryPoint) throws Exception {
        Address address = toAddr(offset);
        disassemble(address);
        Function function = getFunctionAt(address);
        if (function == null) {
            function = createFunction(address, name);
        } else {
            function.setName(name, SourceType.USER_DEFINED);
        }
        if (entryPoint) {
            currentProgram.getSymbolTable().addExternalEntryPoint(address);
        }
        println("Seeded ARM function " + name + " at " + address);
    }

    private void seedThumb(long offset, String name) throws Exception {
        Address address = toAddr(offset);
        Register tmode = currentProgram.getProgramContext().getRegister("TMode");
        RegisterValue thumbMode = new RegisterValue(tmode, BigInteger.ONE);
        AddressSet start = new AddressSet(address, address);
        DisassembleCommand command = new DisassembleCommand(start, null, true);
        command.setInitialContext(thumbMode);
        command.applyTo(currentProgram, monitor);

        Function function = getFunctionAt(address);
        if (function == null) {
            function = createFunction(address, name);
        } else {
            function.setName(name, SourceType.USER_DEFINED);
        }
        println("Seeded Thumb function " + name + " at " + address);
    }

    @Override
    public void run() throws Exception {
        if (!currentProgram.getLanguageID().toString().equals("ARM:LE:32:v4t")) {
            throw new IllegalStateException("Expected ARM:LE:32:v4t, got " + currentProgram.getLanguageID());
        }

        seedArm(0x080000c0L, "AgbMain", true);
        seedArm(0x08000104L, "IntrMain", false);
        seedThumb(0x08000220L, "VBlankIntr");
        seedThumb(0x0800038cL, "InitInterrupts");
        seedThumb(0x08000430L, "GameInit");
        seedThumb(0x08000730L, "NoOpVBlankFinalize");
        seedThumb(0x08000734L, "DummyIntr");
        seedThumb(0x08000738L, "RunVBlankTransfers");
        seedThumb(0x08000798L, "NoOpInterruptHelper");
        seedThumb(0x0800079cL, "VCountIntr");
        seedThumb(0x080007b4L, "ResetDisplayAndInterrupts");
        seedThumb(0x0800082cL, "InitSaveSystem");
        seedThumb(0x0800091cL, "ReadEepromBytes");
        seedThumb(0x080009ecL, "WriteEepromBytes");
        seedThumb(0x08000b00L, "WriteSaveSlot");
        seedThumb(0x08000b78L, "ReadSaveSlot");
        seedThumb(0x08000be4L, "IsSaveSlotValid");
        seedThumb(0x08000c00L, "ReadSaveMetadata");
        seedThumb(0x08000c14L, "WriteSaveMetadata");
        seedThumb(0x08000c28L, "InitSaveManager");
        seedThumb(0x08000d20L, "LoadSaveSlot");
        seedThumb(0x08000d80L, "WriteGameSaveSlot");
        seedThumb(0x08000ddcL, "ReadEepromRange");
        seedThumb(0x08000f1cL, "WriteEepromRange");
        seedThumb(0x08001094L, "EraseSaveSlot");
        seedThumb(0x080010d4L, "GetSaveSlotHeader");
        seedThumb(0x080010f8L, "ReadU8");
        seedThumb(0x080010fcL, "ReadU16LE");
        seedThumb(0x08001108L, "ReadU32LE");
        seedThumb(0x08001120L, "WriteU8");
        seedThumb(0x08001124L, "WriteU16LE");
        seedThumb(0x08001130L, "WriteU32LE");
        seedThumb(0x0800114cL, "BuildActiveMenuItems");
        seedThumb(0x080011ecL, "DrawMenuItems");
        seedThumb(0x080013acL, "InitMenuScreen");
        seedThumb(0x08001458L, "RunMenuScreen");
        seedThumb(0x08001dc0L, "ResetMenuState");
        seedThumb(0x08001e00L, "IsMenuFlagSet");
        seedThumb(0x08001e1cL, "FinalizeMenuLayout");
        seedThumb(0x08001e30L, "LoadMenuGraphics");
        seedThumb(0x08001ea4L, "ClearMenuVram");
        seedThumb(0x08001ee4L, "EnableMenuDisplay");
        seedThumb(0x08001ef4L, "EnableMenuDisplayAlt");
        seedThumb(0x0806bcd0L, "LibraryAssertWrapper0");
        seedThumb(0x0806bce0L, "LibraryAssertWrapper1");
        seedThumb(0x0806bcf0L, "LibraryAssertHandler");
    }
}
