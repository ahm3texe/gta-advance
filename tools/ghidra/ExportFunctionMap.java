// Export the current Ghidra function database as stable CSV.
//@category GTAAdvance

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

public class ExportFunctionMap extends GhidraScript {
    private String csv(String value) {
        return "\"" + value.replace("\"", "\"\"") + "\"";
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) {
            throw new IllegalArgumentException("Expected one output CSV path");
        }

        File output = new File(args[0]);
        File parent = output.getParentFile();
        if (parent != null) {
            parent.mkdirs();
        }

        int count = 0;
        try (PrintWriter writer = new PrintWriter(output, "UTF-8")) {
            writer.println("address,name,size,calling_convention,external,thunk");
            FunctionIterator functions = currentProgram.getFunctionManager().getFunctions(true);
            while (functions.hasNext() && !monitor.isCancelled()) {
                Function function = functions.next();
                writer.printf("0x%08X,%s,%d,%s,%s,%s%n",
                    function.getEntryPoint().getOffset(),
                    csv(function.getName()),
                    function.getBody().getNumAddresses(),
                    csv(function.getCallingConventionName()),
                    function.isExternal(),
                    function.isThunk());
                count++;
            }
        }
        println("Exported " + count + " functions to " + output.getAbsolutePath());
    }
}

