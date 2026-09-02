// Export one function's initial Ghidra C output for analysis notes.
//@category GTAAdvance

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class ExportDecompile extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) {
            throw new IllegalArgumentException("Expected address and output path");
        }

        Address address = toAddr(Long.decode(args[0]));
        Function function = getFunctionAt(address);
        if (function == null) {
            throw new IllegalStateException("No function at " + address);
        }

        DecompInterface decompiler = new DecompInterface();
        try {
            decompiler.toggleCCode(true);
            decompiler.toggleSyntaxTree(true);
            decompiler.setSimplificationStyle("decompile");
            if (!decompiler.openProgram(currentProgram)) {
                throw new IllegalStateException(decompiler.getLastMessage());
            }
            DecompileResults result = decompiler.decompileFunction(function, 120, monitor);
            if (!result.decompileCompleted() || result.getDecompiledFunction() == null) {
                throw new IllegalStateException(result.getErrorMessage());
            }

            File output = new File(args[1]);
            output.getParentFile().mkdirs();
            try (PrintWriter writer = new PrintWriter(output, "UTF-8")) {
                writer.println("/* Initial Ghidra output; analysis aid, not accepted source code. */");
                writer.println("/* Function: " + function.getName() + " @ " + function.getEntryPoint() + " */");
                writer.println(result.getDecompiledFunction().getC());
            }
            println("Exported decompile to " + output.getAbsolutePath());
        } finally {
            decompiler.dispose();
        }
    }
}

